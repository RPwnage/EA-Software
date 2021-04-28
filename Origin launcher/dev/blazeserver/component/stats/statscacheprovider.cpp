/*************************************************************************************************/
/*!
    \file   statscacheprovider.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "statscacheprovider.h"
#include "shard.h"

namespace Blaze
{
namespace Stats
{

const static eastl::string ENTITY_ID_TAG = "entity_id";

int64_t StatsCacheProviderRow::getValueInt64(const char8_t* column) const
{
    StatsCacheCellMap::const_iterator cellItr = mRow.find(column);
    if (cellItr != mRow.end())
    {
        if ((cellItr->second).type != STAT_TYPE_INT)
        {
            ERR_LOG("[StatsCacheProviderRow].getValueInt64: wrong type: " << (cellItr->second).type);
        }
        return (cellItr->second).cell.intData;
    }

    const ScopeNameValueMap& scopeMap = ((const StatsCacheProviderResultSet&)mProviderResultSet).getScopeMap();
    ScopeNameValueMap::const_iterator scopeItr = scopeMap.find(column);
    if (scopeItr != scopeMap.end())
    {
        return scopeItr->second;
    }

    // shouldn't happen
    ERR_LOG("[StatsCacheProviderRow:].getValueInt64: unknown column: " << column);
    return 0;
}

int32_t StatsCacheProviderRow::getValueInt(const char8_t* column) const
{
    // 32-bit integers are stored as 64bit in stats cache provider
    return (int32_t) getValueInt64(column);
}

float_t StatsCacheProviderRow::getValueFloat(const char8_t* column) const
{
    StatsCacheCellMap::const_iterator cellItr = mRow.find(column);
    if (cellItr != mRow.end())
    {
        if ((cellItr->second).type != STAT_TYPE_FLOAT)
        {
            ERR_LOG("[StatsCacheProviderRow].getValueFloat: wrong type: " << (cellItr->second).type);
        }
        return (cellItr->second).cell.floatData;
    }

    // shouldn't happen
    ERR_LOG("[StatsCacheProviderRow:].getValueFloat: unknown column: " << column);
    return (float_t) 0;
}

const char8_t* StatsCacheProviderRow::getValueString(const char8_t* column) const
{
    StatsCacheCellMap::const_iterator cellItr = mRow.find(column);
    if (cellItr != mRow.end())
    {
        if ((cellItr->second).type != STAT_TYPE_STRING)
        {
            ERR_LOG("[StatsCacheProviderRow].getValueString: wrong type: " << (cellItr->second).type);
            return nullptr;
        }
        return (cellItr->second).cell.stringData;
    }

    // shouldn't happen
    ERR_LOG("[StatsCacheProviderRow:].getValueString: unknown column: " << column);
    return nullptr;
}

EntityId StatsCacheProviderRow::getEntityId() const
{
    return (EntityId) getValueInt64(ENTITY_ID_TAG.c_str());
}

int32_t StatsCacheProviderRow::getPeriodId() const
{
    return ((const StatsCacheProviderResultSet&)mProviderResultSet).getPeriodId();
}

StatsCacheProviderResultSet::StatsCacheProviderResultSet(const StatCategory& category, int32_t periodId, const ScopeNameValueMap& scopeNameValueMap, StatsProvider& statsProvider)
    : StatsProviderResultSet(category.getName(), statsProvider), mPeriodId(periodId)
{
    scopeNameValueMap.copyInto(mScopeNameValueMap);
}

StatsCacheProviderResultSet::~StatsCacheProviderResultSet()
{
    StatsCacheRowList::iterator rowItr = mCacheRowList.begin();
    StatsCacheRowList::iterator rowEnd = mCacheRowList.end();
    for (; rowItr != rowEnd; ++rowItr)
    {
        StatsCacheCellMap* cellMap = *rowItr;

        StatsCacheCellMap::iterator mapItr = cellMap->begin();
        StatsCacheCellMap::iterator mapEnd = cellMap->end();
        for (; mapItr != mapEnd; ++mapItr)
        {
            if ((mapItr->second).type == STAT_TYPE_STRING)
            {
                BLAZE_FREE((mapItr->second).cell.stringData);
            }
        }

        delete cellMap;
    }
}

StatsProviderRowPtr StatsCacheProviderResultSet::getProviderRow(const EntityId entityId, const int32_t periodId, const RowScopesVector* rowScopesVector) const
{
    ERR_LOG("[StatsCacheProviderResultSet].getProviderRow: unsupported");
    return StatsProviderRowPtr();
}

StatsProviderRowPtr StatsCacheProviderResultSet::getProviderRow(const EntityId entityId, const int32_t periodId, const ScopeNameValueMap* rowScopesMap) const
{
    RowScopesVector rowScopesVector;
    if (rowScopesMap != nullptr)
    {
        ScopeNameValueMap::const_iterator itr = rowScopesMap->begin();
        ScopeNameValueMap::const_iterator end = rowScopesMap->end();
        rowScopesVector.reserve(rowScopesMap->size());
        for (; itr != end; ++itr)
        {
            ScopeNameIndex sni;
            sni.mScopeName = itr->first;
            sni.mScopeValue = itr->second;
            rowScopesVector.push_back(sni);
        }
    }

    return getProviderRow(entityId, periodId, &rowScopesVector);
}

StatsProviderRowPtr StatsCacheProviderResultSet::getProviderRow(const ScopeNameValueMap* rowScopesMap) const
{
    ERR_LOG("[StatsCacheProviderResultSet].getProviderRow: unsupported");
    return StatsProviderRowPtr();
}

bool StatsCacheProviderResultSet::hasNextRow() const
{
    return (mRowIterator != mCacheRowList.end());
}

StatsProviderRowPtr StatsCacheProviderResultSet::getNextRow()
{
    return StatsProviderRowPtr(BLAZE_NEW StatsCacheProviderRow(*this, **(mRowIterator++)));
}

void StatsCacheProviderResultSet::reset()
{
    mRowIterator = mCacheRowList.begin();
}

const size_t StatsCacheProviderResultSet::size() const
{
    return mCacheRowList.size();
}

void StatsCacheProviderResultSet::addCacheRow(EntityId entityId, const CacheRow& cacheRow)
{
    // own a copy since the cached one may get uncached
    // (and since we're copying anyway, use a more efficient lookup structure)
    StatsCacheCellMap* cellMap = BLAZE_NEW StatsCacheCellMap();
    StatsCacheCell& entityIdCell = (*cellMap)[ENTITY_ID_TAG.c_str()];
    entityIdCell.type = STAT_TYPE_INT;
    entityIdCell.cell.intData = entityId;

    StatList::const_iterator statItr = cacheRow.mCategory.getStats().begin();
    StatList::const_iterator statEnd = cacheRow.mCategory.getStats().end();
    for (int32_t i = 0; statItr != statEnd; ++statItr, ++i)
    {
        const Stat* stat = *statItr;

        StatsCacheCell& statCell = (*cellMap)[stat->getName()];
        statCell.type = stat->getDbType().getType();
        statCell.cell.intData = 0;

        switch (statCell.type)
        {
        case STAT_TYPE_INT:
            statCell.cell.intData = cacheRow.mCacheRow[i].intData;
            break;
        case STAT_TYPE_FLOAT:
            statCell.cell.floatData = cacheRow.mCacheRow[i].floatData;
            break;
        case STAT_TYPE_STRING:
            statCell.cell.stringData = blaze_strdup(cacheRow.mCacheRow[i].stringData);
            break;
        default:
            break;
        }
    }

    mCacheRowList.push_back(cellMap);
}

StatsCacheProvider::StatsCacheProvider(StatsSlaveImpl& statsSlave, StatsCache &statsCache)
    : DbStatsProvider(statsSlave),
    mStatsCache(statsCache),
    mTotalCacheRowHits(0),
    mTotalDbRowHits(0)
{
}

StatsCacheProvider::~StatsCacheProvider()
{
    mStatsCache.mCacheRowFetchCount.increment(mTotalCacheRowHits);
    mStatsCache.mDbRowFetchCount.increment(mTotalDbRowHits);
}

BlazeRpcError StatsCacheProvider::executeFetchRequest(
    const StatCategory& category,
    const EntityIdList& entityIds,
    int32_t periodType,
    int32_t periodId,
    int32_t periodCount,
    const ScopeNameValueMap& scopeNameValueMap)
{
    BlazeRpcError err;
    StatsCacheProviderResultSet* cacheResultSet = nullptr;
    bool dbOnly = isWildcardFetch(category, entityIds, periodType, periodId, periodCount, scopeNameValueMap);
    if (dbOnly)
    {
        // just pass the whole request to the DbStatsProvider...
        err = DbStatsProvider::executeFetchRequest(category, entityIds, periodType, periodId, periodCount, scopeNameValueMap);
    }
    else
    {
        err = fetchStats(category, entityIds, periodType, periodId, scopeNameValueMap, cacheResultSet);
    }

    if (err != ERR_OK)
    {
        delete cacheResultSet;
        return err;
    }

    updateCache(category, periodType, !dbOnly);

    // add any cached rows found earlier to the results
    if (cacheResultSet != nullptr)
    {
        mResultSetVector.push_back(cacheResultSet);
    }

    mResultSetIterator = mResultSetVector.begin();

    return ERR_OK;
}

BlazeRpcError StatsCacheProvider::executeFetchRequest(
    const StatCategory& category,
    const EntityIdList& entityIds,
    int32_t periodType,
    int32_t periodId,
    int32_t periodCount,
    const ScopeNameValueListMap& scopeNameValueListMap)
{
    BlazeRpcError err;
    StatsCacheProviderResultSet* cacheResultSet = nullptr;
    bool dbOnly = isWildcardFetch(category, entityIds, periodType, periodId, periodCount, scopeNameValueListMap);
    if (dbOnly)
    {
        // just pass the whole request to the DbStatsProvider...
        err = DbStatsProvider::executeFetchRequest(category, entityIds, periodType, periodId, periodCount, scopeNameValueListMap);
    }
    else
    {
        // must have a specific keyscope, so convert ScopeNameValueListMap to ScopeNameValueMap
        // and call helper...
        ScopeNameValueMap scopeNameValueMap;

        ScopeNameValueListMap::const_iterator scopeItr = scopeNameValueListMap.begin();
        ScopeNameValueListMap::const_iterator scopeEnd = scopeNameValueListMap.end();
        for (; scopeItr != scopeEnd; ++scopeItr)
        {
            scopeNameValueMap.insert(eastl::make_pair(scopeItr->first, (scopeItr->second->getKeyScopeValues().begin())->first));
        }

        err = fetchStats(category, entityIds, periodType, periodId, scopeNameValueMap, cacheResultSet);
    }

    if (err != ERR_OK)
    {
        delete cacheResultSet;
        return err;
    }

    updateCache(category, periodType, !dbOnly);

    // add any cached rows found earlier to the results
    if (cacheResultSet != nullptr)
    {
        mResultSetVector.push_back(cacheResultSet);
    }

    mResultSetIterator = mResultSetVector.begin();

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief fetchStats

    Helper to get specific period and keyscope stats from the cache or DB.  Any cached stats
    retrieved will be included as a result set (out parameter).

    \param[in]  category - category info for the stats to be retrieved
    \param[in]  entityIds - entity IDs for the stats to be retrieved
    \param[in]  periodType - period type for the stats to be retrieved
    \param[in]  periodId - period ID for the stats to be retrieved (if any)
    \param[in]  scopeNameValueMap - specific keyscope combination for the stats to be retrieved
    \param[out] cacheResultSet - provider result set of cache rows fetched

    \return - Blaze error code
*/
/*************************************************************************************************/
BlazeRpcError StatsCacheProvider::fetchStats(
    const StatCategory& category,
    const EntityIdList& entityIds,
    int32_t periodType,
    int32_t periodId,
    const ScopeNameValueMap& scopeNameValueMap,
    StatsCacheProviderResultSet*& cacheResultSet)
{
    BlazeRpcError err = ERR_OK;
    EntityIdList notCachedIds;
    EntityIdSet entityIdSet;

    // check for each entity in the stats cache
    EntityIdList::const_iterator entityItr = entityIds.begin();
    EntityIdList::const_iterator entityEnd = entityIds.end();
    for (; entityItr != entityEnd; ++entityItr)
    {
        EntityId entityId = *entityItr;

        // skip dupe fetches
        if (entityIdSet.find(entityId) != entityIdSet.end())
            continue;
        entityIdSet.insert(entityId);

        const CacheRow* cacheRow = mStatsCache.getRow(category, entityId, periodType, periodId, &scopeNameValueMap);
        if (cacheRow != nullptr)
        {
            // collect the cache results ...
            if (cacheResultSet == nullptr)
            {
                cacheResultSet = BLAZE_NEW StatsCacheProviderResultSet(category, periodId, scopeNameValueMap, *this);
            }
            cacheResultSet->addCacheRow(entityId, *cacheRow);
            ++mTotalCacheRowHits;
        }
        else
        {
            notCachedIds.push_back(entityId);
        }
    }

    // go to the DB for the rest...
    if (!notCachedIds.empty())
    {
        err = DbStatsProvider::executeFetchRequest(category, notCachedIds, periodType, periodId, 1, scopeNameValueMap);
    }

    return err;
}

