/*************************************************************************************************/
/*!
    \file   statscache.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_STATSCACHE_H
#define BLAZE_STATS_STATSCACHE_H

#include "stats/statscommontypes.h"
#include "stats/statsprovider.h"

#include "framework/metrics/metrics.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Stats
{

class StatsSlaveImpl;

class CacheRow
{
private:
    NON_COPYABLE(CacheRow);

public:
    CacheRow(const StatCategory& category, const StatsProviderRowPtr row);
    ~CacheRow();

    typedef union
    {
        int64_t intData;
        float_t floatData;
        char8_t* stringData;
    } CacheCell;

    CacheCell* mCacheRow;
    const StatCategory& mCategory;
};

struct CacheKey
{
    CacheKey() { mScopes[0] = '\0'; }
    CacheKey(uint32_t catId, EntityId entityId, int32_t periodType, int32_t periodId, const char8_t* keyScopeString, uint64_t hashValue);
    CacheKey(uint32_t catId, EntityId entityId, int32_t periodType, int32_t periodId, const ScopeNameValueMap* scopeMap, uint64_t hashValue = 0);

    uint32_t mCatId;
    EntityId mEntityId;
    int32_t mPeriodType;
    int32_t mPeriodId;
    char8_t mScopes[128];
    uint64_t mHashValue;
};

class StatsCache
{
private:
    NON_COPYABLE(StatsCache);

public:
    StatsCache(StatsSlaveImpl& component, size_t size);
    ~StatsCache();

    static uint64_t buildHash(uint32_t catId,
        EntityId entityId,
        int32_t periodType,
        int32_t periodId,
        const char8_t* scopes);
    static uint64_t computeCacheRowUpdateHashKey(const CacheRowUpdate& row);

    void reset();
    void resetAndResize(size_t newSize);

    const CacheRow* getRow(
        const StatCategory& cat,
        EntityId entityId,
        int32_t periodType,
        int32_t periodId,
        const ScopeNameValueMap* scopes);
    void insertRow(
        const StatCategory& cat,
        EntityId entityId,
        int32_t periodType,
        int32_t periodId,
        const ScopeNameValueMap* scopes,
        const StatsProviderRowPtr row);
    void deleteRow(
        uint32_t catId,
        EntityId entityId,
        int32_t periodType,
        int32_t periodId,
        const ScopeNameValueMap* scopes,
        uint64_t hashValue = 0);

    void deleteRows(const CacheRowUpdateList& data);
    void deleteRows(const CacheRowDeleteList& data, const Stats::PeriodIdMap& currentPeriodIdMap);

    bool isLruEnabled() const { return mLru != nullptr; }
    size_t getLruMaxSize() const { return (mLru != nullptr ? mLru->getMaxSize() : 0); }

private:
    StatsSlaveImpl& mComponent;

    struct CacheKeyHash
    {
        uint64_t operator()(const CacheKey& key) const;
    };

    struct CacheKeyEqualTo
    {
        bool operator()(const CacheKey& key1, const CacheKey& key2) const;
    };

    static void lruRemoveCb(const CacheKey& key, CacheRow* row);
    typedef Blaze::LruCache<CacheKey, CacheRow*, CacheKeyHash, CacheKeyEqualTo> LruCacheRow;
    LruCacheRow* mLru;

    // metrics
public:
    Metrics::Counter mCacheRowFetchCount;
    Metrics::Counter mDbRowFetchCount;
};

} // Stats
} // Blaze

#endif // BLAZE_STATS_STATSCACHE_H
