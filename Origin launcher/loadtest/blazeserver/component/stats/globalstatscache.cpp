/*************************************************************************************************/
/*!
    \file   globalstatscache.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/util/shared/stringbuilder.h"

#include "stats/tdf/stats_server.h"
#include "statsconfig.h"
#include "statsslaveimpl.h"
#include "globalstatscache.h"

namespace Blaze
{
namespace Stats
{

GlobalCacheCell::GlobalCacheCell(int64_t value)
{
    setInt64(value);
}

GlobalCacheCell::GlobalCacheCell(float value)
{
    setFloat(value);
}

GlobalCacheCell::GlobalCacheCell(const char8_t* str)
{
    setString(str);
}

int64_t GlobalCacheCell::getInt64() const
{
    if (mCellType != STAT_TYPE_INT)
    {
        ERR_LOG("[GlobalStatsCache::Cell].getInt64: Wrong cell type: " << mCellType); 
    }

    return mIntValue;
}

const eastl::string GlobalCacheCell::toString() const
{
    eastl::string result;

    if (mCellType == STAT_TYPE_FLOAT)
    {
        result.sprintf("%f", mFloatValue);
    } else if (mCellType == STAT_TYPE_INT)
    {
        result.sprintf("%" PRId64, mIntValue);
    } else if (mCellType == STAT_TYPE_STRING)
    {
        result = mStringValue;
    } else
    {
        EA_ASSERT_MSG(false, "Invalid cell type");
    }

    return result;
}

float GlobalCacheCell::getFloat() const
{
    if (mCellType != STAT_TYPE_FLOAT)
    {
        ERR_LOG("[GlobalStatsCache::Cell].getFloat: Wrong cell type: " << mCellType); 
    }

    return mFloatValue;
}

const char8_t* GlobalCacheCell::getString() const
{
    if (mCellType != STAT_TYPE_STRING)
    {
        ERR_LOG("[GlobalStatsCache::Cell].getString: Wrong cell type: " << mCellType); 
    }

    return mStringValue.c_str();
}

void GlobalCacheCell::setInt64(int64_t value)
{
    mCellType = STAT_TYPE_INT;
    mIntValue = value;
    mFloatValue = 0.0;
}

void GlobalCacheCell::setFloat(float value)
{
    mCellType = STAT_TYPE_FLOAT;
    mIntValue = 0;
    mFloatValue = value;
}

void GlobalCacheCell::setString(const char8_t* value)
{
    mCellType = STAT_TYPE_STRING;
    mIntValue = 0;
    mFloatValue = 0.0;
    mStringValue = value;
}

GlobalCacheRow::GlobalCacheRow(
    GlobalCacheRowState rowState, 
    GlobalCacheCategoryTable &table, 
    size_t cellCount,
    const GlobalCacheRowKey &key) 
    : mRowState(rowState), mTable(table), mKey(key) 
{ 
    mRow.reserve(cellCount); 

    // create cells for scopes
    const GlobalCacheScopeVector& scopes = mKey.getScopeVector();
    mScopes.reserve(scopes.size());
    for (GlobalCacheScopeVector::const_iterator it = scopes.begin();
         it != scopes.end();
         it++)
    {
        GlobalCacheCell cell((int64_t)*it);
        mScopes.push_back(cell);
    }
}

const GlobalCacheCell& GlobalCacheRow::getCell(const char8_t *name) const
{
    size_t i = mTable.getCellIndex(name);

    if (i == GlobalCacheCategoryTable::ILLEGAL_INDEX)
    {
        // is this scope?
        i = mTable.getScopeIndex(name);
        if (i != GlobalCacheCategoryTable::ILLEGAL_INDEX)
        {
            return mScopes[i];
        }
    }
    
    EA_ASSERT_MSG(i != GlobalCacheCategoryTable::ILLEGAL_INDEX, "Illegal cell name");
    
    return mRow[i];
}

const GlobalCacheCell& GlobalCacheRow::getStatCell(size_t index) const
{
    EA_ASSERT_MSG(index < getCellCount(), "Illegal cell index");

    return mRow[index];
}

void GlobalCacheRow::setStatCell(size_t index, const GlobalCacheCell& cell)
{
    EA_ASSERT_MSG(index < getCellCount(), "Illegal cell index");

    if (mRowState == GCRS_UNCHANGED)
        mRowState = GCRS_UPDATED;

    mRow[index] = cell;
}

void GlobalCacheRow::setStatCell(const char8_t *name, const GlobalCacheCell& cell)
{
    setStatCell(mTable.getCellIndex(name), cell);
}

bool GlobalCacheRowKey::operator<(const GlobalCacheRowKey& ref) const
{
    if (mEntityId != ref.mEntityId)
        return (mEntityId < ref.mEntityId);

    if (mPeriodId != ref.mPeriodId)
        return (mPeriodId < ref.mPeriodId);

    if (mScopeVector.size() != ref.mScopeVector.size())
        return  (mScopeVector.size() < ref.mScopeVector.size());

    GlobalCacheScopeVector::const_iterator it1, it2;

    for (it1 = mScopeVector.begin(), it2 = ref.mScopeVector.begin();
         it1 != mScopeVector.end() && it2 != ref.mScopeVector.end();
         it1++, it2++)
    {
        if (*it1 != *it2)
            return (*it1 < *it2);
    }

    // they are equal
    return false;
}

bool GlobalCacheRowKey::operator==(const GlobalCacheRowKey& ref) const
{
    if (mEntityId != ref.mEntityId)
        return false;

    if (mPeriodId != ref.mPeriodId)
        return false;

    // if one of the two has no scopes defined that means we don't care about scopes
    if (mScopeVector.size() == 0 || ref.mScopeVector.size() == 0)
        return true;

    if (mScopeVector.size() != ref.mScopeVector.size())
        return false;

    GlobalCacheScopeVector::const_iterator it1, it2;

    for (it1 = mScopeVector.begin(), it2 = ref.mScopeVector.begin();
         it1 != mScopeVector.end() && it2 != ref.mScopeVector.end();
         it1++, it2++)
    {
        if (*it1 != *it2)
            return false;
    }

    return true;
}

GlobalCacheRowKey& GlobalCacheRowKey::operator=(const GlobalCacheRowKey& ref)
{
    mEntityId = ref.mEntityId;
    mPeriodId = ref.mPeriodId;
    mScopeVector.clear();

    GlobalCacheScopeVector::const_iterator it;

    mScopeVector.reserve(ref.mScopeVector.size());
    for (it = ref.mScopeVector.begin();
         it != ref.mScopeVector.end();
         it++)
    {
        mScopeVector.push_back(*it);
    }

    return *this;
}

const eastl::string GlobalCacheRowKey::toString() const
{
    eastl::string str;
    str.sprintf("GlobalCacheRowKey(EntityId=%" PRId64 ", PeriodId=%d", mEntityId, mPeriodId);

    if (mScopeVector.size() > 0)
        str.append(" scopeVector=[ ");

    for (GlobalCacheScopeVector::const_iterator it = mScopeVector.begin();
         it != mScopeVector.end();
         it++)
    {
        eastl::string val;
        val.sprintf("%d ", *it);
        str.append(val);
    }
    if (mScopeVector.size() > 0)
        str.append("]");

    str.append(")");

    return str;
}


GlobalCacheCategoryTable::GlobalCacheCategoryTable(const StatCategory &category, const int32_t periodType)
    : mCategory(category), mPeriod(periodType)
{ 
    mRowIterator = mCategoryTable.end();

    // create name-to-index maps
    size_t index = 0; 

    const ScopeNameSet *scopes = mCategory.getScopeNameSet();
    if (scopes != nullptr)
    {
        for (ScopeList::const_iterator it = scopes->begin(); it != scopes->end(); it++)
            mNameToScopeIndexMap.insert(eastl::make_pair(it->c_str(), index++));
    }

    // set the scope size;
    mScopeSize = index;

    index = 0;
    const StatList &stats = mCategory.getStats();
    for (StatList::const_iterator it = stats.begin(); it != stats.end(); it++)
        mNameToCellIndexMap.insert(eastl::make_pair((*it)->getName(), index++));
    
    // set the row size;
    mRowSize = index;
}

GlobalCacheCategoryTable::~GlobalCacheCategoryTable()
{
    for (CategoryTable::const_iterator it = mCategoryTable.begin();
        it != mCategoryTable.end();
        it++)
    {
        delete it->second;
    }
}

size_t GlobalCacheCategoryTable::getCellIndex(const char8_t *name) const
{
    NameToIndexMap::const_iterator it = mNameToCellIndexMap.find(name);
    if (it == mNameToCellIndexMap.end())
    {
        return ILLEGAL_INDEX;
    }

    return it->second;
}

size_t GlobalCacheCategoryTable::getScopeIndex(const char8_t *name) const
{
    NameToIndexMap::const_iterator it = mNameToScopeIndexMap.find(name);
    if (it == mNameToScopeIndexMap.end())
    {
        ERR_LOG("[GlobalCacheCategoryTable].getScopeIndex: Unknown scope name: " << name); 
        return ILLEGAL_INDEX;
    }

    return it->second;
}

GlobalCacheRow& GlobalCacheCategoryTable::addNewRow(const GlobalCacheRowKey& key)
{
    return addRow(key, GCRS_INSERTED);
}

GlobalCacheRow& GlobalCacheCategoryTable::addRow(const GlobalCacheRowKey& key, GlobalCacheRowState rowState)
{
    // do we have this row already? if so just return referece to it
    TRACE_LOG("[GlobalCacheCategoryTable].addRow to "
        "table(name=" << mCategory.getName() << ", periodType=" << getPeriod() << ") "
        << key.toString().c_str());

    CategoryTable::iterator itr = mCategoryTable.find(key);
    if (itr != mCategoryTable.end())
        return *itr->second;

    EA_ASSERT_MSG((key.getScopeVector().size()) == mScopeSize, "Wrong scope vector element count for this category.");

    // create new row 
    GlobalCacheRow *row = BLAZE_NEW GlobalCacheRow(rowState, *this, mRowSize, key);

    const StatList &stats = mCategory.getStats();
    row->mRow.reserve(stats.size());
    for (StatList::const_iterator it = stats.begin(); it != stats.end(); it++)
    {
        StatType cellType = (*it)->getDbType().getType();
        if (cellType == STAT_TYPE_INT)
        {
            GlobalCacheCell cell((*it)->getDefaultIntVal());
            row->mRow.push_back(cell);
        }
        else if (cellType == STAT_TYPE_FLOAT)
        {
            GlobalCacheCell cell((*it)->getDefaultFloatVal());
            row->mRow.push_back(cell);
        }
        else if (cellType == STAT_TYPE_STRING)
        {
            GlobalCacheCell cell((*it)->getDefaultStringVal());
            row->mRow.push_back(cell);
        }
        else
        {
            EA_ASSERT_MSG(false, "Unkown category type.");
        }
    }

    eastl::pair<GlobalCacheRowKey, GlobalCacheRow*> pair(key, row);
    mCategoryTable.insert(pair);
    return *row;
}

void GlobalCacheCategoryTable::deleteRow(const GlobalCacheRowKey &key)
{
    // find the row
    CategoryTable::iterator it = mCategoryTable.find(key);
    if (it == mCategoryTable.end())
        return; // nothing to do, row is already gone
    
    GlobalCacheRow *row = it->second;
    mCategoryTable.erase(it);
    delete row;

    mDeletedKeys.push_back(key);
}

void GlobalCacheCategoryTable::reset()
{
    mRowIterator = mCategoryTable.begin();
}

bool GlobalCacheCategoryTable::hasNextRow()
{
    return mCategoryTable.end() != mRowIterator;
}

GlobalCacheRow* GlobalCacheCategoryTable::getNextRow()
{
    if (mRowIterator == mCategoryTable.end())
        return nullptr;
    GlobalCacheRow *row = mRowIterator->second;
    
    mRowIterator++;
    
    return row;
}

GlobalCacheRow* GlobalCacheCategoryTable::findRow(const GlobalCacheRowKey &key)
{
    CategoryTable::iterator rowIterator = mCategoryTable.find(key);

    if (rowIterator == mCategoryTable.end())
        return nullptr;
    
    return rowIterator->second;
}

GlobalCacheRow* GlobalCacheCategoryTable::findRow(const EntityId entityId, const int32_t periodId, const ScopeNameValueMap &scopeKey)
{
    EA_ASSERT_MSG(scopeKey.size() == getScopeSize(), "Wrong scope vector size.");
    
    GlobalCacheScopeVector scopeVector(getScopeSize());
    for (ScopeNameValueMap::const_iterator it = scopeKey.begin(); it != scopeKey.end(); it++)
    {
        size_t index = getScopeIndex(it->first);
        EA_ASSERT_MSG(index != ILLEGAL_INDEX, "Scope name not found!");

        scopeVector[index] = it->second;
    }

    GlobalCacheRowKey key(entityId, periodId, scopeVector);
    
    return findRow(key);
}



GlobalStatsCache::GlobalStatsCache(StatsSlaveImpl& statsSlave)
    : mComponent(statsSlave),
    mIsInitialized(false), mIsReady(false)
{
}

GlobalStatsCache::~GlobalStatsCache()
{
    for (GlobalCacheCategoriesByNameByPeriodMap::iterator itr = mCategoriesByNameByPeriodMap.begin();
         itr != mCategoriesByNameByPeriodMap.end();
         itr++)
    {
        GlobalCacheCategoriesByNameMap &map = itr->second;
        for (GlobalCacheCategoriesByNameMap::iterator it = map.begin();
             it != map.end();
             it++)
        {
            delete it->second;
            it->second = nullptr;
        }
    }
}

BlazeRpcError GlobalStatsCache::initialize()
{
    TRACE_LOG("[GlobalStatsCache].initialize: Building Global Stats Cache"); 

    BlazeRpcError error = Blaze::ERR_OK;

    // find global stats categories in config file 
    const StatsConfigData *configData = mComponent.getConfigData();

    // get database instance
    DbConnPtr dbConn = gDbScheduler->getReadConnPtr(mComponent.getDbId());
    if (dbConn == nullptr)
    {
        ERR_LOG("[GlobalStatsCache].initialize: Could not get database connection"); 
        return Blaze::ERR_SYSTEM;
    }

    const CategoryMap *categories = configData->getCategoryMap();
    CategoryMap::const_iterator it = categories->begin();
    for (; it != categories->end(); it++)
    {
        const StatCategory *category = it->second;
        if (category->getCategoryType() == CATEGORY_TYPE_GLOBAL)
        {
            // find all periods for this global stats category
            int32_t periodTypes = category->getPeriodTypes();
            for (int32_t periodType = 0; periodType < STAT_NUM_PERIODS; periodType++)
            {
                if ((1 << periodType) & periodTypes)
                {
                    GlobalCacheCategoriesByNameMap categoriesByNameMap;
                    GlobalCacheCategoriesByNameMap *targetMap = &categoriesByNameMap;
                    GlobalCacheCategoriesByNameByPeriodMap::iterator pit = mCategoriesByNameByPeriodMap.find(periodType);
                    
                    bool newPeriod = true;
                    if (pit != mCategoriesByNameByPeriodMap.end())
                    {
                        newPeriod = false;
                        targetMap = &pit->second;
                    }

                    GlobalCacheCategoryTable *table = BLAZE_NEW GlobalCacheCategoryTable(*category, periodType);

                    error = fetchCategoryTable(
                        dbConn, 
                        category->getName(),
                        periodType, 
                        *table);

                    if (error != Blaze::ERR_OK)
                    {
                        ERR_LOG("[GlobalStatsCache].initialize: Failed to fetch table for category " << category->getName()); 
                        return error;
                    }

                    targetMap->insert(eastl::make_pair(category->getName(), table));

                    if (newPeriod)
                    {
                        mCategoriesByNameByPeriodMap.insert(eastl::make_pair(periodType, *targetMap));
                    }
                }
            }
        }
    }

    mIsInitialized = true;
    mIsReady = true;

    // wake up fibers that issued read/write request while cache was being initialized
    signalBlockedFibers();
 
    TRACE_LOG("[GlobalStatsCache].initialize: Completed Building Global Stats Cache"); 

    return Blaze::ERR_OK;
}

void GlobalStatsCache::signalBlockedFibers()
{
   GlobalCacheEvents events;
   events.swap(mGlobalCacheEvents);
   mGlobalCacheEvents.clear();

   for (GlobalCacheEvents::iterator it = events.begin();
         it != events.end();
         it++)
    {
        Fiber::signal(*it, ERR_OK);
    }

    
}

GlobalCacheCategoryTable* GlobalStatsCache::getCategoryTable(
    const char8_t* categoryName, 
    const int32_t periodType)
{
    if (!mIsInitialized || !mIsReady)
    {
        // block until initialized or write cache operation is complete
        Fiber::EventHandle eventHandle = Fiber::getNextEventHandle();
        mGlobalCacheEvents.push_back(eventHandle);
        BlazeRpcError err = Fiber::wait(eventHandle, "GlobalStatsCache::getCategoryTable");

        if (err != Blaze::ERR_OK)
        {
            return nullptr;
        }
    }

    GlobalCacheCategoriesByNameByPeriodMap::iterator it = mCategoriesByNameByPeriodMap.find(periodType);
    if (it == mCategoriesByNameByPeriodMap.end())
        return nullptr;

    GlobalCacheCategoriesByNameMap::iterator byNameIt = it->second.find(categoryName);
    if (byNameIt == it->second.end())
        return nullptr;

    return byNameIt->second;
}

const StatCategory* GlobalStatsCache::findCategory(
    const char8_t* categoryName) const 
{
    // find category description in config
    const CategoryMap *map = mComponent.getConfigData()->getCategoryMap();
    CategoryMap::const_iterator it = map->find(categoryName);
    
    if (it == map->end())
    {
        ERR_LOG("[GlobalStatsCache].findCategory: unknown category: " << categoryName);
        return nullptr;
    }

    return it->second;
}

BlazeRpcError GlobalStatsCache::fetchCategoryTable(
    DbConnPtr dbConn, 
    const char8_t* categoryName,
    int32_t periodType, 
    GlobalCacheCategoryTable &category)
{
    TRACE_LOG("[GlobalStatsCache].fetchCategoryTable: fetching table for category " << categoryName);

    category.mCategoryTable.clear();

    // find category description in config
    const StatCategory *statCategory = findCategory(categoryName);

    if (statCategory == nullptr)
        return Blaze::ERR_SYSTEM;

    // create & execute query
    //
    // Note: we explicitly specify all column names in the query because we
    // want to match cell index in column with stat name (GlobalStatsCategoryTable
    // creates map of index-to-name).

    QueryPtr dbQuery = DB_NEW_QUERY_PTR(dbConn);
    dbQuery->append("SELECT `entity_id`, ");
  
    if (category.getPeriod() != STAT_PERIOD_ALL_TIME)
        dbQuery->append("`period_id`, ");

    // add scopes to query if any
    const ScopeNameSet *scopes = statCategory->getScopeNameSet();
    if (scopes != nullptr)
    {
        for (ScopeList::const_iterator it = scopes->begin(); it != scopes->end(); it++)
            dbQuery->append("`$s`, ", it->c_str());
    }

    // we don't need last_modified, so it's part of query!

    // add stats to query
    const StatList &stats = statCategory->getStats();
    for (StatList::const_iterator sit = stats.begin(); sit != stats.end(); sit++)
           dbQuery->append("`$s`,", (*sit)->getName());

    // remove trialing ','
    dbQuery->trim(1);
    
    dbQuery->append(" FROM `$s`", statCategory->getTableName(periodType));

    DbResultPtr dbRes;
    BlazeRpcError error = dbConn->executeQuery(dbQuery, dbRes); 

    if (error != Blaze::ERR_OK)
    {
        ERR_LOG("[GlobalStatsCache].fetchCategoryTable: failed to execute db statement."); 
    }
    else
    {
        for (DbResult::const_iterator it = dbRes->begin(); it != dbRes->end(); it++)
        {
            size_t index = 0;
            DbRow *dbRow = *it;
            
            // entity id is the first column
            EntityId entityId = (EntityId)dbRow->getInt64((uint32_t)index++);
            
            // periodid is the second
            int32_t periodId = 0;
            if (category.getPeriod() != STAT_PERIOD_ALL_TIME)
                periodId = (int32_t)dbRow->getInt((uint32_t)index++);

            // following columns are scope vector
            GlobalCacheScopeVector scopeVector;
            scopeVector.reserve(category.mScopeSize);
            for (size_t i = 0; i < category.mScopeSize; i++)
                scopeVector.push_back(dbRow->getInt64((uint32_t)index++));

            GlobalCacheRowKey key(entityId, periodId, scopeVector);
            GlobalCacheRow& row = category.addNewRow(key);

            error = dbRowToCategoryRow(statCategory, *dbRow, index, row);
            // reset row state because we've just fetched it from db
            row.mRowState = GCRS_UNCHANGED;
        }
    }

    TRACE_LOG("[StatsSerializer].fetchCategoryTable: completed fatching table for category " << categoryName); 

    return error;
}

BlazeRpcError GlobalStatsCache::dbRowToCategoryRow(const StatCategory *statCategory, const DbRow &dbRow,  const size_t index, GlobalCacheRow &row)
{
    const StatList& statList = statCategory->getStats();
    StatList::const_iterator it = statList.begin();
    size_t dbCol = index; 
    size_t statCol = 0;
    for (; it != statList.end(); it++)
    {
        const Stat *stat = *it;
        StatType statType = stat->getDbType().getType();

        if (statType == STAT_TYPE_INT)
        {
            GlobalCacheCell cell((int64_t)dbRow.getInt64(dbCol));
            row.setStatCell(statCol, cell);
        } else if (statType == STAT_TYPE_FLOAT) 
        {
            GlobalCacheCell cell(dbRow.getFloat(dbCol));
            row.setStatCell(statCol, cell);
        } else if (statType == STAT_TYPE_STRING)
        {
            GlobalCacheCell cell(dbRow.getString(dbCol));
            row.setStatCell(statCol, cell);
        }
        else
        {
            // this is invalid - unknown stat type
            EA_ASSERT_MSG(false, "Unknown stat type.");
        }

        dbCol++;
        statCol++;
    }

    return Blaze::ERR_OK;
}

BlazeRpcError GlobalStatsCache::serializeToDatabase(
    bool force,
    size_t &countOfRowsWritten)
{
    TRACE_LOG("[GlobalStatsCache].serializeToDatabase: Serializing to database"); 

    BlazeRpcError error = Blaze::ERR_OK;
    mIsReady = false;

    // get database instance
    DbConnPtr dbConn = gDbScheduler->getConnPtr(mComponent.getDbId());
    if (dbConn == nullptr)
    {
        ERR_LOG("[GlobalStatsCache].serializeToDatabase: Could not get database connection"); 
        return Blaze::ERR_SYSTEM;
    }

    countOfRowsWritten = 0;

    // iterate all tables and serialize them to database
    for (GlobalCacheCategoriesByNameByPeriodMap::iterator it = mCategoriesByNameByPeriodMap.begin();
         it != mCategoriesByNameByPeriodMap.end();
         it++)
    {
        GlobalCacheCategoriesByNameMap &catMap = it->second;
        for (GlobalCacheCategoriesByNameMap::iterator nit = catMap.begin();
            nit != catMap.end() && error == Blaze::ERR_OK;
            nit++)
        {
            GlobalCacheCategoryTable *table = nit->second;
            error = serializeCategoryTable(dbConn, force, *table, countOfRowsWritten);
        }
    }

    mIsReady = true;
    
    // wake up fibers that issued read/write request while cache was being written to db
    signalBlockedFibers();

    TRACE_LOG("[GlobalStatsCache].serializeToDatabase: Done serializing to database."); 
    return error;
}

BlazeRpcError GlobalStatsCache::serializeCategoryTable(
    DbConnPtr dbConn, 
    bool force,
    GlobalCacheCategoryTable &categoryTable,
    size_t &countOfRowsWritten)
{
    TRACE_LOG("[GlobalStatsCache].serializeCategoryTable: serializing category into table " << categoryTable.mCategory.getTableName(categoryTable.mPeriod));

    QueryPtr dbQuery = DB_NEW_QUERY_PTR(dbConn);
    categoryTable.reset();
    
    size_t countOfRowsToWrite = 0;

    while (categoryTable.hasNextRow())
    {
        GlobalCacheRow* row = categoryTable.getNextRow();
        if (force || row->mRowState == GCRS_UPDATED || row->mRowState == GCRS_INSERTED)
        {
            countOfRowsToWrite++;

            dbQuery->append("INSERT INTO `$s` (`entity_id`", categoryTable.mCategory.getTableName(categoryTable.mPeriod));

            if (categoryTable.getPeriod() != STAT_PERIOD_ALL_TIME)
            {
                dbQuery->append(", `period_id` ");
            }

            for (const auto& pair : categoryTable.mNameToScopeIndexMap)
            {
                dbQuery->append(", `$s` ", pair.first.c_str());
            }

            for (const auto& pair : categoryTable.mNameToCellIndexMap)
            {
                dbQuery->append(", `$s` ", pair.first.c_str());
            }

            dbQuery->append(") VALUES ( ");

            dbQuery->append("'$D' ", row->getEntityId());

            if (categoryTable.getPeriod() != STAT_PERIOD_ALL_TIME)
            {
                dbQuery->append(", '$d' ", row->getPeriodId());
            }

            const GlobalCacheScopeVector& scopeVector = row->getScopeVector();
            for (const auto& pair : categoryTable.mNameToScopeIndexMap)
            {
                dbQuery->append(", '$d' ", scopeVector[pair.second]);
            }

            for (const auto& pair : categoryTable.mNameToCellIndexMap)
            {
                dbQuery->append(", '$s' ", row->getStatCell(pair.second).toString().c_str());
            }

            dbQuery->append(") ON DUPLICATE KEY UPDATE ");

            for (const auto& pair : categoryTable.mNameToCellIndexMap)
            {
                eastl::string str = row->getStatCell(pair.second).toString();
                dbQuery->append(" `$s`='$s',", pair.first.c_str(), str.c_str());
            }

            // eliminate trailing comma
            dbQuery->trim(1); 
            dbQuery->append(";\n");
        }
    }

    if (countOfRowsToWrite > 0)
    {
        DbResultPtrs dbRes;
        BlazeRpcError error = dbConn->executeMultiQuery(dbQuery, dbRes); 

        if (error != Blaze::ERR_OK)
        {
            ERR_LOG("[GlobalStatsCache].serializeCategoryTable: failed to execute db statement."); 
        }
        else
        {
            countOfRowsWritten += countOfRowsToWrite;

            categoryTable.reset();
            while (categoryTable.hasNextRow())
            {
                // absolve all the rows
                GlobalCacheRow* row = categoryTable.getNextRow();
                row->mRowState = GCRS_UNCHANGED;
            }
        }
    }

    TRACE_LOG("[GlobalStatsCache].serializeCategoryTable: completed serializing " 
        << countOfRowsToWrite << " rows to table " 
        << categoryTable.mCategory.getTableName(categoryTable.mPeriod));

    return Blaze::ERR_OK;
}

void GlobalStatsCache::makeCacheRowsClean()
{
    // iterate all tables and clean rows
    for (GlobalCacheCategoriesByNameByPeriodMap::iterator it = mCategoriesByNameByPeriodMap.begin();
         it != mCategoriesByNameByPeriodMap.end();
         it++)
    {
        GlobalCacheCategoriesByNameMap &catMap = it->second;
        for (GlobalCacheCategoriesByNameMap::iterator nit = catMap.begin();
            nit != catMap.end();
            nit++)
        {
            GlobalCacheCategoryTable &categoryTable = *(nit->second);
            categoryTable.reset();
            while (categoryTable.hasNextRow())
            {
                GlobalCacheRow* row = categoryTable.getNextRow();
                row->mRowState = GCRS_UNCHANGED;
            }
        }
    }
}


} // Stats
} // Blaze
