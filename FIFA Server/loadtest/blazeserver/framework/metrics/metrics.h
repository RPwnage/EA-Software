/*************************************************************************************************/ /*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*
 * 
 * Refer to https://developer.ea.com/display/blaze/Blaze+Operational+Metrics for a detailed
 * description of the functionality of this metrics system and how to use it.
 *
 */

#ifndef BLAZE_METRICS_H
#define BLAZE_METRICS_H

#include "EASTL/string.h"
#include "EASTL/fixed_string.h"
#include "EASTL/vector.h"
#include "EASTL/map.h"
#include "EASTL/tuple.h"
#include "EASTL/fixed_vector.h"
#include "EASTL/fixed_function.h"
#include "framework/tdf/usersessiontypes_server.h"
#include "framework/tdf/userdefines.h"
#include "framework/logger.h"
#include "framework/controller/metricsexporter.h"

#include "EATDF/time.h"
#include "framework/blazedefines.h"


// Defining this enables some debug functionality.  At time of writing this includes:
//     1. Exported metrics are written to files in the cwd named:
//            metrics-<serviceName>-<instanceName>.statsd
//     2. The blazecontroller.getMetricsSchema() RPC will also write a csv files of all the
//        metrics to the cwd named:
//            metrics-<serviceName>-<instanceName>.schema.csv
//#define BLAZE_ENABLE_METRICS_DEBUG


// If set, the OI metrics will include the Tag groups:
// The tag groups include the tags in the key name (ex. gamemanager_master.activeGames.game_type.game_pingsite.game_mode)
//#define BLAZE_OI_METRICS_INCLUDE_TAG_GROUPS

// If set, the Metrics (OI and internally stored) will *only* include the explicitly set Tag groups.  (Metrics without tags groups are unaffected.)
// This means it is not possible to query for metric tag combinations that were not part of a tag group.
//#define BLAZE_METRICS_SKIP_FULL_TAG_GROUP

// Combination explanations: 
// (Neither defined)                    - OI Metrics only include Full Tag Group.  Full Tag Metrics stored in blaze, in addition to Tag Group values. 
// BLAZE_OI_METRICS_INCLUDE_TAG_GROUPS  - OI Metrics and values stored in Blaze contain Full Tag Metrics and Tag Group values. 
// BLAZE_METRICS_SKIP_FULL_TAG_GROUP    - (Invalid) OI Metrics are missing for metrics with Tag Groups.    Only Tag Group values are stored in blaze.
// BLAZE_OI_METRICS_INCLUDE_TAG_GROUPS + BLAZE_METRICS_SKIP_FULL_TAG_GROUP -  Only TagGroups are sent to OI.  No Full Tags metrics are stored.

namespace Blaze
{

class UdpMetricsInjector;

namespace Metrics
{

class MetricBase;
class ExportInfo;
class Filter;

// Tagged metrics which have tags where the value is naturally a string value provide less type
// safety than cases where the tag value is a more specific data type (like an enum).  The
// following "using" statements are meant to be used as template arguments to tagged metrics
// to make it clear what the string parameter is meant to represent.
using BlazeSdkVersion = const char8_t*;
using ProductName = const char8_t*;
using Sandbox = const char8_t*;
using GameReportName = const char8_t*;
using PingSite = const char8_t*;
using FiberContext = const char8_t*;
using DbQueryName = const char8_t*;
using DbPoolName = const char8_t*;
using LogLocation = const char8_t*;
using ClusterName = const char8_t*;
using Hostname = const char8_t*;
using EndpointName = const char8_t*;
using GameMode = const char8_t*;
using PlatformList = const eastl::string;
using ExternalServiceName = const char8_t*;
using CommandUri = const char8_t*;
using ResponseCode = const char8_t*;

// Represents the value portion of a tag/value pair.
using TagValue = eastl::fixed_string<char, 32>;

// Convenience functions for common data types which can be used when defining TagInfo<> instances.
const char8_t* int64ToString(const int64_t& value, TagValue& buffer);
const char8_t* uint32ToString(const uint32_t& value, TagValue& buffer);
const char8_t* int32ToString(const int32_t& value, TagValue& buffer);

class TagInfoBase;
using TagInfoList = eastl::vector<TagInfoBase*>;
using TagInfoLists = eastl::vector<TagInfoList>;

// See definition of TagInfo class for description of purpose.
class TagInfoBase
{
    NON_COPYABLE(TagInfoBase);

public:
    // Size of function capture argument space (increase if needed)
    static const uint32_t FIXED_FUNCTION_SIZE_BYTES = 32;
    const char8_t* getName() const { return mName; }

protected:
    TagInfoBase(const char8_t* name)
        : mName(name)
    {
        TagInfoBase::registerInfo(this);
    }

protected:
    // TagInfo should only be destroyed on process exit
    ~TagInfoBase() { }

private:
    const char8_t* mName;

