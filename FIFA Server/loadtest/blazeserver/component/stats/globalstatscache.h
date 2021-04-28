/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_GLOBALSTATSCACHE_H
#define BLAZE_STATS_GLOBALSTATSCACHE_H

#include "EASTL/map.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"

#include "framework/database/dbconn.h"

#include "stats/tdf/stats.h"
#include "stats/statscommontypes.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
namespace Stats
{

class StatsSlaveImpl;

class GlobalCacheCell
{
public:
    GlobalCacheCell(int64_t value);
    GlobalCacheCell(float value);
    GlobalCacheCell(const char8_t* str);

    int64_t getInt64() const;
    float getFloat() const;
    const char8_t* getString() const;

    void setInt64(int64_t value);
    void setFloat(float value);
    void setString(const char8_t* value);

    StatType getType() const { return mCellType; }

    const eastl::string toString() const;

private:
    int64_t mIntValue;
    float mFloatValue;
    eastl::string mStringValue;
    StatType mCellType; 
};


enum GlobalCacheRowState
{
    GCRS_UNCHANGED = 0,
    GCRS_UPDATED,
    GCRS_INSERTED,
    GCRS_DELETED
};

typedef eastl::vector<ScopeValue> GlobalCacheScopeVector;

class GlobalCacheRowKey
{
public:
     GlobalCacheRowKey(
         EntityId entityId,
         int32_t periodId,
         const GlobalCacheScopeVector &scopeVector)
         : mEntityId(entityId), mPeriodId(periodId), mScopeVector(scopeVector) { }

     GlobalCacheRowKey(const GlobalCacheRowKey& ref) { *this = ref; }

     EntityId getEntityId() const { return mEntityId; }
     int32_t getPeriodId() const { return mPeriodId; }
     const GlobalCacheScopeVector& getScopeVector() const { return mScopeVector; }

     bool operator==(const GlobalCacheRowKey&) const;
     bool operator<(const GlobalCacheRowKey&) const;
     GlobalCacheRowKey& operator=(const GlobalCacheRowKey&);

     const eastl::string toString() const;

private:
    EntityId mEntityId;
    int32_t mPeriodId;
    GlobalCacheScopeVector mScopeVector;

};

class GlobalCacheCategoryTable;

class GlobalCacheRow
{
private:
    NON_COPYABLE(GlobalCacheRow);

public:
    const GlobalCacheCell& getCell(const char8_t *name) const;
    void setStatCell(const char8_t *name, const GlobalCacheCell& cell);
    void setStatCell(size_t index, const GlobalCacheCell& cell);
    const GlobalCacheCell& getStatCell(size_t index) const;
    
    EntityId getEntityId() const { return mKey.getEntityId(); }
    int32_t getPeriodId() const { return mKey.getPeriodId(); }
    const GlobalCacheScopeVector& getScopeVector() const { return mKey.getScopeVector(); }

    size_t getCellCount() const { return mRow.size(); }
    GlobalCacheRowState getState() const { return mRowState; }

private:
    friend class GlobalCacheCategoryTable;
    friend class GlobalStatsCache;

    GlobalCacheRow(
        GlobalCacheRowState rowState, 
        GlobalCacheCategoryTable &table, 
        size_t cellCount,
        const GlobalCacheRowKey &key);

    GlobalCacheRowState mRowState;
    GlobalCacheCategoryTable &mTable;

    typedef eastl::vector<GlobalCacheCell> Row;
    Row mRow;
    Row mScopes;
    GlobalCacheRowKey mKey;
};

class GlobalCacheCategoryTable
{
private:
    NON_COPYABLE(GlobalCacheCategoryTable);

public:
    ~GlobalCacheCategoryTable();

    // row info
    static const size_t ILLEGAL_INDEX = (size_t)(-1);
    size_t getCellIndex(const char8_t *name) const;
    size_t getScopeIndex(const char8_t *name) const;
    
    // row manipulation
    GlobalCacheRow& addNewRow(const GlobalCacheRowKey &key);
    void deleteRow(const GlobalCacheRowKey &key);

    // row iteration
    void reset();
    bool hasNextRow();
    GlobalCacheRow* getNextRow();

    GlobalCacheRow* findRow(const GlobalCacheRowKey &key);
    GlobalCacheRow* findRow(const EntityId entityId, const int32_t periodId, const ScopeNameValueMap &scopeKey);

    int32_t getPeriod() { return mPeriod; }
    size_t getRowSize() { return mRowSize; }
    size_t getScopeSize() { return mScopeSize; }

    typedef eastl::map<GlobalCacheRowKey, GlobalCacheRow*> CategoryTable;

private:
    friend class GlobalStatsCache;

    GlobalCacheCategoryTable(const StatCategory &category, const int32_t periodType);

    GlobalCacheRow& addRow(const GlobalCacheRowKey &key, GlobalCacheRowState rowState);

    typedef eastl::map<eastl::string, size_t> NameToIndexMap;
    typedef eastl::vector<GlobalCacheRowKey> KeyVector;

    const StatCategory &mCategory;
    CategoryTable::iterator mRowIterator;
    CategoryTable mCategoryTable;
    NameToIndexMap mNameToCellIndexMap;
    NameToIndexMap mNameToScopeIndexMap;
    KeyVector mDeletedKeys;
    const int32_t mPeriod;
    size_t mRowSize;
    size_t mScopeSize;
};

class GlobalStatsCache
{
private:
    NON_COPYABLE(GlobalStatsCache);

public:
    GlobalStatsCache(StatsSlaveImpl& statsSlave);
    ~GlobalStatsCache();

    BlazeRpcError initialize();

    GlobalCacheCategoryTable* getCategoryTable(
        const char8_t* categoryName, 
        const int32_t periodType);

    BlazeRpcError serializeToDatabase(
        bool force,
        size_t &countOfRowsWritten);

    void makeCacheRowsClean();
    bool isInitialized() const { return mIsInitialized; } 
    bool isReady() const { return mIsReady; } 

private:
    typedef eastl::map<eastl::string, GlobalCacheCategoryTable*> GlobalCacheCategoriesByNameMap;
    typedef eastl::map<int32_t, GlobalCacheCategoriesByNameMap> GlobalCacheCategoriesByNameByPeriodMap;

    GlobalCacheCategoriesByNameByPeriodMap mCategoriesByNameByPeriodMap;

    BlazeRpcError fetchCategoryTable(
        DbConnPtr dbConn, 
        const char8_t* categoryName,
        int32_t periodType, 
        GlobalCacheCategoryTable &category);

    void signalBlockedFibers();

private:
    // helpers
    BlazeRpcError dbRowToCategoryRow(const StatCategory *statCategory, const DbRow &dbRow,  const size_t index, GlobalCacheRow &row);
    const StatCategory* findCategory(const char8_t* categoryName) const;
    BlazeRpcError serializeCategoryTable(
        DbConnPtr dbConn, 
        bool force,
        GlobalCacheCategoryTable &categoryTable,
        size_t &countOfRowsWritten);

    typedef eastl::vector<Fiber::EventHandle> GlobalCacheEvents;
    GlobalCacheEvents mGlobalCacheEvents;

private:
    StatsSlaveImpl& mComponent;
    bool mIsInitialized;
    bool mIsReady;
    bool mIsWritingToDatabase;
};


} // Stats
} // Blaze
#endif 
