/*************************************************************************************************/
/*!
    \file   dbstatsprovider.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbscheduler.h"

#include "dbstatsprovider.h"
#include "statsconfig.h"
#include "statsslaveimpl.h"

namespace Blaze
{
namespace Stats
{

int64_t DbStatsProviderRow::getValueInt64(const char8_t* column) const
{
    return mDbRow->getInt64(column); 
}

int32_t DbStatsProviderRow::getValueInt(const char8_t* column) const
{
    return mDbRow->getInt(column); 
}

float_t DbStatsProviderRow::getValueFloat(const char8_t* column) const
{
    return mDbRow->getFloat(column); 
}

const char8_t* DbStatsProviderRow::getValueString(const char8_t* column) const
{
    return mDbRow->getString(column); 
}

EntityId DbStatsProviderRow::getEntityId() const
{
    return mDbRow->getInt64("entity_id");
}

int32_t DbStatsProviderRow::getPeriodId() const
{
    return mDbRow->getInt("period_id");
}

StatsProviderRowPtr DbStatsProviderResultSet::getProviderRow(const EntityId entityId, const int32_t periodId, const RowScopesVector* rowScopesVector) const
{
    // NOTE:
    // Ignoring entityId and periodId because we expect them to be part of 
    // query when we fetch stats for update. This is consequence of adding Stats Provider concept while
    // keeping db code as it was before stats provider was added.
    //

    DbResult::const_iterator it = mDbResult->begin();
    DbResult::const_iterator ite = mDbResult->end();

    // empty result set
    if (it == ite)
        return StatsProviderRowPtr();

    // no scopes? return the first row in result set
    if (rowScopesVector == nullptr)
        return StatsProviderRowPtr(BLAZE_NEW DbStatsProviderRow(*this, *it));

    RowScopesVector::const_iterator rse = rowScopesVector->end();
    for (; it != ite; ++it)
    {
        bool found = true;
        for (RowScopesVector::const_iterator rsit = rowScopesVector->begin(); rsit != rse; ++rsit)
        {
            ScopeValue value = (*it)->getInt64(rsit->mScopeName);
            if (value != rsit->mScopeValue)
            {
                found = false;
                break;
            }
        }
        if (found)
            return StatsProviderRowPtr(BLAZE_NEW DbStatsProviderRow(*this, *it));
    }

    return StatsProviderRowPtr();
}

StatsProviderRowPtr DbStatsProviderResultSet::getProviderRow(const EntityId entityId, const int32_t periodId, const ScopeNameValueMap* rowScopesMap) const
{
    // NOTE:
    // Ignoring entityId and periodId because we expect them to be part of 
    // query when we fetch stats for update. This is consequence of adding Stats Provider concept while
    // keeping db code as it was before stats provider was added.
    return getProviderRow(rowScopesMap);
}

StatsProviderRowPtr DbStatsProviderResultSet::getProviderRow(const ScopeNameValueMap* rowScopesMap) const
{
    //
    // Notice how implementation of this method looks almost the same as the the one that takes 
    // RowScopesVector as parameter. We could just repack ScopeNameValueMap into RowScopesVector and invoke
    // the other method instead. However we'll keep it here to make code more comparable to older versions
    //

    DbResult::const_iterator it = mDbResult->begin();    
    DbResult::const_iterator ite = mDbResult->end();    

    // empty result set
    if (it == ite)
        return StatsProviderRowPtr();

    // no scopes? return the first row in result set
    if (rowScopesMap == nullptr)
        return StatsProviderRowPtr(BLAZE_NEW DbStatsProviderRow(*this, *it));

    Stats::ScopeNameValueMap::const_iterator itme = rowScopesMap->end();
    Stats::ScopeNameValueMap::const_iterator itm;
    for (; it != ite; ++it)
    {
        bool found = true;
        for (itm = rowScopesMap->begin(); itm != itme; ++itm)
        {
            ScopeValue value = (*it)->getInt64(itm->first.c_str());
            if (value != itm->second)
            {
                found = false;
                break;
            }
        }
        
        if (found) 
        {
            return StatsProviderRowPtr(BLAZE_NEW DbStatsProviderRow(*this, *it));
        }
    }

    return StatsProviderRowPtr();
}

void DbStatsProviderResultSet::setDbResult(DbResultPtr dbResult)
{
    mDbResult = dbResult;
    mDbResultIterator = mDbResult->begin();
}

const size_t DbStatsProviderResultSet::size() const
{
    return mDbResult->size();
}

bool DbStatsProviderResultSet::hasNextRow() const
{
    return mDbResultIterator != mDbResult->end();    
}

StatsProviderRowPtr DbStatsProviderResultSet::getNextRow()
{
    if (!hasNextRow())
        return StatsProviderRowPtr();

    return StatsProviderRowPtr(BLAZE_NEW DbStatsProviderRow(*this, *mDbResultIterator++));
}

void DbStatsProviderResultSet::reset()
{
    mDbResultIterator = mDbResult->begin();    
}

DbStatsProvider::DbStatsProvider(StatsSlaveImpl& statsSlave) : 
    StatsProvider(statsSlave)
{
    mResultSetIterator = mResultSetVector.begin();
}

DbStatsProvider::~DbStatsProvider() 
{
}

BlazeRpcError DbStatsProvider::addStatsFetchForUpdateRequest(
    const StatCategory& category,
    EntityId entityId,
    int32_t periodType,
    int32_t periodId,
    const ScopeNameValueSetMap &scopeNameValueSetMap)
{
    BlazeRpcError err = ERR_OK;
    uint32_t dbId;
    err = mComponent.getConfigData()->getShardConfiguration().lookupDbId(
        category.getCategoryEntityType(), entityId, dbId);
    if (err != ERR_OK)
        return err;

    // Either create a new connection or acquire an already open one, and kick off
    // a transaction if we acquire a new one
    DbConnPtr conn;
    QueryPtr query;
    err = mConnMgr.acquireConnWithTxn(dbId, conn, query);
    if (err != ERR_OK)
        return err;

    // prepare resultset for this request
    DbStatsProviderResultSet *resultSet = BLAZE_NEW DbStatsProviderResultSet(category.getName(), *this, dbId);
    mResultSetVector.push_back(resultSet);

    query->append("SELECT * FROM `$s` WHERE `entity_id` = $D", category.getTableName(periodType), entityId);

    if (periodType != STAT_PERIOD_ALL_TIME)
        query->append(" AND `period_id` = $d", periodId);

    if (!scopeNameValueSetMap.empty())
    {
        ScopeNameValueSetMap::const_iterator itScope = scopeNameValueSetMap.begin();
        ScopeNameValueSetMap::const_iterator itendScope = scopeNameValueSetMap.end();
        for (; itScope != itendScope; ++itScope)
        {
            query->append(" AND `$s` IN (", itScope->first.c_str());

            const ScopeValueSet* valueSet = itScope->second;
            size_t valueSetSize = valueSet->size();
            for (size_t i = 0; i < valueSetSize; i++)
            {
                query->append("$D,", (*valueSet)[i]);
            }
            query->trim(1); // trim the comma
            query->append(")");
        }
    }
    query->append(" FOR UPDATE;\n");
    return ERR_OK;
}

BlazeRpcError DbStatsProvider::addDefaultStatsInsertForUpdateRequest(
    const StatCategory& category,
    EntityId entityId,
    int32_t periodType,
    int32_t periodId,
    const RowScopesVector* rowScopes)
{
    BlazeRpcError err = ERR_OK;
    uint32_t dbId;
    err = mComponent.getConfigData()->getShardConfiguration().lookupDbId(
        category.getCategoryEntityType(), entityId, dbId);
    if (err != ERR_OK)
        return err;
    QueryPtr query = mConnMgr.getQuery(dbId);

    query->append("INSERT INTO `$s` SET `entity_id` = $D", category.getTableName(periodType), entityId);
    if (periodType != STAT_PERIOD_ALL_TIME)
        query->append(", period_id = $d", periodId);

    if (rowScopes != nullptr)
    {
        for (RowScopesVector::const_iterator ri = rowScopes->begin(), re = rowScopes->end(); ri != re; ++ri)
        {
            query->append(", `$s` = $D", ri->mScopeName.c_str(), ri->mScopeValue);
        }
    }

    query->append(";\n");
    return ERR_OK;
}

BlazeRpcError DbStatsProvider::addStatsUpdateRequest(
    int32_t periodId,
    const UpdateAggregateRowKey &aggKey, 
    const StatValMap &statsValMap)
{
    // TBD change category name to category pointer in the key 
    const StatCategory* cat = mComponent.getConfigData()->getCategoryMap()->find(aggKey.mCategory)->second;
    const StatMap* statMap = cat->getStatsMap();

    BlazeRpcError err = ERR_OK;
    uint32_t dbId;
    err = mComponent.getConfigData()->getShardConfiguration().lookupDbId(
        cat->getCategoryEntityType(), aggKey.mEntityId, dbId);
    if (err != ERR_OK)
        return err;

    StatValMap::const_iterator valIter = statsValMap.begin();
    StatValMap::const_iterator valEnd = statsValMap.end();
    for (; valIter != valEnd; ++valIter)
    {
        if (valIter->second->changed)
            break;
    }
    if (valIter == valEnd)
    {
        // no stats changed, so don't bother building an UPDATE query for this one
        return ERR_OK;
    }

    QueryPtr query = mConnMgr.getQuery(dbId);

    const char8_t* tableName = cat->getTableName(aggKey.mPeriod);

    // The query is always UPDATE.
    // We rely on new default stats rows to be inserted (via addDefaultStatsInsertForUpdateRequest)
    // immediately after the initial fetch (executeFetchForUpdateRequest).

    query->append("UPDATE `$s` SET `last_modified`=NULL", tableName);

    for (; valIter != valEnd; ++valIter)
    {
        StatMap::const_iterator statIter = statMap->find(valIter->first);
        int32_t type = statIter->second->getDbType().getType();
        if (!valIter->second->changed)
            continue;

        query->append(", `$s` = ", valIter->first);
        switch (type)
        {
            case STAT_TYPE_INT:
                query->append("$D", valIter->second->intVal);
                break;
            case STAT_TYPE_FLOAT:
                query->append("$f", valIter->second->floatVal);
                break;
            case STAT_TYPE_STRING:
                query->append("'$s'", valIter->second->stringVal);
                break;
            default:
                WARN_LOG("[DbStatsProvider].addStatsUpdateRequest(): Invalid stat type(" << type << ") specified.");
                return DB_ERR_SYSTEM;
        }
    }

    query->append(" WHERE `entity_id` = $D", aggKey.mEntityId);

    if (aggKey.mPeriod != STAT_PERIOD_ALL_TIME)
        query->append(" AND `period_id` = $d", periodId);

    for (RowScopesVector::const_iterator ri = aggKey.mRowScopesVector.begin(), re = aggKey.mRowScopesVector.end(); ri != re; ++ri)
    {
        query->append(" AND `$s` = $D", ri->mScopeName.c_str(), ri->mScopeValue);
    }

    query->append(";\n");

    return ERR_OK;
}

BlazeRpcError DbStatsProvider::addStatsUpdateRequest(
    const StatCategory& cat,
    EntityId entityId,
    int32_t periodType,
    int32_t periodId,
    const RowScopesVector* rowScopes,
    const StatPtrValMap &valMap,
    bool /*newRow*/)
{
    BlazeRpcError err = ERR_OK;
    uint32_t dbId;
    err = mComponent.getConfigData()->getShardConfiguration().lookupDbId(
        cat.getCategoryEntityType(), entityId, dbId);
    if (err != ERR_OK)
        return err;

    StatPtrValMap::const_iterator iter = valMap.begin();
    StatPtrValMap::const_iterator end = valMap.end();
    for (; iter != end; ++iter)
    {
        if (iter->second->changed)
            break;
    }
    if (iter == end)
    {
        // no stats changed, so don't bother building an UPDATE query for this one
        return ERR_OK;
    }

    QueryPtr query = mConnMgr.getQuery(dbId);

    // The query is always UPDATE.
    // We rely on new default stats rows to be inserted (via addDefaultStatsInsertForUpdateRequest)
    // immediately after the initial fetch (executeFetchForUpdateRequest).

    query->append("UPDATE `$s` SET `last_modified`=NULL", cat.getTableName(periodType));

    for (; iter != end; ++iter)
    {
        const Stat* stat = iter->first;
        const StatVal* statVal = iter->second;
        if (!statVal->changed)
            continue;

        if (stat->getDbType().getType() == STAT_TYPE_INT)
        {
            query->append(", `$s` = $D", stat->getName(), statVal->intVal);
        }
        else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
        {
            query->append(", `$s` = $f", stat->getName(), (double_t) statVal->floatVal);
        }
        else if (stat->getDbType().getType() == STAT_TYPE_STRING)
        {
            query->append(", `$s` = '$s'", stat->getName(), statVal->stringVal);
        }
    }

    query->append(" WHERE `entity_id` = $D", entityId);

    if (periodType != STAT_PERIOD_ALL_TIME)
        query->append(" AND `period_id` = $d", periodId);

    if (rowScopes != nullptr)
    {
        for (RowScopesVector::const_iterator ri = rowScopes->begin(), re = rowScopes->end(); ri != re; ++ri)
        {
            query->append(" AND `$s` = $D", ri->mScopeName.c_str(), ri->mScopeValue);
        }
    }

    query->append(";\n");
    return ERR_OK;
}