    static void registerInfo(TagInfoBase* info);

    class Registry
    {
    public:
        ~Registry();

        EA::Thread::Mutex mMutex;
        TagInfoList mTagInfoList;
    };
};

// This class is used to represent a tag in the metrics system.  A tag is an attribute/value pair
// that is associated with a metric to provide dimensioned information.  Tags are strongly typed
// in the metrics system and this class provides a way to obtain both the name of the tag as
// well as a conversion function to generate a string representation of the tag's value.
template<typename T>
class TagInfo : public TagInfoBase
{
    NON_COPYABLE(TagInfo);

public:
    // Implementations of this function must comply with the following rules:
    // 1. Return a const pointer to the string value of T
    // 2. If there is no constant string available, use the provided TagValue reference to
    //    build the string and then return TagValue.c_str() from the function.
    // 3. The returned value will fall out of scope at the same time as the passed in T parameter
    //    so it is okay to return a pointer referencing T.
    using StringConversionFunction = eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, const char8_t* (const T&, TagValue&)>;

    TagInfo(const char8_t* name, StringConversionFunction valueToStringFunc)
        : TagInfoBase(name)
        , mValueToStringFunc(valueToStringFunc)
    {
    }

    const char8_t* getValue(const T& value, TagValue& buffer) const { return mValueToStringFunc(value, buffer); }

protected:
    // TagInfo objects should only be destroyed on process exit
    ~TagInfo() { }

private:
    StringConversionFunction mValueToStringFunc;
};

// Because string tag values are common, this definition provides a specialization for strings
// that makes it slightly more convenient when defining a TagInfo and also slightly better
// performance.
template<>
class TagInfo<const char8_t*> : public TagInfoBase
{
    NON_COPYABLE(TagInfo);

public:
    TagInfo(const char8_t* name) : TagInfoBase(name) { }

    const char8_t* getValue(const char8_t* const & value, TagValue& buffer) const { return value; }

protected:
    // TagInfo should only be destroyed on process exit to prevent accidental deletion.
    ~TagInfo() { }
};

// Represents a tag instance (attribute/value pair) associated with a metric.
struct TagPair : public eastl::pair<TagInfoBase*, TagValue>
{
    TagPair() : eastl::pair<TagInfoBase*, TagValue>() { }
    TagPair(TagInfoBase* x, const TagValue& y) : eastl::pair<TagInfoBase*, TagValue>(x, y) { }
    TagPair(TagInfoBase* x, const eastl::string& y) : eastl::pair<TagInfoBase*, TagValue>(x, y.c_str()) { }
    TagPair(TagInfoBase* x, const char8_t* y) : eastl::pair<TagInfoBase*, TagValue>(x, y) { }
};

// Used primarily for internal optimization to reduce allocation/copying.
using TagPairRef = eastl::pair<TagInfoBase*, const char8_t*>;

class TagPairList : public eastl::fixed_vector<TagPair, 8>
{
public:
    TagPairList() { }

    TagPairList(std::initializer_list<TagPair> ilist)
        : eastl::fixed_vector<TagPair, 8>(ilist)
    {
    }

private:
    template<typename> friend class TaggedMetricBase;

