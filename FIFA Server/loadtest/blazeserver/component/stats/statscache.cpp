/*************************************************************************************************/
/*!
    \file   statscache.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "EAStdC/EAHashCRC.h"
#include "framework/blaze.h"
#include "statscache.h"
#include "statsslaveimpl.h"
#include "leaderboardindex.h"
#include "stats/tdf/stats_server.h"

namespace Blaze
{
namespace Stats
{

CacheKey::CacheKey(uint32_t catId, EntityId entityId, int32_t periodType, int32_t periodId, const char8_t* keyScopeString, uint64_t hashValue)
    :mCatId(catId), mEntityId(entityId), mPeriodType(periodType), mPeriodId(periodId), mHashValue(hashValue)
{
    blaze_strnzcpy(mScopes, keyScopeString, sizeof(mScopes));
}

CacheKey::CacheKey(uint32_t catId, EntityId entityId, int32_t periodType, int32_t periodId, const ScopeNameValueMap* scopeMap, uint64_t hashValue /* = 0 */)
    :mCatId(catId), mEntityId(entityId), mPeriodType(periodType), mPeriodId(periodId), mHashValue(hashValue)
{
    LeaderboardIndexes::buildKeyScopeString(mScopes, sizeof(mScopes), *scopeMap);
}

/*************************************************************************************************/
/*!
    \brief constructor
*/
/*************************************************************************************************/
CacheRow::CacheRow(const StatCategory& category, const StatsProviderRowPtr row) : mCategory(category)
{
    size_t count = mCategory.getStats().size();
    mCacheRow = BLAZE_NEW_ARRAY_STATS_CACHE(CacheCell, count);

    // transfer data from provider row into cache row using data type definitions from the category
    StatList::const_iterator statItr = mCategory.getStats().begin();
    StatList::const_iterator statEnd = mCategory.getStats().end();
    for (int32_t index = 0; statItr != statEnd; ++statItr, ++index)
    {
        const Stat* stat = *statItr;
        const char8_t* statName = stat->getName();
        CacheCell cell;
        cell.intData = 0;

        switch (stat->getDbType().getType())
        {
        case STAT_TYPE_INT:
            cell.intData = row->getValueInt64(statName);
            break;
        case STAT_TYPE_FLOAT:
            cell.floatData = row->getValueFloat(statName);
            break;
        case STAT_TYPE_STRING:
            cell.stringData = blaze_strdup(row->getValueString(statName), MEM_GROUP_STATS_CACHE);
            break;
        default:
            break;
        }
        mCacheRow[index] = cell;
    }
}

/*************************************************************************************************/
/*!
    \brief destructor
*/
/*************************************************************************************************/
CacheRow::~CacheRow()
{
    StatList::const_iterator statItr = mCategory.getStats().begin();
    StatList::const_iterator statEnd = mCategory.getStats().end();
    for (int32_t i = 0; statItr != statEnd; ++statItr, ++i)
    {
        const Stat* stat = *statItr;

        if (stat->getDbType().getType() == STAT_TYPE_STRING)
        {
            if (mCacheRow[i].stringData != nullptr)
            {
                BLAZE_FREE(mCacheRow[i].stringData);
            }
        }
    }

    delete[] mCacheRow;
}

/*************************************************************************************************/
/*!
    \brief constructor
*/
/*************************************************************************************************/
StatsCache::StatsCache(StatsSlaveImpl& component, size_t size)
    : mComponent(component),
    mLru(nullptr),
    mCacheRowFetchCount(component.getMetricsCollection(), "cacheRowsFetched"),
    mDbRowFetchCount(component.getMetricsCollection(), "dbRowsFetched")
{
    if (size > 0)
    {
        mLru = BLAZE_NEW LruCacheRow(size, BlazeStlAllocator("StatsCache::mLru", StatsSlave::COMPONENT_MEMORY_GROUP), LruCacheRow::RemoveCb(&StatsCache::lruRemoveCb));
    }
}

StatsCache::~StatsCache()
{
    if (mLru != nullptr)
    {
        delete mLru;
        mLru = nullptr;
    }
}

/*************************************************************************************************/
/*!
    \brief getRow

    Retrieves row from cache.  Parameters MUST have been validated before this call.

    \param[in]  cat - category info for the row to be retrieved
    \param[in]  entityId - entity ID for the row to be retrieved
    \param[in]  periodType - period type for the row to be retrieved
    \param[in]  periodId - period ID for the row to be retrieved (if any)
    \param[in]  scopes - specific keyscope combination for the row to be retrieved (if any)

    \return - pointer to the cached row
*/
/*************************************************************************************************/
const CacheRow* StatsCache::getRow(
    const StatCategory& cat,
    EntityId entityId,
    int32_t periodType,
    int32_t periodId,
    const ScopeNameValueMap* scopes)
{
    if (EA_LIKELY(mLru != nullptr))
    {
        CacheKey key(cat.getId(), entityId, periodType, periodId, scopes);

        CacheRow* row = nullptr;
        mLru->get(key, row);
        return row;
    }

    return nullptr;
}