/*************************************************************************************************/
/*!
    \brief updateCache

    Helper to update the stats cache with any DB results (from requests made before this method).

    \param[in]  category - category info for the stats retrieved
    \param[in]  periodType - period type for the stats retrieved
    \param[in]  updateDbMetric - whether to update the metric for DB row fetch count

    \return - none
*/
/*************************************************************************************************/
void StatsCacheProvider::updateCache(
    const StatCategory& category,
    int32_t periodType,
    bool updateDbMetric)
{
    // update the cache with any DB results
    while (hasNextResultSet())
    {
        StatsProviderResultSet* resultSet = getNextResultSet();

        if (resultSet != nullptr && resultSet->size() > 0)
        {
            TRACE_LOG("[StatsCacheProvider].updateCache: found <" << resultSet->size()
                << "> rows in this result");
            ScopeNameValueMap tempScopeMap;

            if (updateDbMetric)
            {
                mTotalDbRowHits += resultSet->size();
            }

            resultSet->reset();
            while (resultSet->hasNextRow())
            {
                StatsProviderRowPtr row = resultSet->getNextRow();

                // build the keyscope for this row
                tempScopeMap.clear();
                if (category.hasScope())
                {
                    ScopeNameSet::const_iterator snItr = category.getScopeNameSet()->begin();
                    ScopeNameSet::const_iterator snEnd = category.getScopeNameSet()->end();
                    for (; snItr != snEnd; ++snItr)
                    {
                        ScopeValue scopeValue = row->getValueInt64((*snItr).c_str());
                        tempScopeMap.insert(eastl::make_pair(*snItr, scopeValue));
                    }
                }

                int32_t periodId = 0;
                if (periodType != STAT_PERIOD_ALL_TIME)
                {
                    periodId = row->getPeriodId();
                }
                mStatsCache.insertRow(category, row->getEntityId(), periodType, periodId, &tempScopeMap, row);
            }
        }
    }
}