    const eastl::string& getTagString() const { return mTagString; }
    void buildTagString() const;

private:
    // Cache a string representation of this tag list.  Used as an optimization for metrics
    // export to reduce memory allocation and buffer copies.
    mutable eastl::string mTagString;

};

using TagPairRefList = eastl::fixed_vector<TagPairRef, 8>;
using TagPairPtrList = eastl::fixed_vector<TagPair*, 8>;
using TagPairConstPtrList = eastl::fixed_vector<const TagPair*, 8>;
using TagMatchMap = eastl::hash_map<const TagPair*, int32_t>;
using TagValues = eastl::fixed_vector<const char8_t*, 4>;
using TagNames = eastl::fixed_vector<const char8_t*, 4>;

struct TagPairHash { size_t operator()(TagPair const& tag) const; };
struct TagValuesHash { size_t operator()(TagValues const& values) const; };
struct TagValuesEqual { bool operator()(TagValues const& a, TagValues const& b) const; };
struct TagPairRefHash { size_t operator()(TagPairRef const& tag) const; };
struct TagPairRefEqual { bool operator()(const TagPair& a, const TagPairRef& b); };
struct TagPairListHash { size_t operator()(TagPairList const& tagPairs) const; };
struct TagPairRefListHash { size_t operator()(TagPairRefList const& tagPairs) const; };
struct TagPairRefListEqual { bool operator()(const TagPairList& a, const TagPairRefList& b); };

using SumsByTagValue = eastl::hash_map<TagValues, uint64_t, TagValuesHash, TagValuesEqual>;


// Metrics are stored centrally in Blaze in an hierarchy of metrics collections.  A metrics
// collection represents a grouping of metrics which share a common tag (name and value of tag
// match).  It is important to note that the hierarchy is meant to help organize metrics which
// share a common set of tags but that this hierarchy is not preserved in the OI system.  All
// metrics in the OI system are simply names with associate tags.  
//
// The hierarchy of metrics collections in Blaze are used as a convenience to ensure that common
// tags (such as service_name, instance_type, instance_name, and component) are included for groups of metrics
// without the need for each metric to explicitly specify them.
//
// The Blaze metrics framework automatically set the first three levels of the hierarchy which
// represent the Blaze service name, instance name and component.  Further levels in the hierarchy
// should be defined when groups of metrics all share the same tag name/value pairs.
class MetricsCollection
{
    NON_COPYABLE(MetricsCollection);

    static const int64_t METRICS_AGENT_RESOLUTION_PERIOD = 60 * 1000 * 1000; // 60 seconds

public:
    MetricsCollection();
    ~MetricsCollection();

    template<typename T>
    MetricsCollection& getCollection(TagInfo<T>* tag, const T& value, bool enableNamePrefixing = false)
    {
        TagValue buffer;
        return getCollectionInternal(tag, tag->getValue(value, buffer), enableNamePrefixing);
    }

    MetricsCollection& getTaglessCollection(const char* value);

    MetricsCollection* getOwner() const { return mOwner; }

    bool configure(const OperationalInsightsExportConfig& config);

    void exportMetrics();

    bool getMetrics(const GetMetricsRequest& request, GetMetricsResponse& response);
    void getMetricsSchema(GetMetricsSchemaResponse& response);

    const TagPair& getKey() const { return mKey; }

    // Make sure that tag value is conformant to statsd protocol. The special characters in protocol are documented at:
    // 1. https://github.com/b/statsd_spec
    // 2. The above page fails to document dimensions which uses '#' and ',' special characters. 

    static const char8_t* sanitizeTagValue(const char8_t* value, char8_t* outbuf, size_t outlen);

    bool addMetric(MetricBase& metric);

private:
    friend class MetricBase;

    MetricsCollection(TagInfoBase* keyTag, const char8_t* keyValue, MetricsCollection& owner, bool enableNamePrefixing);

    void removeMetric(MetricBase& metric);

    void exportStatsdInternal(ExportInfo& info);
    void updateExportFlags(const Filter& exportIncludeFilter, const Filter& exportExcludeFilter, TagPairConstPtrList& tags);
    bool shouldExport(const char8_t* name, TagPairConstPtrList& tags);
    void getMetricsInternal(const Filter& exportFilter, TagPairConstPtrList& tags, GetMetricsResponse& response);
    void getMetricsSchemaInternal(GetMetricsSchemaResponse& response, TagNames& tags);

    void prependNamePrefixes(eastl::string& metricName);

    MetricsCollection& getCollectionInternal(TagInfoBase* tag, const char8_t* value, bool enableNamePrefixing);

