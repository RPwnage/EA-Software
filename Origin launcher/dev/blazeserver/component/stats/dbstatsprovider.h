/*************************************************************************************************/
/*!
    \file   DbStatsprovider.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_DBSTATSPROVIDER_H
#define BLAZE_STATS_DBSTATSPROVIDER_H

/*** Include files *******************************************************************************/
#include "shard.h"
#include "statsprovider.h"
#include "framework/database/dbresult.h"
#include "framework/database/dbconn.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{

namespace Stats
{

class DbStatsProviderResultSet;

class DbStatsProviderRow : public StatsProviderRow
{
public:
    ~DbStatsProviderRow() override { }

    int64_t getValueInt64(const char8_t* column) const override;
    int32_t getValueInt(const char8_t* column) const override;
    float_t getValueFloat(const char8_t* column) const override;
    const char8_t* getValueString(const char8_t* column) const override;
    EntityId getEntityId() const override;
    int32_t getPeriodId() const override;

private:
    friend class DbStatsProviderResultSet;
    DbStatsProviderRow(const DbStatsProviderResultSet &resultSet, const DbRow *dbRow)
        : StatsProviderRow((const StatsProviderResultSet&)resultSet), mDbRow(dbRow) {}

    const DbRow *mDbRow;
};

class DbStatsProviderResultSet : public StatsProviderResultSet
{
public:
    ~DbStatsProviderResultSet() override { }

    StatsProviderRowPtr getProviderRow(const EntityId entityId, const int32_t periodId, const RowScopesVector* rowScopesVector) const override;
    StatsProviderRowPtr getProviderRow(const EntityId entityId, const int32_t periodId, const ScopeNameValueMap* rowScopesMap) const override;
    StatsProviderRowPtr getProviderRow(const ScopeNameValueMap* rowScopesMap) const override;
    const size_t size() const override;

    bool hasNextRow() const override;
    StatsProviderRowPtr getNextRow() override;
    void reset() override;

    uint32_t getDbId() const { return mDbId; }

private:
    friend class DbStatsProvider;
    DbStatsProviderResultSet(
        const char8_t* category,
        StatsProvider &statsProvider, uint32_t dbId)
        : StatsProviderResultSet(category, statsProvider), mDbId(dbId)
    {
    }

    void setDbResult(DbResultPtr dbResult);

    uint32_t mDbId;
    DbResultPtr mDbResult;
    DbResult::const_iterator mDbResultIterator;
};


class DbStatsProvider : public StatsProvider
{
public:
    DbStatsProvider(StatsSlaveImpl& statsSlave);
    ~DbStatsProvider() override;

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
    BlazeRpcError executeFetchRequest(
        uint32_t dbId,
        const StatCategory& category,
        const EntityIdList& entityIds,
        int32_t periodType,
        int32_t periodId,
        int32_t periodCount,
        const ScopeNameValueListMap &scopeNameValueListMap);

    StatsDbConnectionManager mConnMgr;
};

} // Stats
} // Blaze
#endif // BLAZE_STATS_DBSTATSPROVIDER_H