BlazeRpcError DbStatsProvider::executeFetchForUpdateRequest()
{
    TimeValue tStart = TimeValue::getTimeOfDay();

    // Execute queries on (potentially) multiple sharded connections, and make sure to reset
    // the query objects on each conn so that they can be used to prepare the update statements
    BlazeRpcError err = mConnMgr.execute(true);
    mConnMgr.resetQueries();

    // We need to provide the results back in the same order that the caller created the queries,
    // the mResultSetVector is an ordered list that was built up while we created those queries,
    // now by iterating through it and looking at which dbId each query was executed on, we can go
    // to the appropriate db results collection to locate the next result - in theory everything
    // should match up perfectly.
    // Need to clear the owning fiber for any results we may have fetched, even in the event of an error.
    // Not doing so can lead to a double delete of the DbResultPtr when the transaction expires or is aborted
    ResultSetVector::iterator itr = mResultSetVector.begin();
    ResultSetVector::iterator end = mResultSetVector.end();
    for (; itr != end; ++itr)
    {
        DbStatsProviderResultSet* resultSet = (DbStatsProviderResultSet*)(*itr);
        DbResultPtrs::const_iterator dbIter = mConnMgr.getNextResultItr(resultSet->getDbId());
        DbResultPtrs::const_iterator dbEnd = mConnMgr.getEndResultItr(resultSet->getDbId());
        if (dbIter == dbEnd)
        {
            if (err == ERR_OK)
            {
                ERR_LOG("[DbStatsProvider].executeFetchForUpdateRequest: retrieved less results than expected from DB");
                err = DB_ERR_SYSTEM;
            }
            continue;
        }
        resultSet->setDbResult(*dbIter);
        DbResultPtrs::iterator dbIterNonConst = const_cast<DbResultPtrs::iterator>(dbIter);
        (*dbIterNonConst)->clearOwningFiber();
    }

    if (err != ERR_OK)
    {
        TimeValue tEnd = TimeValue::getTimeOfDay();
        tEnd -= tStart;
        ERR_LOG("[DbStatsProvider].executeFetchForUpdateRequest: select query failed after "
            << tEnd.getMillis() << " ms: " << getDbErrorString(err) << ".");

        return err;
    }

    mResultSetIterator = mResultSetVector.begin();

    TRACE_LOG("[DbStatsProvider].executeFetchForUpdateRequest: received " << mResultSetVector.size() << " results back");

    return Blaze::ERR_OK;
}