    void resolveAgent();

private:
    MetricsCollection* mOwner;
    TagPair mKey;
    eastl::hash_map<TagPair, MetricsCollection*, TagPairHash> mCollections;
    eastl::hash_map<eastl::string, MetricBase&> mMetrics;
    EA::TDF::TimeValue mNextExportTime;
    eastl::string* mTagString;
    Filter* mExportIncludeFilter;
    Filter* mExportExcludeFilter;
    bool mConfigChanged;
    bool mEnableNamePrefixing;
    InetAddress mAgentAddress;
    bool mAgentAddressResolutionPending;
    TimerId mAgentAddressResolutionTimer;
};


class MetricBase
{
    NON_COPYABLE(MetricBase);

public:
    virtual ~MetricBase();

    const eastl::string& getName() const { return mName; }
    MetricsCollection* getOwner() const { return &mOwner; }

    uint32_t getIdleCount() const { return mIdleCount; }
    bool isExported() const { return (mFlags & MetricBase::FLAG_IS_EXPORTED); }

protected:
    MetricBase(MetricsCollection& owner, const char* name, uint32_t flags, TagPairConstPtrList* extraTags);
    virtual void addMetricsSchemaToResponse(GetMetricsSchemaResponse& response, TagNames& tags, const MetricType type);
    virtual void updateExportFlag(const Filter& exportIncludeFilter, const Filter& exportExcludeFilter, TagPairConstPtrList& tags);
    void updateIsExported(TagPairConstPtrList* extraTags);

private:
    virtual void exportStatsdInternal(ExportInfo& info) { }
    virtual void addMetricsToResponse(const Filter& exportFilter, GetMetricsResponse& response, TagPairConstPtrList& tags) { }

protected:
    friend class MetricsCollection;

    MetricsCollection& mOwner;
    eastl::string mName;
    uint32_t mIdleCount;

    // Indicates that this metrics is part of a collection.  Tagged metrics are not.
    static const uint32_t FLAG_IN_COLLECTION = 1;

    // Should this metric be exported at a higher frequency than normal (used for Timers)
    static const uint32_t FLAG_HIGH_FREQUENCY_EXPORT = 2;

    // Should this metric be exported to statsd
    static const uint32_t FLAG_IS_EXPORTED = 4;

    // Prevent this metric from being garbage collected
    static const uint32_t FLAG_SUPPRESS_GARBAGE_COLLECTION = 8;
    
    // Size of function capture argument space (increase if needed)
    static const uint32_t FIXED_FUNCTION_SIZE_BYTES = 40;
    
    // Set of flags controlling behaviours of the metric.
    mutable uint32_t mFlags;
};


// Counters track the count of times something happened on the system.  Counters never decrease
// but the return value of getInterval() is reset to zero each time they are flushed to the Cloud OI gostatsd agent.
// Typical examples of counter values would be the number of times an RPC has been called or the
// number of game sessions that have been created.  Previous versions of Blaze tracked these types
// of value as a running total since the process was started.  Counters in this system are the
// equivalent of the derivative of these total (eg. the number of times a thing happened after
// each flush interval).
class Counter : public MetricBase
{
    NON_COPYABLE(Counter);

public:
    static const MetricType mMetricType = METRIC_COUNTER;

    Counter(MetricsCollection& owner, const char* name)
        : Counter(MetricBase::FLAG_IN_COLLECTION, owner, name, nullptr)
    {
    }

    inline void increment(uint64_t value = 1)
    {
        mCount += value;
    }

    uint64_t getInterval() const
    {
        return mCount - mLastExportedCount;
    }

    uint64_t getTotal() const
    {
        return mCount;
    }

private:
    template<typename>
    friend class TaggedMetricBase;

    Counter(uint32_t flags, MetricsCollection& owner, const char* name, TagPairConstPtrList* extraTags)
        : MetricBase(owner, name, flags, extraTags)
        , mCount(0)
        , mLastExportedCount(0)
        , mPrevExportedDelta(0)
    {
    }

    virtual void exportStatsdInternal(ExportInfo& info) override;
    virtual void addMetricsToResponse(const Filter& exportFilter, GetMetricsResponse& response, TagPairConstPtrList& tags) override;
    virtual void addMetricsSchemaToResponse(GetMetricsSchemaResponse& response, TagNames& tags, const MetricType type) override;

private:
    uint64_t mCount;
    uint64_t mLastExportedCount;

