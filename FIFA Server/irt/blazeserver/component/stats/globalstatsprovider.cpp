/*************************************************************************************************/
/*!
    \file   globalstatsprovider.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbconn.h"

#include "globalstatsprovider.h"
#include "statsconfig.h"
#include "statsslaveimpl.h"

namespace Blaze
{
namespace Stats
{

int64_t GlobalStatsProviderRow::getValueInt64(const char8_t* column) const
{
    return mRow.getCell(column).getInt64(); 
}

int32_t GlobalStatsProviderRow::getValueInt(const char8_t* column) const
{
    // 32-bit integers are stored as 64bit in global chache provider
    return (int32_t)mRow.getCell(column).getInt64(); 
}

float_t GlobalStatsProviderRow::getValueFloat(const char8_t* column) const
{
    return mRow.getCell(column).getFloat(); 
}

const char8_t* GlobalStatsProviderRow::getValueString(const char8_t* column) const
{
    return mRow.getCell(column).getString(); 
}

EntityId GlobalStatsProviderRow::getEntityId() const
{
    return mRow.getEntityId();
}

int32_t GlobalStatsProviderRow::getPeriodId() const
{
    return mRow.getPeriodId();
}

StatsProviderRowPtr GlobalStatsProviderResultSet::getProviderRow(const EntityId entityId, const int32_t periodId, const RowScopesVector* rowScopesVector) const
{
    GlobalCacheScopeVector scopeVector;
    if (rowScopesVector != nullptr && !rowScopesVector->empty())
    {
        scopeVector.resize(rowScopesVector->size());
        for (RowScopesVector::const_iterator it = rowScopesVector->begin();
             it != rowScopesVector->end();
             it++)
        {
            scopeVector[mCategoryTable.getScopeIndex(it->mScopeName)] = it->mScopeValue;
        }
    }

    GlobalCacheRowKey key(entityId, periodId, scopeVector);
    
    GlobalCacheCategoryTable::CategoryTable::const_iterator ctIt = mResultRows.find(key);

    if (ctIt == mResultRows.end())
        return StatsProviderRowPtr();

    return StatsProviderRowPtr(BLAZE_NEW GlobalStatsProviderRow(*this, *(ctIt->second)));
}

StatsProviderRowPtr GlobalStatsProviderResultSet::getProviderRow(const EntityId entityId, const int32_t periodId, const ScopeNameValueMap* rowScopesMap) const
{
    RowScopesVector rowScopesVector;
    if (rowScopesMap != nullptr)
    {
        rowScopesVector.reserve(rowScopesMap->size());
        for (ScopeNameValueMap::const_iterator it = rowScopesMap->begin(); 
             it != rowScopesMap->end();
             it++)
        {
            ScopeNameIndex sni;
            sni.mScopeName = it->first;
            sni.mScopeValue = it->second;
            rowScopesVector.push_back(sni);
        }
    }

    return getProviderRow(entityId, periodId, &rowScopesVector);
}

StatsProviderRowPtr GlobalStatsProviderResultSet::getProviderRow(const ScopeNameValueMap* rowScopesMap) const
{
    if (mResultRows.size() == 0)
        return StatsProviderRowPtr();

    if (rowScopesMap == nullptr)
        return StatsProviderRowPtr(BLAZE_NEW GlobalStatsProviderRow(*this, *(mResultRows.begin()->second)));

    for (GlobalCacheCategoryTable::CategoryTable::const_iterator it = mResultRows.begin();
         it != mResultRows.end();
         it++)
    {
        const GlobalCacheRow &row = *(it->second);
        const GlobalCacheScopeVector& scopeVector = row.getScopeVector();

        bool matchFound = true;
        for (ScopeNameValueMap::const_iterator sit = rowScopesMap->begin();
             sit != rowScopesMap->end();
             sit++)
        {
            size_t index = mCategoryTable.getScopeIndex(sit->first);

            if (index == GlobalCacheCategoryTable::ILLEGAL_INDEX)
            {
                ERR_LOG("[GlobalStatsProviderResultSet].getProviderRow: Failed to find scope "
                    << sit->first); 
                return StatsProviderRowPtr();
            }

            if (scopeVector[index] != sit->second)
            {
                matchFound = false;
                break;
            }
        }

        if (matchFound)
            return StatsProviderRowPtr(BLAZE_NEW GlobalStatsProviderRow(*this, row));
    }

    return StatsProviderRowPtr();
}

bool GlobalStatsProviderResultSet::hasNextRow() const
{
    return (mRowIterator != mResultRows.end());
}

StatsProviderRowPtr GlobalStatsProviderResultSet::getNextRow()
{
    return StatsProviderRowPtr(BLAZE_NEW GlobalStatsProviderRow(*this, *((mRowIterator++)->second)));
}

void GlobalStatsProviderResultSet::reset()
{
    mRowIterator = mResultRows.begin();
}

const size_t GlobalStatsProviderResultSet::size() const
{
    return mResultRows.size();
}

GlobalStatsProvider::GlobalStatsProvider(StatsSlaveImpl& statsSlave, GlobalStatsCache &globalCache) : 
StatsProvider(statsSlave), mGlobalCache(globalCache), mInternalState(Blaze::ERR_OK)
{
}

GlobalStatsProvider::~GlobalStatsProvider()
{
}

BlazeRpcError GlobalStatsProvider::addStatsFetchForUpdateRequest(
    const StatCategory& category,
    EntityId entityId,
    int32_t periodType,
    int32_t periodId,
    const ScopeNameValueSetMap &scopeNameValueSetMap)
{
    GlobalStatsProviderResultSet* resultSet = nullptr;
    fetchStats(category, entityId, periodType, periodId, scopeNameValueSetMap, resultSet);
    mResultSetVector.push_back(resultSet);
    mResultSetIterator = mResultSetVector.begin();
    return ERR_OK;
}

BlazeRpcError GlobalStatsProvider::addDefaultStatsInsertForUpdateRequest(
    const StatCategory& category,
    EntityId entityId,
    int32_t periodType,
    int32_t periodId,
    const RowScopesVector* rowScopes)
{
    // don't need to insert default stats because this layer for global stats doesn't deal with DB
    return ERR_OK;
}

void GlobalStatsProvider::fetchStats(
    const StatCategory& category,
    EntityId entityId,
    int32_t periodType,
    int32_t periodId,
    const ScopeNameValueSetMap &scopeNameValueSetMap,
    GlobalStatsProviderResultSet* &resultSet)
{
    GlobalCacheCategoryTable *table = mGlobalCache.getCategoryTable(category.getName(), periodType);

    if (table == nullptr)
    {
        ERR_LOG("[GlobalStatsProvider].fetchStats: Failed to fetch table for category " << category.getName());
        mInternalState = Blaze::ERR_SYSTEM;
        return;
    }

    if (resultSet == nullptr)
        resultSet = BLAZE_NEW GlobalStatsProviderResultSet(category.getName(), *this, *table);

    // scan the table and pick all the rows that satisfy the conditions
    table->reset();
    while (table->hasNextRow())
    {
        GlobalCacheRow *row = table->getNextRow();
        
        // check entity id and period id
        if (row->getEntityId() == entityId && row->getPeriodId() == periodId)
        {
            // check scopes, if no scopes then add row
            GlobalCacheScopeVector scopes = row->getScopeVector();
            if (scopes.size() == 0 || scopeNameValueSetMap.size() == 0)
            {
                resultSet->mResultRows.insert(
                    eastl::make_pair(GlobalCacheRowKey(row->getEntityId(), row->getPeriodId(), row->getScopeVector())
                    , row));
            }
            else
            {
                bool scopesMatch = true;
                // scan the whole value set for each scopes and compare
                // at the fisrt dissagreement bail out
                for (ScopeNameValueSetMap::const_iterator snvit = scopeNameValueSetMap.begin();
                     snvit != scopeNameValueSetMap.end() && scopesMatch;
                     snvit++)
                {
                    const char8_t *scopeName = snvit->first;
                    size_t index = table->getScopeIndex(scopeName);
                    if (index == GlobalCacheCategoryTable::ILLEGAL_INDEX)
                    {
                        ERR_LOG("[GlobalStatsProvider].fetchStats: Failed to find scope "
                            << scopeName << " in table for category " << category.getName()); 
                        mInternalState = Blaze::ERR_SYSTEM;
                        return;
                    }
                    ScopeValue scopeValue = scopes[index];

                    // is this scope value within requested set?
                    scopesMatch = false;
                    ScopeValueSet *valueSet = snvit->second;
                    for (ScopeValueSet::const_iterator vit = valueSet->begin();
                         vit != valueSet->end();
                         vit ++)
                    {
                        if (*vit == scopeValue)
                        {
                            scopesMatch = true;
                            break;
                        }
                    }
                }

                if (scopesMatch)
                {
                    resultSet->mResultRows.insert(
                        eastl::make_pair(GlobalCacheRowKey(row->getEntityId(), row->getPeriodId(), row->getScopeVector())
                        , row));
                }
            }
        }
    }
}

BlazeRpcError GlobalStatsProvider::addStatsUpdateRequest(
    int32_t periodId,
    const UpdateAggregateRowKey &aggKey,
    const StatValMap &statsValMap)
{
    CategoryMap::const_iterator iter =
        mComponent.getConfigData()->getCategoryMap()->find(aggKey.mCategory);

    if (iter == mComponent.getConfigData()->getCategoryMap()->end())
    {
        ERR_LOG("[GlobalStatsProvider].addStatsUpdateRequest: Failed to find category " 
            << aggKey.mCategory); 
        mInternalState = Blaze::ERR_SYSTEM;
        return ERR_SYSTEM;
    }

    const StatCategory &category = *(iter->second);

    StatPtrValMap valMap;
    const StatMap &statMap = *category.getStatsMap();

    for (StatValMap::const_iterator it = statsValMap.begin();
        it != statsValMap.end();
        it++)
    {
        StatVal *val = it->second;
        StatMap::const_iterator stit = statMap.find(it->first);
        if (stit == statMap.end())
        {
            ERR_LOG("[GlobalStatsProvider].addStatsUpdateRequest: Failed to find stat " 
                << it->first << " in table for category " << category.getName()); 
            mInternalState = Blaze::ERR_SYSTEM;
            return ERR_SYSTEM;
        }
        const Stat *stat = stit->second;
        valMap.insert(eastl::make_pair(stat, val));
    }
    
    return addStatsUpdateRequest(
        category, 
        aggKey.mEntityId, 
        aggKey.mPeriod, 
        periodId,
        &aggKey.mRowScopesVector, 
        valMap,
        aggKey.mStatus == AGG_INSERT);

}

BlazeRpcError GlobalStatsProvider::addStatsUpdateRequest(
    const StatCategory& cat,
    EntityId entityId,
    int32_t periodType,
    int32_t periodId,
    const RowScopesVector* rowScopes,
    const StatPtrValMap &valMap,
    bool newRow)
{
    GlobalCacheCategoryTable *table = mGlobalCache.getCategoryTable(cat.getName(), periodType);

    if (table == nullptr)
    {
        ERR_LOG("[GlobalStatsProvider].addStatsUpdateRequest: Failed to fetch table for category " 
            << cat.getName()); 
        mInternalState = Blaze::ERR_SYSTEM;
        return ERR_SYSTEM;
    }

    // create internal scope representation vector
    GlobalCacheScopeVector scopes;
    if (rowScopes != nullptr)
    {
        scopes.resize(rowScopes->size());

        for (RowScopesVector::const_iterator it = rowScopes->begin();
            it != rowScopes->end();
            it++)
        {
            size_t index = table->getScopeIndex(it->mScopeName);
            if (index == GlobalCacheCategoryTable::ILLEGAL_INDEX)
            {
                ERR_LOG("[GlobalStatsProvider].addStatsUpdateRequest: Cannot find scope " << it->mScopeName 
                    << " for category " << cat.getName()); 
                mInternalState = Blaze::ERR_SYSTEM;
                return ERR_SYSTEM;
            }

            scopes[index] = it->mScopeValue;
        }
    }

    GlobalCacheRow *globalCacheRow = nullptr;
    GlobalCacheRowKey key(entityId, periodId, scopes);

    if (newRow)
    {
        globalCacheRow = &table->addNewRow(key);
    }
    else
    {
        globalCacheRow = table->findRow(key);
    }

    if (globalCacheRow == nullptr)
    {
        ERR_LOG("[GlobalStatsProvider].addStatsUpdateRequest: Cannot find row to update " 
            << " for category " << cat.getName()); 
        mInternalState = Blaze::ERR_SYSTEM;
        return ERR_SYSTEM;
    }

    for (StatPtrValMap::const_iterator sit = valMap.begin();
         sit != valMap.end();
         sit++)
    {
        const Stat *stat = sit->first;
        const StatVal *statVal = sit->second;

        size_t index = table->getCellIndex(stat->getName());
        
        if (index == GlobalCacheCategoryTable::ILLEGAL_INDEX)
        {
            ERR_LOG("[GlobalStatsProvider].addStatsUpdateRequest: Cannot find stat " << stat->getName()
                << " in category " << cat.getName()); 
            mInternalState = Blaze::ERR_SYSTEM;
            return ERR_SYSTEM;
        }

        if (stat->getDbType().getType() == STAT_TYPE_INT)
        {
            GlobalCacheCell cell(statVal->intVal);
            globalCacheRow->setStatCell(index, cell);
        } else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
        {
            GlobalCacheCell cell(statVal->floatVal);
            globalCacheRow->setStatCell(index, cell);
        } else if (stat->getDbType().getType() == STAT_TYPE_STRING)
        {
            GlobalCacheCell cell(statVal->stringVal);
            globalCacheRow->setStatCell(index, cell);
        }
    }
    return ERR_OK;
}

BlazeRpcError GlobalStatsProvider::executeFetchRequest(
    const StatCategory& category,
    const EntityIdList &entityIds,
    int32_t periodType,
    int32_t periodId,
    int32_t periodCount,
    const ScopeNameValueMap &scopeNameValueMap)
{
    ScopeNameValueSetMapWrapper nameValueSetMap;

    // transform ScopeNameValueMap into ScopeNameValueSetMap
    // where each scope will have only one value in value set
    for (ScopeNameValueMap::const_iterator sit = scopeNameValueMap.begin();
         sit != scopeNameValueMap.end();
         sit++)
    {
        ScopeValueSet *valueSet = nullptr;
        if (sit->second == KEY_SCOPE_VALUE_AGGREGATE)
        {
            valueSet = BLAZE_NEW ScopeValueSet;
            valueSet->insert(mComponent.getConfigData()->getAggregateKeyValue(sit->first.c_str()));
            nameValueSetMap.insert(eastl::make_pair(sit->first, valueSet));
        }
        else if (sit->second != KEY_SCOPE_VALUE_ALL)
        {
            valueSet = BLAZE_NEW ScopeValueSet;
            valueSet->insert(sit->second);
            nameValueSetMap.insert(eastl::make_pair(sit->first, valueSet));
        }
    }

    GlobalStatsProviderResultSet* resultSet = nullptr;

    for (EntityIdList::const_iterator it = entityIds.begin();
         it != entityIds.end();
         it++)
    {
        fetchStats(category, *it, periodType, periodId, nameValueSetMap, resultSet); 
    }

    mResultSetVector.push_back(resultSet);
    mResultSetIterator = mResultSetVector.begin();

    return mInternalState;
}

BlazeRpcError GlobalStatsProvider::executeFetchRequest(
    const StatCategory& category,
    const EntityIdList &entityIds,
    int32_t periodType,
    int32_t periodId,
    int32_t periodCount,
    const ScopeNameValueListMap &scopeNameValueListMap)
{
    ScopeNameValueSetMapWrapper nameValueSetMap;

    // transform ScopeNameValueListMap into ScopeNameValueSetMap
    // where each scope can have more than one value in value set
    for (ScopeNameValueListMap::const_iterator sit = scopeNameValueListMap.begin();
         sit != scopeNameValueListMap.end();
         sit++)
    {
        ScopeValueSet *valueSet = BLAZE_NEW ScopeValueSet;
        const ScopeValues* scopeValues = sit->second;
        ScopeStartEndValuesMap::const_iterator valuesItr = scopeValues->getKeyScopeValues().begin();
        ScopeStartEndValuesMap::const_iterator valuesEnd = scopeValues->getKeyScopeValues().end();
        for (; valuesItr != valuesEnd; ++valuesItr)
        {
            ScopeValue scopeValue;
            for (scopeValue = valuesItr->first; scopeValue <= valuesItr->second; ++scopeValue)
            {
                valueSet->insert(scopeValue);
            }
        }
        nameValueSetMap.insert(eastl::make_pair(sit->first, valueSet));
    }

    GlobalStatsProviderResultSet* resultSet = nullptr;

    for (EntityIdList::const_iterator it = entityIds.begin();
         it != entityIds.end();
         it++)
    {
        fetchStats(category, *it, periodType, periodId, nameValueSetMap, resultSet);
    }

    mResultSetVector.push_back(resultSet);
    mResultSetIterator = mResultSetVector.begin();

    return mInternalState;
}

BlazeRpcError GlobalStatsProvider::executeFetchForUpdateRequest()
{
    // everything already done in addFetchForUpdateRequest
    return mInternalState;
}

BlazeRpcError GlobalStatsProvider::executeUpdateRequest()
{
    // everything already done in addUpdateRequest
    return mInternalState;
}

//
// Transaction Interface
// -------------------------
// Currently there is no need to support transactions in GlobalStatsProvider
// because global stats updates will be performed in-memory on each slave
// and only global stats OR non global stats updates are performed at the same time.
// We assume update can never fail because the request is always validated on slave
// before being distributed to other slaves.

BlazeRpcError GlobalStatsProvider::commit()
{
    // unsupported
    return Blaze::ERR_OK;
}

BlazeRpcError GlobalStatsProvider::rollback()
{
    // unsupported
    return Blaze::ERR_OK;
}

}// Stats
} // Blaze