/*************************************************************************************************/
/*!
    \brief reset

    Empty/clear the cache.
*/
/*************************************************************************************************/
void StatsCache::reset()
{
    if (EA_LIKELY(mLru != nullptr))
    {
        mLru->reset();
    }
}

/*************************************************************************************************/
/*!
    \brief resize

    Empty/clear the cache and resize it.
*/
/*************************************************************************************************/
void StatsCache::resetAndResize(size_t newSize)
{
    if (EA_UNLIKELY(mLru == nullptr) && newSize > 0)
    {
        mLru = BLAZE_NEW LruCacheRow(newSize, BlazeStlAllocator("StatsCache::mLru", StatsSlave::COMPONENT_MEMORY_GROUP), LruCacheRow::RemoveCb(&StatsCache::lruRemoveCb));
    }
    else
    {
        if (newSize == 0)
        {
            delete mLru;
            mLru = nullptr;
        }
        else if (mLru->getMaxSize() != newSize)
        {
            mLru->reset();
            mLru->setMaxSize(newSize);
        }
    }
}

/*************************************************************************************************/
/*!
    \brief insertRow

    Inserts row into cache.  Parameters MUST have been validated before this call.

    \param[in]  cat - category info for the row to be inserted
    \param[in]  entityId - entity ID for the row to be inserted
    \param[in]  periodType - period type for the row to be inserted
    \param[in]  periodId - period ID for the row to be inserted (if any)
    \param[in]  scopes - specific keyscope combination for the row to be inserted (if any)
    \param[in]  row - row to be inserted
*/
/*************************************************************************************************/
void StatsCache::insertRow(
        const StatCategory& cat,
        EntityId entityId,
        int32_t periodType,
        int32_t periodId,
        const ScopeNameValueMap* scopes,
        const StatsProviderRowPtr row)
{
    if (EA_UNLIKELY(row == nullptr) || EA_UNLIKELY(mLru == nullptr))
    {
        return;
    }

    CacheKey key(cat.getId(), entityId, periodType, periodId, scopes);

    CacheRow* cacheRow = BLAZE_NEW_STATS_CACHE CacheRow(cat, row);
    // add/replace existing row
    mLru->add(key, cacheRow);
}

/*************************************************************************************************/
/*!
    \brief deleteRow

    Deletes row from cache.  Parameters MUST have been validated before this call.

    \param[in]  catId - (internal) category ID for the row to be deleted
    \param[in]  entityId - entity ID for the row to be deleted
    \param[in]  periodType - period type for the row to be deleted
    \param[in]  periodId - period ID for the row to be deleted (if any)
    \param[in]  scopes - specific keyscope combination for the row to be deleted (if any)
*/
/*************************************************************************************************/
void StatsCache::deleteRow(
    uint32_t catId,
    EntityId entityId,
    int32_t periodType,
    int32_t periodId,
    const ScopeNameValueMap* scopes,
    uint64_t hashValue)
{
    if (EA_LIKELY(mLru != nullptr))
    {
        CacheKey key(catId, entityId, periodType, periodId, scopes, hashValue);
        mLru->remove(key);
    }
}

/*************************************************************************************************/
/*!
    \brief deleteRows

    Deletes rows from cache -- as per update broadcast

    \param[in]  data - rows to be deleted
*/
/*************************************************************************************************/
void StatsCache::deleteRows(const CacheRowUpdateList& data)
{
    if (EA_LIKELY(mLru != nullptr))
    {
        CacheRowUpdateList::const_iterator i = data.begin();
        CacheRowUpdateList::const_iterator e = data.end();
        for (; i != e; ++i)
        {
            const CacheRowUpdate* row = *i;

            const uint8_t* bytes = row->getBinaryData().getData();
            CacheKey key(*((const uint32_t*)(bytes)), *((const EntityId*)(bytes+4)), *((const int32_t*)(bytes+12)), *((const int32_t*)(bytes+16)),
                row->getKeyScopeString(), *((const uint64_t*)(bytes+20)));
            mLru->remove(key);
        }
    }
}