BlazeRpcError DbStatsProvider::executeUpdateRequest()
{
    BlazeRpcError err = mConnMgr.execute(false);
    mConnMgr.resetQueries();
    return err;
}

BlazeRpcError DbStatsProvider::executeFetchRequest(const StatCategory& category, const EntityIdList& entityIds,
    int32_t periodType, int32_t periodId, int32_t periodCount, const ScopeNameValueMap &scopeNameValueMap)
{
    // convert scopeNameValueMap to ScopeNameValueListMap and call other executeFetchRequest()...
    ScopeNameValueListMap scopeNameValueListMap;
    scopeNameValueListMap.reserve(scopeNameValueMap.size());

    ScopeNameValueMap::const_iterator scopeItr = scopeNameValueMap.begin();
    ScopeNameValueMap::const_iterator scopeEnd = scopeNameValueMap.end();
    for (; scopeItr != scopeEnd; ++scopeItr)
    {
        ScopeValue scopeValue = 0; // no invalid defined, but 0 is fine because...
        // validation should have been done before db conn obtained
        if (scopeItr->second == KEY_SCOPE_VALUE_AGGREGATE)
        {
            scopeValue = mComponent.getConfigData()->getAggregateKeyValue(scopeItr->first.c_str());
        }
        else if (scopeItr->second != KEY_SCOPE_VALUE_ALL)
        {
            scopeValue = scopeItr->second;
        }
        else
        {
            continue;
        }

        ScopeValues& scopeValues = *scopeNameValueListMap.allocate_element();
        scopeNameValueListMap[scopeItr->first.c_str()] = &scopeValues;
        scopeValues.getKeyScopeValues()[scopeValue] = scopeValue;
    }

    return DbStatsProvider::executeFetchRequest(category, entityIds, periodType, periodId, periodCount, scopeNameValueListMap);
}