bool StatsCacheProvider::isWildcardFetch(
    const StatCategory& category,
    const EntityIdList& entityIds,
    int32_t periodType,
    int32_t periodId,
    int32_t periodCount,
    const ScopeNameValueMap& scopeNameValueMap) const
{
    if (entityIds.empty())
        return true;

    if (periodType != STAT_PERIOD_ALL_TIME && periodCount > 1)
        return true;

    if (category.hasScope())
    {
        // assuming that any scope values have already been validated, we're only checking for wildcard
        ScopeNameSet::const_iterator snItr = category.getScopeNameSet()->begin();
        ScopeNameSet::const_iterator snEnd = category.getScopeNameSet()->end();
        for (; snItr != snEnd; ++snItr)
        {
            ScopeNameValueMap::const_iterator scopeItr = scopeNameValueMap.find((*snItr).c_str());
            if (scopeItr == scopeNameValueMap.end())
                return true; // equiv to KEY_SCOPE_VALUE_ALL

            if (scopeItr->second == KEY_SCOPE_VALUE_ALL)
                return true;
        }
    }

    return false;
}

bool StatsCacheProvider::isWildcardFetch(
    const StatCategory& category,
    const EntityIdList& entityIds,
    int32_t periodType,
    int32_t periodId,
    int32_t periodCount,
    const ScopeNameValueListMap& scopeNameValueListMap) const
{
    if (entityIds.empty())
        return true;

    if (periodType != STAT_PERIOD_ALL_TIME && periodCount > 1)
        return true;

    if (category.hasScope())
    {
        // assuming that any scope values have already been validated, we're only checking for wildcard
        ScopeNameSet::const_iterator snItr = category.getScopeNameSet()->begin();
        ScopeNameSet::const_iterator snEnd = category.getScopeNameSet()->end();
        for (; snItr != snEnd; ++snItr)
        {
            ScopeNameValueListMap::const_iterator scopeItr = scopeNameValueListMap.find(*snItr);
            if (scopeItr == scopeNameValueListMap.end())
                return true; // equiv to KEY_SCOPE_VALUE_ALL

            const ScopeValues* scopeValues = scopeItr->second;
            if (scopeValues->getKeyScopeValues().size() != 1)
                return true;

            ScopeStartEndValuesMap::const_iterator valuesItr = scopeValues->getKeyScopeValues().begin();
            if (valuesItr->first != valuesItr->second)
                return true;
        }
    }

    return false;
}

} // Stats
} // Blaze