    // We export Counters by delta, rather than sending the full count each time.  If an update is skipped, then the OI system assumes that
    // we want to repeat the last value.  Since we generally don't want to do that, we only stop exporting if our previous delta was 0.
    // Effectively, we will send one 0 after a series of non-0.
    uint64_t mPrevExportedDelta;
};

// These are used to represent a "moment-in-time" value.  The value typically goes up and down
// over time.  Typical examples of gauge usage would be to represent the current number of game
// sessions on the server or the current PSU.
class Gauge : public MetricBase
{
    NON_COPYABLE(Gauge);

public:
    static const MetricType mMetricType = METRIC_GAUGE;

    Gauge(MetricsCollection& owner, const char8_t* name)
        : Gauge(MetricBase::FLAG_IN_COLLECTION, owner, name, nullptr)
    {
    }

    inline uint64_t get() const
    {
        return mValue;
    }

    inline void set(uint64_t value)
    {
        mValue = value;
    }

    inline void increment(uint64_t value = 1)
    {
        mValue += value;
    }

    inline void decrement(uint64_t value = 1)
    {
        if (mValue >= value) {
            mValue -= value;
        }
        else 
        {
            BLAZE_WARN_LOG(Log::METRICS, "[Gauge].decrement: Metric " << mName << " value " << mValue << " will be less than zero after decrementing " << value
                << ". Set to 0 to prevent unsigned integer underflow.");
            mValue = 0;
        }
    }

private:
    template<typename>
    friend class TaggedMetricBase;

    Gauge(uint32_t flags, MetricsCollection& owner, const char8_t* name, TagPairConstPtrList* extraTags)
        : MetricBase(owner, name, flags, extraTags)
        , mValue(0)
        , mPrevValue(0)
    {
    }

    virtual void exportStatsdInternal(ExportInfo& info) override;
    virtual void addMetricsToResponse(const Filter& exportFilter, GetMetricsResponse& response, TagPairConstPtrList& tags) override;
    virtual void addMetricsSchemaToResponse(GetMetricsSchemaResponse& response, TagNames& tags, const MetricType type) override;

private:
    uint64_t mValue;
    uint64_t mPrevValue;
};

// The PolledGauge class does not maintain the current gauge value and a provided function is
// called when the gauge's value is required.  This class should be used in cases where the gauge
// value is derived and it is impractical or inefficient to update the value every time it
// changes.  An example of this might be where the gauge value represents the size of an
// eastl::map.  Rather than updating the gauge value every time an element is added or removed
// from the map, simply use a PolledGauge instance and provide a polling function which returns
// the map's size.
class PolledGauge : public MetricBase
{
    NON_COPYABLE(PolledGauge);

public:
    static const MetricType mMetricType = METRIC_GAUGE;

    using PollerFunction = eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, uint64_t ()>;

    PolledGauge(MetricsCollection& owner, const char8_t* name, PollerFunction poller)
        : MetricBase(owner, name, MetricBase::FLAG_IN_COLLECTION, nullptr)
        , mPollerFunction(poller)
        , mPrevValue(0)
    {
    }

    inline uint64_t get() const
    {
        return mPollerFunction();
    }

private:
    template<typename>
    friend class TaggedMetricBase;

    template<typename, typename...>
    friend class TaggedPolledGauge;

    PolledGauge(uint32_t flags, MetricsCollection& owner, const char8_t* name, TagPairConstPtrList* extraTags)
        : MetricBase(owner, name, flags, extraTags)
        , mPollerFunction([]() { return 0; })
        , mPrevValue(0)
    {
    }

    virtual void exportStatsdInternal(ExportInfo& info) override;
    virtual void addMetricsToResponse(const Filter& exportFilter, GetMetricsResponse& response, TagPairConstPtrList& tags) override;
    virtual void addMetricsSchemaToResponse(GetMetricsSchemaResponse& response, TagNames& tags, const MetricType type) override;