BlazeRpcError DbStatsProvider::executeFetchRequest(const StatCategory& category, const EntityIdList& entityIds,
    int32_t periodType, int32_t periodId, int32_t periodCount, const ScopeNameValueListMap &scopeNameValueListMap)
{
    BlazeRpcError err = ERR_OK;
    const ShardConfiguration& shardConfig = mComponent.getConfigData()->getShardConfiguration();
    if (entityIds.empty())
    {
        const DbIdList& dbIds = shardConfig.getDbIds(category.getCategoryEntityType());
        for (DbIdList::const_iterator itr = dbIds.begin(); itr != dbIds.end(); ++itr)
        {
            err = executeFetchRequest(*itr, category, entityIds, periodType, periodId, periodCount, scopeNameValueListMap);
            if (err != ERR_OK)
                return err;
        }
    }
    else
    {
        ShardEntityMap shardEntityMap;
        shardConfig.lookupDbIds(category.getCategoryEntityType(), entityIds, shardEntityMap);
        for (ShardEntityMap::const_iterator itr = shardEntityMap.begin(); itr != shardEntityMap.end(); ++itr)
        {
            err = executeFetchRequest(itr->first, category, itr->second, periodType, periodId, periodCount, scopeNameValueListMap);
        }
    }
    return ERR_OK;
}

