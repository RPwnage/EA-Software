/*************************************************************************************************/
/*!
    \file   globalstatsprovider.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_GLOBALSTATSPROVIDER_H
#define BLAZE_STATS_GLOBALSTATSPROVIDER_H

/*** Include files *******************************************************************************/
#include "EASTL/vector_map.h"
#include "statsprovider.h"
#include "globalstatscache.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace Stats
{
// forward definitions
class GlobalStatsProviderResultSet;

class GlobalStatsProviderRow : public StatsProviderRow
{
public:
    ~GlobalStatsProviderRow() override { }

    int64_t getValueInt64(const char8_t* column) const override;
    int32_t getValueInt(const char8_t* column) const override;
    float_t getValueFloat(const char8_t* column) const override;
    const char8_t* getValueString(const char8_t* column) const override;
    EntityId getEntityId() const override;
    int32_t getPeriodId() const override;

private:
    friend class GlobalStatsProviderResultSet;
    GlobalStatsProviderRow(const GlobalStatsProviderResultSet &resultSet, const GlobalCacheRow &row)
        : StatsProviderRow((const StatsProviderResultSet&)resultSet), mRow(row) {}

    const GlobalCacheRow &mRow;
};

class GlobalStatsProviderResultSet : public StatsProviderResultSet
{
public:
    ~GlobalStatsProviderResultSet() override { };

    StatsProviderRowPtr getProviderRow(const EntityId entityId, const int32_t periodId, const RowScopesVector* rowScopesVector) const override;
    StatsProviderRowPtr getProviderRow(const EntityId entityId, const int32_t periodId, const ScopeNameValueMap* rowScopesMap) const override;
    StatsProviderRowPtr getProviderRow(const ScopeNameValueMap* rowScopesMap) const override;

    bool hasNextRow() const override;
    StatsProviderRowPtr getNextRow() override;
    void reset() override;

    const size_t size() const override;

private:
    friend class GlobalStatsProvider;
    GlobalStatsProviderResultSet(
        const char8_t* category,
        StatsProvider &statsProvider,
        const GlobalCacheCategoryTable &categoryTable)
        : StatsProviderResultSet(category, statsProvider), mCategoryTable(categoryTable) {};

    GlobalCacheCategoryTable::CategoryTable mResultRows;
    const GlobalCacheCategoryTable &mCategoryTable;
    GlobalCacheCategoryTable::CategoryTable::const_iterator mRowIterator;
};

class GlobalStatsProvider : public StatsProvider
{
public:
    GlobalStatsProvider(StatsSlaveImpl& statsSlave, GlobalStatsCache &globalCache);
    ~GlobalStatsProvider() override;

    BlazeRpcError addStatsFetchForUpdateRequest(
        const StatCategory& category,
        EntityId entityId,
        int32_t periodType,
        int32_t periodId,
        const ScopeNameValueSetMap &scopeNameValueSetMap) override;

    BlazeRpcError addDefaultStatsInsertForUpdateRequest(
        const StatCategory& category,
        EntityId entityId,
        int32_t periodType,
        int32_t periodId,
        const RowScopesVector* rowScopes) override;

    BlazeRpcError addStatsUpdateRequest(
        int32_t periodId,
        const UpdateAggregateRowKey &aggKey,
        const StatValMap &statsValMap) override;

    BlazeRpcError addStatsUpdateRequest(
        const StatCategory& category,
        EntityId entityId,
        int32_t periodType,
        int32_t periodId,
        const RowScopesVector* rowScopes,
        const StatPtrValMap &valMap, 
        bool newRow) override;

    BlazeRpcError executeFetchRequest(
        const StatCategory& category,
        const EntityIdList &entityIds,
        int32_t periodType,
        int32_t periodId,
        int32_t periodCount,
        const ScopeNameValueMap &scopeNameValueMap) override;

    BlazeRpcError executeFetchRequest(
        const StatCategory& category,
        const EntityIdList &entityIds,
        int32_t periodType,
        int32_t periodId,
        int32_t periodCount,
        const ScopeNameValueListMap &scopeNameValueListMap) override;

    BlazeRpcError executeFetchForUpdateRequest() override;
    BlazeRpcError executeUpdateRequest() override;

    BlazeRpcError commit() override;
    BlazeRpcError rollback() override;

private:
    GlobalStatsCache &mGlobalCache;
    BlazeRpcError mInternalState;

    void fetchStats(
        const StatCategory& category, 
        EntityId entityId,
        int32_t periodType,
        int32_t periodId,
        const ScopeNameValueSetMap &scopeNameValueSetMap,
        GlobalStatsProviderResultSet* &resultSet);
};

} // Stats
} // Blaze
#endif // BLAZE_STATS_GLOBALSTATSPROVIDER_H