    void setPollerFunction(PollerFunction poller)
    {
        mPollerFunction = poller;
    }

private:
    PollerFunction mPollerFunction;
    uint64_t mPrevValue;
};

// Timer objects are used to measure how long things take on the server.  Individual measurements
// are tracked and exported to the Cloud OI gostatsd agent.  The gostatsd agent then calculates
// useful values such as minimum, maximum, sum, mean, median and 95th percentile for the exported
// times over the flush interval.  A typical example of timer usage would be to track how long
// individual RPC operations take.  Timers can be very useful but it is important to note that
// because individual operations are measured and exported they can have performance implications
// if used for very high rate operations.  The metrics implementation has optimizations in place
// to help manage this but be judicious in usage of timers.
class Timer : public MetricBase
{
    NON_COPYABLE(Timer);

public:
    static const MetricType mMetricType = METRIC_TIMER;

    Timer(MetricsCollection& owner, const char8_t* name)
        : Timer(MetricBase::FLAG_IN_COLLECTION, owner, name, nullptr)
    {
    }

    inline void record(const EA::TDF::TimeValue& time)
    {
        uint64_t us = time.getMicroSeconds();

        if (isExported())
        {
            // Only accumulate samples if we will export the metrics to avoid growing samples without bound
            mTimes.push_back(us);
        }

        mTotalTime += us;
        mTotalCount++;

        if (us > mTotalMax)
            mTotalMax = us;
        if (us < mTotalMin)
            mTotalMin = us;
    }

    uint64_t getTotalCount() const
    {
        return mTotalCount;
    }

    uint64_t getTotalTime() const
    {
        return mTotalTime;
    }

    uint64_t getTotalAverage() const
    {
        return mTotalCount == 0 ? 0 : mTotalTime / mTotalCount;
    }

    uint64_t getTotalMin() const
    {
        return mTotalMin == UINT64_MAX ? 0 : mTotalMin;
    }

    uint64_t getTotalMax() const
    {
        return mTotalMax;
    }

private:
    template<typename>
    friend class TaggedMetricBase;

    Timer(uint32_t flags, MetricsCollection& owner, const char8_t* name, TagPairConstPtrList* extraTags)
        : MetricBase(owner, name, flags | FLAG_HIGH_FREQUENCY_EXPORT, extraTags)
        , mTotalTime(0)
        , mTotalCount(0)
        , mTotalMin(UINT64_MAX)
        , mTotalMax(0)
    {
    }

    virtual void exportStatsdInternal(ExportInfo& info) override;
    virtual void addMetricsToResponse(const Filter& exportFilter, GetMetricsResponse& response, TagPairConstPtrList& tags) override;
    virtual void addMetricsSchemaToResponse(GetMetricsSchemaResponse& response, TagNames& tags, const MetricType type) override;

private:
    eastl::vector<uint64_t> mTimes;

    uint64_t mTotalTime;
    uint64_t mTotalCount;
    uint64_t mTotalMin;
    uint64_t mTotalMax;
};

// The current version only uses the templating of specific functions (constructor, getMetrics)
template<typename T_MetricType>
class TaggedMetricBase : public MetricBase
{
    NON_COPYABLE(TaggedMetricBase);

public:
    virtual ~TaggedMetricBase();

    virtual void reset();

    // The get() function attempts to match the tag list to the directly, and falls back to iteration if no exact match exists:
    bool get(const TagPairList& tags, bool exactTagMatch, eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, void(const TagPairList& tags, const T_MetricType&, bool exactTagMatch)> const& func) const;

    // Iterate functions will attempt to find all values that match the given tags info, but do *not* check the tag values:
    bool iterate(eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, void(const TagPairList& tags, const T_MetricType&)> const& func) const;
    bool iterate(const TagPairList& tags, eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, void (const TagPairList& tags, const T_MetricType&)> const& func) const;


    virtual void exportStatsdInternal(ExportInfo& info) override;

    // Tag groups divide up the metric into multiple groups that don't overlap.  
    // This is useful if you want metrics to only be set in a single place, but with different groupings. 
    // Example usage: mActivePlayers.defineTagGroups({ Tag::network_topology }, 
    //                                                { Tag::game_mode, Tag::game_topology, Tag::ping_site })
    void defineTagGroups(const TagInfoLists& validTags);
    void disableTags(const TagInfoList& tags);   // Replaces the current supported tag list with a restricted one.

protected:
    template<typename T_TagInfo, typename ... T_TagInfoRest>
    TaggedMetricBase(MetricsCollection& owner, const char* name, uint32_t flags, T_TagInfo firstTagInfos, T_TagInfoRest... tagInfos);
    TaggedMetricBase(MetricsCollection& owner, const char* name, uint32_t flags, const TagInfoList& tagInfos);