BlazeRpcError DbStatsProvider::executeFetchRequest(uint32_t dbId, const StatCategory &category, const EntityIdList &entityIds, 
    int32_t periodType, int32_t periodId, int32_t periodCount, const ScopeNameValueListMap &scopeNameValueListMap)
{
    BlazeRpcError err;
    DbConnPtr conn = gDbScheduler->getReadConnPtr(dbId);
    if (conn == nullptr)
    {
        ERR_LOG("[DbStatsProvider].executeFetchRequest: failed to obtain connection.");
        return DB_ERR_SYSTEM;
    }
    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    query->append("SELECT * FROM `$s` WHERE TRUE", category.getTableName(periodType));

    if (!entityIds.empty())
    {
        query->append(" AND `entity_id` IN (");

        GetStatsRequest::EntityIdList::const_iterator entityIter = entityIds.begin();
        GetStatsRequest::EntityIdList::const_iterator entityEnd = entityIds.end();

        while (entityIter != entityEnd)
        {
            query->append("$D", *entityIter++);
            if (entityIter != entityEnd)
                query->append(", ");
        }

        query->append(")");
    }

    if (periodType != STAT_PERIOD_ALL_TIME)
    {
        if (periodCount > 1)
        {
            query->append(" AND `period_id` > $d AND `period_id` <= $d", periodId - periodCount, periodId);
        }
        else
        {
            query->append(" AND `period_id` = $d", periodId);
        }
    }

    // add keyscopes
    ScopeNameValueListMap::const_iterator scopeItr = scopeNameValueListMap.begin();
    ScopeNameValueListMap::const_iterator scopeEnd = scopeNameValueListMap.end();
    for (; scopeItr != scopeEnd; ++scopeItr)
    {
        const ScopeValues* scopeValues = scopeItr->second;
        ScopeStartEndValuesMap::const_iterator valuesItr = scopeValues->getKeyScopeValues().begin();
        ScopeStartEndValuesMap::const_iterator valuesEnd = scopeValues->getKeyScopeValues().end();
        if (valuesItr == valuesEnd)
        {
            continue; // for safety -- this shouldn't happen
        }
        if (scopeValues->getKeyScopeValues().size() == 1)
        {
            // check for single value keyscope
            if (valuesItr->first == valuesItr->second)
            {
                query->append(" AND `$s` = $D", scopeItr->first.c_str(), valuesItr->second);
                continue;
            }
        }

        query->append(" AND `$s` IN (", scopeItr->first.c_str());
        for (; valuesItr != valuesEnd; ++valuesItr)
        {
            ScopeValue scopeValue;
            for (scopeValue = valuesItr->first; scopeValue <= valuesItr->second; ++scopeValue)
            {
                query->append("$D,", scopeValue);
            }
        }
        query->trim(1);
        query->append(")");
    }

    if (entityIds.empty())
    {
        query->append(" LIMIT $u", mComponent.getConfigData()->getMaxRowsToReturn());
    }

    query->append(";"); 

    DbResultPtr dbResult;
    err = conn->executeQuery(query, dbResult);

    TRACE_LOG("[DbStatsProvider].executeFetchRequest: query done");

    if (err != DB_ERR_OK)
    {
        TRACE_LOG("[DbStatsProvider].executeFetchRequest: Error executing query: " << getDbErrorString(err));
        return err;
    }

    // create result sets
    DbStatsProviderResultSet *resultSet = BLAZE_NEW DbStatsProviderResultSet(category.getName(), *this, dbId);
    resultSet->setDbResult(dbResult);
    mResultSetVector.push_back(resultSet);

    mResultSetIterator = mResultSetVector.begin();

    return Blaze::ERR_OK;
}

BlazeRpcError DbStatsProvider::commit()
{
    return mConnMgr.commit();
}

BlazeRpcError DbStatsProvider::rollback()
{
    return mConnMgr.rollback();
}

} // Stats
} // Blaze