/*************************************************************************************************/
/*!
    \brief deleteRows

    Deletes rows from cache -- as per delete broadcast

    \param[in]  data - rows to be deleted
    \param[in]  currentPeriodIdMap - map of current period ids for each period type
*/
/*************************************************************************************************/
void StatsCache::deleteRows(const CacheRowDeleteList& data, const Stats::PeriodIdMap& currentPeriodIdMap)
{
    if (EA_LIKELY(mLru != nullptr))
    {
        CacheRowDeleteList::const_iterator i = data.begin();
        CacheRowDeleteList::const_iterator e = data.end();
        for (; i != e; ++i)
        {
            const CacheRowDelete* row = *i;

            // Iterate through period types
            for (int32_t periodType = 0; periodType < STAT_NUM_PERIODS; ++periodType)
            {
                if (!row->getPeriodTypes().empty())
                {
                    PeriodTypeList::const_iterator pi = row->getPeriodTypes().begin();
                    PeriodTypeList::const_iterator pe = row->getPeriodTypes().end();
                    for (; pi != pe; ++pi)
                    {
                        if (*pi == periodType)
                        {
                            // this period type requested
                            break;
                        }
                    }
                    if (pi == pe)
                    {
                        // this period type not requested; skip it
                        continue;
                    }
                }
                int32_t currentPeriodId;
                PeriodIdMap::const_iterator cpi = currentPeriodIdMap.find(periodType);
                if (cpi != currentPeriodIdMap.end())
                {
                    currentPeriodId = cpi->second;
                }
                else
                {
                    ERR_LOG("[StatsCache].deleteRows: missing current period id; using default for\n" << row);
                    currentPeriodId = mComponent.getPeriodId(periodType);
                }

                if (periodType == STAT_PERIOD_ALL_TIME || row->getPerOffsets().empty())
                {
                    // delete all periods
                    int32_t periodId = currentPeriodId - mComponent.getRetention(periodType);
                    for (; periodId <= currentPeriodId; ++periodId)
                    {
                        deleteRow(row->getCategoryId(), row->getEntityId(), periodType, periodId,
                            &row->getKeyScopeNameValueMap());
                    }
                }
                else
                {
                    // delete specific periods
                    OffsetList::const_iterator oi = row->getPerOffsets().begin();
                    OffsetList::const_iterator oe = row->getPerOffsets().end();
                    for (; oi != oe; ++oi)
                    {
                        int32_t periodId = currentPeriodId - static_cast<int32_t>(*oi);

                        deleteRow(row->getCategoryId(), row->getEntityId(), periodType, periodId,
                            &row->getKeyScopeNameValueMap());
                    }
                }
            } // for each period type
        } // for each data row
    }
}

void StatsCache::lruRemoveCb(const CacheKey& key, CacheRow* row)
{
    delete row;
}

uint64_t StatsCache::CacheKeyHash::operator()(const CacheKey& key) const
{
    return (key.mHashValue == 0) ? buildHash(key.mCatId, key.mEntityId, key.mPeriodType, key.mPeriodId, key.mScopes) : key.mHashValue;
}

bool StatsCache::CacheKeyEqualTo::operator()(const CacheKey& key1, const CacheKey& key2) const
{
    if (key1.mEntityId != key2.mEntityId)
        return false;

    if (key1.mCatId != key2.mCatId)
        return false;
    if (key1.mPeriodType != key2.mPeriodType)
        return false;
    if (key1.mPeriodId != key2.mPeriodId)
        return false;

    if (blaze_strcmp(key1.mScopes, key2.mScopes) != 0)
        return false;

    return true;
}

uint64_t StatsCache::buildHash(uint32_t catId,
                               EntityId entityId,
                               int32_t periodType,
                               int32_t periodId,
                               const char8_t* scopes)
{
    // use CRC to generate non-colliding hash values
    uint64_t crc = EA::StdC::kCRC64InitialValue;

    // use single call to hash members in cachKey
    crc = EA::StdC::CRC64(&catId, sizeof(catId), crc, false);
    crc = EA::StdC::CRC64(&entityId, sizeof(entityId), crc, false); 
    crc = EA::StdC::CRC64(&periodType, sizeof(periodType), crc, false); 
    crc = EA::StdC::CRC64(&periodId, 4, crc, false);
    crc = EA::StdC::CRC64(scopes, strlen(scopes), crc, true);

    return crc;
}

uint64_t StatsCache::computeCacheRowUpdateHashKey(const CacheRowUpdate& row)
{
    const uint8_t* bytes = row.getBinaryData().getData();
    return buildHash(*((const uint32_t*)(bytes)), *((const EntityId*)(bytes+4)), *((const int32_t*)(bytes+12)), *((const int32_t*)(bytes+16)), row.getKeyScopeString());
}

} // Stats
} // Blaze