    bool aggregateBase(const TagInfoList& keys, SumsByTagValue& sums, eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, uint64_t(const T_MetricType&)> const& func) const;

    virtual void addMetricsToResponse(const Filter& exportFilter, GetMetricsResponse& response, TagPairConstPtrList& tags) override;
    virtual void addMetricsSchemaToResponse(GetMetricsSchemaResponse& response, TagNames& tags, const MetricType type) override;

    inline void getMetrics(eastl::list<T_MetricType*>& outValues, const TagPairRefList& tagList);
    template<typename TagType, typename ... TagTypeRest>
    inline void getMetrics(eastl::list<T_MetricType*>& outValues, const TagType& firstTag, const TagTypeRest& ... restTags);


    virtual void updateExportFlag(const Filter& exportIncludeFilter, const Filter& exportExcludeFilter, TagPairConstPtrList& tags) override;


    bool checkAllTagsMatch(const TagPairList& tags) const;
    bool checkAllTagsMatch(const TagInfoList& tags) const;
    TagInfoList mTagInfos;

    bool mAllowTagGroups;
    eastl::list<TaggedMetricBase<T_MetricType>*> mTagGroups;

private:
    eastl::hash_map<TagPairList, T_MetricType*, TagPairListHash> mChildren;

    template<typename TagType>
    inline void getMetrics(eastl::list<T_MetricType*>& outValues, TagPairRefList& tagList, const TagType& tag);
    template<typename TagType, typename ... TagTypeRest>
    inline void getMetrics(eastl::list<T_MetricType*>& outValues, TagPairRefList& tagList, const TagType& firstTag, const TagTypeRest& ... restTags);
};


// Templated version of the Counter metric type which allows metrics to be defined with a well
// known set of tags.  Updates to the counter are type-safe and allow for easy tracking of
// dimensioned counters.
template<typename T_Tag, typename ...T_RestTags>
class TaggedCounter : public TaggedMetricBase<Counter>
{
    NON_COPYABLE(TaggedCounter);

public:
    TaggedCounter(MetricsCollection& owner, const char* name, TagInfo<T_Tag>* firstTag, TagInfo<T_RestTags>*... restTags);

    inline void increment(uint64_t value, const T_Tag& firstTag, const T_RestTags& ... restTags);

    uint64_t getTotal() const;
    uint64_t getTotal(const TagPairList& query) const;

    void aggregate(const TagInfoList& keys, SumsByTagValue& sums) const;
    void reset() override;

protected:
    // mTotal - Used as an fast lookup in the case where we just want the sum:
    uint64_t mTotal;
};

// Templated version of the Gauge metric type which allows metrics to be defined with a well
// known set of tags.  Updates to the gauge are type-safe and allow for easy tracking of
// dimensioned gauges.
template<typename T_Tag, typename ...T_RestTags>
class TaggedGauge : public TaggedMetricBase<Gauge>
{
    NON_COPYABLE(TaggedGauge);

public:
    TaggedGauge(MetricsCollection& owner, const char* name, TagInfo<T_Tag>* firstTag, TagInfo<T_RestTags>*... restTags);

    inline void set(uint64_t value, const T_Tag& firstTag, const T_RestTags& ... restTags);
    inline void increment(uint64_t value, const T_Tag& firstTag, const T_RestTags& ... restTags);
    inline void decrement(uint64_t value, const T_Tag& firstTag, const T_RestTags& ... restTags);

    uint64_t get() const;
    uint64_t get(const TagPairList& query) const;

    void aggregate(const TagInfoList& keys, SumsByTagValue& sums) const;
    void reset() override;

protected:
    // mTotal - Used as an fast lookup in the case where we just want the sum:
    uint64_t mTotal;
};

// Templated version of the Polled Gauge metric type which allows metrics to be defined with
// a well known set of tags.  Updates to the gauge are type-safe and allow for easy tracking
// of dimensioned gauges.
template<typename T_Tag, typename ...T_RestTags>
class TaggedPolledGauge : public TaggedMetricBase<PolledGauge>
{
    NON_COPYABLE(TaggedPolledGauge);

public:
    using PollerFunction = PolledGauge::PollerFunction;

