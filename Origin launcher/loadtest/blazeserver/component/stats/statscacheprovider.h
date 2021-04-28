/*************************************************************************************************/
/*!
    \file   statscacheprovider.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_STATSCACHEPROVIDER_H
#define BLAZE_STATS_STATSCACHEPROVIDER_H

/*** Include files *******************************************************************************/
#include "dbstatsprovider.h"
#include "statscache.h"
#include "EASTL/hash_map.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace Stats
{

// forward declarations
class StatsCacheProviderResultSet;

typedef struct
{
    StatType type;
    union {
        int64_t intData;
        float_t floatData;
        char8_t* stringData;
    } cell;
} StatsCacheCell;

typedef eastl::hash_map<const char8_t *, StatsCacheCell, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > StatsCacheCellMap;
typedef eastl::list<StatsCacheCellMap *> StatsCacheRowList;

class StatsCacheProviderRow : public StatsProviderRow
{
public:
    ~StatsCacheProviderRow() override {}

    int64_t getValueInt64(const char8_t* column) const override;
    int32_t getValueInt(const char8_t* column) const override;
    float_t getValueFloat(const char8_t* column) const override;
    const char8_t* getValueString(const char8_t* column) const override;
    EntityId getEntityId() const override;
    int32_t getPeriodId() const override;

private:
    friend class StatsCacheProviderResultSet;

    StatsCacheProviderRow(const StatsCacheProviderResultSet& resultSet, const StatsCacheCellMap& row)
        : StatsProviderRow((const StatsProviderResultSet&) resultSet), mRow(row) {}

    const StatsCacheCellMap& mRow;
};

class StatsCacheProviderResultSet : public StatsProviderResultSet
{
public:
    ~StatsCacheProviderResultSet() override;

    StatsProviderRowPtr getProviderRow(const EntityId entityId, const int32_t periodId, const RowScopesVector* rowScopesVector) const override;
    StatsProviderRowPtr getProviderRow(const EntityId entityId, const int32_t periodId, const ScopeNameValueMap* rowScopesMap) const override;
    StatsProviderRowPtr getProviderRow(const ScopeNameValueMap* rowScopesMap) const override;

    const size_t size() const override;

    bool hasNextRow() const override;
    StatsProviderRowPtr getNextRow() override;
    void reset() override;

    int32_t getPeriodId() const { return mPeriodId; }
    const ScopeNameValueMap& getScopeMap() const { return mScopeNameValueMap; }

private:
    friend class StatsCacheProvider;

    StatsCacheProviderResultSet(const StatCategory& category, int32_t periodId, const ScopeNameValueMap& scopeNameValueMap, StatsProvider& statsProvider);

    void addCacheRow(EntityId entityId, const CacheRow& cacheRow);

    int32_t mPeriodId;
    ScopeNameValueMap mScopeNameValueMap;
    StatsCacheRowList mCacheRowList;
    StatsCacheRowList::const_iterator mRowIterator;
};

class StatsCacheProvider : public DbStatsProvider
{
public:
    StatsCacheProvider(StatsSlaveImpl& statsSlave, StatsCache& statsCache);
    ~StatsCacheProvider() override;

    BlazeRpcError executeFetchRequest(
        const StatCategory& category,
        const EntityIdList& entityIds,
        int32_t periodType,
        int32_t periodId,
        int32_t periodCount,
        const ScopeNameValueMap& scopeNameValueMap) override;

    BlazeRpcError executeFetchRequest(
        const StatCategory& category,
        const EntityIdList& entityIds,
        int32_t periodType,
        int32_t periodId,
        int32_t periodCount,
        const ScopeNameValueListMap& scopeNameValueListMap) override;

private:
    BlazeRpcError fetchStats(
        const StatCategory& category,
        const EntityIdList& entityIds,
        int32_t periodType,
        int32_t periodId,
        const ScopeNameValueMap& scopeNameValueMap,
        StatsCacheProviderResultSet*& cacheResultSet);

    void updateCache(
        const StatCategory& category,
        int32_t periodType,
        bool updateDbMetric);

    bool isWildcardFetch(
        const StatCategory& category,
        const EntityIdList& entityIds,
        int32_t periodType,
        int32_t periodId,
        int32_t periodCount,
        const ScopeNameValueMap& scopeNameValueMap) const;

    bool isWildcardFetch(
        const StatCategory& category,
        const EntityIdList& entityIds,
        int32_t periodType,
        int32_t periodId,
        int32_t periodCount,
        const ScopeNameValueListMap& scopeNameValueListMap) const;

private:
    StatsCache& mStatsCache;

    // metrics
public:
    uint64_t mTotalCacheRowHits;
    uint64_t mTotalDbRowHits;
};

} // Stats
} // Blaze

#endif // BLAZE_STATS_STATSCACHEPROVIDER_H