    TaggedPolledGauge(MetricsCollection& owner, const char* name, TagInfo<T_Tag>* firstTag, TagInfo<T_RestTags>*... restTags);

    inline void startOrReconfigure(PollerFunction poller, const T_Tag& firstTag, const T_RestTags& ... restTags);

    uint64_t get() const;
    uint64_t get(const TagPairList& query) const;

    void aggregate(const TagInfoList& keys, SumsByTagValue& sums) const;
};

// Templated version of the Timer metric type which allows metrics to be defined with a well
// known set of tags.  Updates to the timer are type-safe and allow for easy tracking of
// dimensioned timers.
template<typename T_Tag, typename ... T_RestTags>
class TaggedTimer : public TaggedMetricBase<Timer>
{
    NON_COPYABLE(TaggedTimer);

public:
    TaggedTimer(MetricsCollection& owner, const char* name, TagInfo<T_Tag>* firstTag, TagInfo<T_RestTags>*... restTags);

    inline void record(const EA::TDF::TimeValue& time, const T_Tag& firstTag, const T_RestTags& ... restTags);
};

extern EA_THREAD_LOCAL MetricsCollection* gMetricsCollection;
extern EA_THREAD_LOCAL MetricsCollection* gFrameworkCollection;

// Defined all the well-known, component agnostic tag types.  Component/module specific TagInfo
// objects should be defined local to the module.
namespace Tag
{
    extern TagInfo<const char*>* environment;
    extern TagInfo<const char*>* service_name;
    extern TagInfo<const char*>* instance_type;
    extern TagInfo<const char*>* instance_name;
    extern TagInfo<const char*>* component;
    extern TagInfo<const char*>* command;
    extern TagInfo<const char*>* endpoint; // This includes endpoints configured as grpc endpoints.
    extern TagInfo<const char*>* inboundgrpc; // This is for grpc specific metrics.
    extern TagInfo<const char*>* outboundgrpcservice;
    extern TagInfo<ClientType>* client_type;
    extern TagInfo<BlazeRpcError>* rpc_error;
    extern TagInfo<const char*>* blaze_sdk_version;
    extern TagInfo<const char*>* product_name;
    extern TagInfo<ClientPlatformType>* platform;    
    extern TagInfo<PlatformList>* platform_list;
    extern TagInfo<const char*>* sandbox;
    extern TagInfo<UserSessionDisconnectType>* disconnect_type;
    extern TagInfo<ClientState::Status>* client_status;
    extern TagInfo<ClientState::Mode>* client_mode;
    extern TagInfo<const char*>* pingsite;
    extern TagInfo<const char*>* fiber_context;
    extern TagInfo<Fiber::StackSize>* fiber_stack_size;
    extern TagInfo<const char*>* db_query_name;
    extern TagInfo<const char*>* db_pool;
    extern TagInfo<Log::Category>* log_category;
    extern TagInfo<Logging::Level>* log_level;
    extern TagInfo<const char*>* log_location;
    extern TagInfo<const char*>* cluster_name;
    extern TagInfo<const char*>* hostname;
    extern TagInfo<const char*>* sliver_namespace;
    extern TagInfo<const char*>* http_pool;
    extern TagInfo<uint32_t>* http_error;
    extern TagInfo<const char*>* memory_category;
    extern TagInfo<const char*>* memory_subcategory;
    extern TagInfo<const char*>* node_pool;
    extern TagInfo<InetAddress>* ip_addr;
    extern TagInfo<const char*>* external_service_name;
    extern TagInfo<const char*>* command_uri;
    extern TagInfo<const char*>* response_code;
    extern TagInfo<RedisError>* redis_error;

} // namespace Tag

// Convenience functions for common iterate cases.
void addTaggedMetricToComponentStatusInfoMap(ComponentStatus::InfoMap& map, const char8_t* metricBaseName, const TagPairList& tagList, uint64_t value);

// All the inlined functions are declared in metricsinternal.h to keep this main interface
// header a little cleaner
#include "framework/metrics/metricsinternal.h"

} // namespace Metrics
} // namespace Blaze

#endif // BLAZE_METRICS_H

