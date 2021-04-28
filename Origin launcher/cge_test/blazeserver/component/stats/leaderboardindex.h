/*************************************************************************************************/
/*!
    \file   leaderboardindex.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_LEADERBOARDINDEX_H
#define BLAZE_STATS_LEADERBOARDINDEX_H

#include "EASTL/list.h"
#include "EASTL/hash_map.h"
#include "EASTL/hash_set.h"
#include "EASTL/queue.h"
#include "EASTL/sort.h"
#include "framework/identity/identity.h"
#include "framework/userset/userset.h"
#include "framework/database/dbconn.h"
#include "framework/metrics/metrics.h"
#include "stats/leaderboarddata.h"
#include "stats/tdf/stats_server.h"
#include "statsconfig.h"
#include "statscache.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
namespace Stats
{
class StatsMaster;
class KeyScopeLeaderboards;
class KeyScopeLeaderboard;
class LeaderboardIndexBase;
class HistoricalLeaderboards;

typedef eastl::list<EA::TDF::TdfBlobPtr> LeaderboardEntries; // for extraction and population
typedef eastl::vector_map<uint32_t, EntityId> RankedUserSetMap;
typedef eastl::vector<EntityId> UnrankedUserSetList;
typedef eastl::set<eastl::string> ColumnSet;
typedef eastl::hash_set<const char8_t*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > StatNameSet;

/*************************************************************************************************/
/*!
    \brief TableInfo

    For a given stat category, contains information about all of the associated leaderboards
*/
/*************************************************************************************************/
struct TableInfo
{
    TableInfo()
    :   periodType(STAT_PERIOD_ALL_TIME),
        categoryName(nullptr),
        loadFromDbTables(true)
    {}

    const ScopeNameSet* keyScopeColumns; // collection of all keyscope columns for this category
    ColumnSet statColumns; // collection of non keyscope stat columns used by all leaderboards for this category
    typedef eastl::vector<KeyScopeLeaderboards*> LeaderboardList;
    LeaderboardList leaderboards; // collection of leaderboards configured for this category
    int32_t periodType; // period type for table
    const char8_t* categoryName; // name of the stat category
    bool loadFromDbTables; //true if the leaderboards from this category should be loaded from the database
};

typedef eastl::hash_map<const char8_t*, TableInfo, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > TableInfoMap; //map of stat table name to TableInfo

class LeaderboardIndexes
{
public:
    LeaderboardIndexes(StatsComponentInterface* component, Metrics::MetricsCollection& metricsCollection, bool isMaster);
    ~LeaderboardIndexes();

    void buildIndexesStructures();
    BlazeRpcError loadData(const TableInfoMap& tableInfo);

    BlazeRpcError getLeaderboardRows(const StatLeaderboard& leaderboard,
        const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, int32_t startRank, int32_t count,
        const EA::TDF::ObjectId& userSetId, EntityIdList& keys, LeaderboardStatValues& response, bool& sorted) const;
    BlazeRpcError getCenteredLeaderboardRows(const StatLeaderboard& leaderboard,
        const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, EntityId centerEntityId, int32_t count,
        const EA::TDF::ObjectId& userSetId, bool showAtBottomIfNotFound,
        EntityIdList& keys, LeaderboardStatValues& response, bool& sorted) const;
    BlazeRpcError getFilteredLeaderboardRows(const StatLeaderboard& leaderboard,
        const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId, const EntityIdList& idList, uint32_t limit,
        EntityIdList& keys, LeaderboardStatValues& response, bool& sorted) const;
    BlazeRpcError getLeaderboardRanking(const StatLeaderboard* leaderboard, const ScopeNameValueListMap* scopeNameValueListMap,
        int32_t periodId, EntityId centerEntityId, int32_t &entityRank) const;
    BlazeRpcError getEntityCount(const StatLeaderboard* leaderboard, const ScopeNameValueListMap* scopeNameValueListMap,
        int32_t periodId, EntityCount* response) const;

    bool requestIndexesFromMaster(StatsMaster* statsMaster);
    void extract(const PopulateLeaderboardIndexRequest& request, LeaderboardEntries* leaderboardEntries) const;
    bool writeIndexesToDb(DbConnPtr conn, const TableInfoMap& tableInfoMap);
    void scheduleDbUpdates();

    bool isLeaderboard(uint32_t categoryId, const char8_t* statName, int32_t periodType, const ScopeNameValueMap* scopeNameValueMap) const;

    void updateStats(const CacheRowUpdateList& data);
    void updateStats(uint32_t categoryId, EntityId entityId, int32_t periodType, int32_t periodId, const StatValMap* statValMap, const ScopeNameValueMap* scopeMap, const char8_t* keyScopeString, StringBuilder& lbQueryString, StatNameValueMap& statMap);

    void deleteStats(const CacheRowDeleteList& data, const Stats::PeriodIdMap& currentPeriodIdMap, StringBuilder* lbQueryString);
    void deleteStatsByEntity(const CacheRowDeleteList& data, const Stats::PeriodIdMap& currentPeriodIdMap, StringBuilder* lbQueryString);
    void deleteCategoryStats(const CacheRowDeleteList& data, const Stats::PeriodIdMap& currentPeriodIdMap, StringBuilder* lbQueryString, bool dbTruncateBlocksAllTxn);
    void deleteCategoryStats(const CacheRowDeleteList& data, const Stats::PeriodIdMap& currentPeriodIdMap) { deleteCategoryStats(data, currentPeriodIdMap, nullptr, false); }
    void trimPeriods(int32_t periodType, int32_t periodsToKeep);
    void scheduleTrimPeriods(int32_t periodType, int32_t periodsToKeep);

    uint64_t getMemoryBudget() const;
    uint64_t getMemoryAllocated() const;
    uint64_t getLeaderboardCount() const;
    uint64_t getLeaderboardRowCount() const;

    void getStatusInfo(ComponentStatus& status) const;

    void initStatTablesForLeaderboards();
    TableInfoMap& getStatTablesForLeaderboards() { return mStatTables; }
    static void getLeaderboardTableName(const char8_t* statTableName, char8_t* buffer, size_t length);

    // Maps leaderboard ID to the class LeaderboardIndexBase
    typedef eastl::hash_map<int32_t, LeaderboardIndexBase*> LeaderboardMap;

    // Maps period id to leaderboards
    typedef eastl::hash_map<int32_t, LeaderboardMap*> PeriodIdMap;

    // Maps keyscope string into period id map
    typedef eastl::hash_map<const char8_t*, HistoricalLeaderboards*,
        eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > KeyScopeLeaderboardMap;

    // Maps single value keyscope string to multi-value keyscope string
    typedef eastl::hash_map<const char8_t*, eastl::list<const char8_t*>*,
        eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > KeyScopeSingleMultiValueMap;

    // Maps period type to the KeyScopeLeaderboards class
    typedef eastl::hash_map<int32_t, KeyScopeLeaderboards*> PeriodTypeMap;

    // Maps internal category id to the map of period types
    typedef eastl::hash_map<uint32_t, PeriodTypeMap*> CategoryMap;

    // list of leaderboards for the given key scope
    typedef eastl::vector<const StatLeaderboard*> LeaderboardList;

    struct ScopeCell
    {
        const char8_t* name;
        ScopeStartEndValuesMap::iterator mapItr;
        ScopeValue currentValue; // for multi-value keyscopes, KEY_SCOPE_VALUE_MULTI indicates valuesMap holds the multi-values
        ScopeStartEndValuesMap valuesMap; // *copy* from config's keyscopes map (but includes aggregrate key value, if any)

        ScopeCell(): name(nullptr), mapItr(nullptr), currentValue(0) { valuesMap.clear();}

        ScopeCell& operator=(const ScopeCell& rhs)
        {
            if (&rhs != this)
            {
                name = rhs.name;
                rhs.valuesMap.copyInto(valuesMap);
                mapItr = valuesMap.begin();
                currentValue = rhs.currentValue;
            }
            return *this;
        }

        ScopeCell(const ScopeCell& rhs)
        {
            *this = rhs;
        }
    };
    
    typedef eastl::vector<ScopeCell> ScopeVector;

    static void buildKeyScopeString(char8_t* buffer, size_t bufLen, const ScopeNameValueMap& scopeNameValueMap);
    static void populateCacheRow(CacheRowUpdate& update, uint32_t categoryId, EntityId entityId, int32_t periodType, int32_t periodId, const char8_t* keyScopeString);

private:
    BlazeRpcError loadData(uint32_t dbId, const char8_t* tableName, const TableInfo& table);
    void loadDataRow(const char8_t* tableName, const TableInfo& table, const char8_t* lbTableName, const char8_t* keyscope,
        HistoricalLeaderboards* histLb, DbConnPtr conn, StreamingDbResultPtr result, DbRow* row);

    void buildDetailedIndexesStructures(const ScopeVector *scopeVector,
        const StatLeaderboard* leaderboard, KeyScopeLeaderboards* keyScopeLeaderboards);

    void updateIndexes(const StatLeaderboard* leaderboard, EntityId entityId, int32_t periodId, const StatNameValueMap& statValMap, PeriodIdMap* periodIdMap);
    void updateIndexes(const StatLeaderboard* leaderboard, EntityId entityId, int32_t periodId, const StatValMap* statValMap, PeriodIdMap* periodIdMap, StatNameValueMap& statMap, UpdateLeaderboardResult& result);

    uint64_t getInfo(uint64_t(LeaderboardIndexBase::*mf)() const) const;

    BlazeRpcError getLeaderboardIndex(const StatLeaderboard& leaderboard,
        const ScopeNameValueListMap* scopeNameValueListMap, int32_t periodId,
        LeaderboardIndexBase** leaderboardIndex) const;

    char8_t* makeKeyScopeString(const ScopeNameValueListMap* scopeNameValueListMap) const;
    char8_t* makeKeyScopeString(const ScopeNameValueMap* scopeNameValueMap) const;
    char8_t* makeKeyScopeString(const ScopeVector* scopeVector) const;
    char8_t* makeKeyScopeString(const ScopeVector* scopeVector, bool& isMultiValue) const;
    char8_t* makeKeyScopeString(const ScopeNameSet* scopeNameSet, const DbRow* row) const;

    bool growString(char8_t*& buf, size_t& bufSize, size_t bufCount, size_t bytesNeeded) const;

    void writeLbIndexesUpdatesToDb();
    BlazeRpcError doWriteIndexesUpdatesToDb();

    // queueDbTablesUpdate helpers
    void makeDbTablesDelete(StringBuilder& lbQueryString, EntityId entityId, uint32_t categoryId, int32_t periodType, int32_t periodId, const ScopeNameValueMap* scopeMap);
    void makeDbTablesUpsert(StringBuilder& lbQueryString, EntityId entityId, uint32_t categoryId, int32_t periodType, int32_t periodId, const ScopeNameValueMap* scopeMap, const StatNameSet& statNames, const StatValMap* statValMap);
    bool isRanked(EntityId entityId, uint32_t categoryId, int32_t periodType, int32_t periodId, const ScopeNameValueMap* scopeMap);

    void schedulePopulateTimeout();
    void executePopulateTimeout();
    void cancelPopulateTimeout();

private:
    StatsComponentInterface* mComponent;
    bool mIsMaster;
    CategoryMap mCategoryMap;
    TableInfoMap mStatTables;

    // startup metrics
    uint64_t mTotalLeaderboards; // tallied when building index structures
    uint64_t mTotalLeaderboardEntries; // tallied when building index structures
    uint64_t mCountLeaderboards; // running count when populating
    const char8_t* mPopulatingLeaderboardName;
    TimerId mPopulateTimerId;

    // On server startup, any missing lb index tables will be built up by dumping from the main stats table,
    // these data structure handle the temporary startup-time-only caching of that data before it gets
    // written back out to the db to the lb index tables
    struct QueryAndCount
    {
        QueryAndCount() : count(0), query(nullptr) {}
        ~QueryAndCount()
        {
            if (query != nullptr )
                BLAZE_FREE(query);
            query = nullptr;
        }

        uint32_t count;
        const char8_t* query;
    };

    typedef eastl::hash_map<const char8_t*, QueryAndCount, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > KeyscopeQueryMap;
    typedef eastl::hash_map<EntityId, KeyscopeQueryMap> EntityIdKeyscopeMap;
    typedef eastl::hash_map<const char8_t*, EntityIdKeyscopeMap> LeaderboardQueryMap;
    LeaderboardQueryMap mLeaderboardQueryMap;

    TimerId mTrimPeriodsTimerIds[STAT_NUM_PERIODS];

    Metrics::Gauge mGaugeBudgetOfAllRankingTables;
    Metrics::PolledGauge mGaugeSizeOfAllRankingTables;
    Metrics::PolledGauge mGaugeLeaderboardCount;
    Metrics::PolledGauge mGaugeLeaderboardRowCount;
};

class KeyScopeLeaderboards
{
public:
    KeyScopeLeaderboards(){}
    ~KeyScopeLeaderboards();

    LeaderboardIndexes::KeyScopeLeaderboardMap* getKeyScopeLeaderboardMap()
    { return &mKeyScopeLeaderboardMap; }
    const LeaderboardIndexes::KeyScopeLeaderboardMap* getKeyScopeLeaderboardMap() const
    { return &mKeyScopeLeaderboardMap; }
    LeaderboardIndexes::KeyScopeSingleMultiValueMap* getKeyScopeSingleMultiValueMap()
    { return &mKeyScopeSingleMultiValueMap; }
    const LeaderboardIndexes::KeyScopeSingleMultiValueMap* getKeyScopeSingleMultiValueMap() const
    { return &mKeyScopeSingleMultiValueMap; }

    void addLeaderboardStat(const char8_t* statName)
    {
        mStatNames.insert(statName);
    }
    bool isLeaderboardStat(const char8_t* statName) const
    {
        return (mStatNames.find(statName) != mStatNames.end());
    }
    const StatNameSet& getStatNames() const { return mStatNames; }

private:
    LeaderboardIndexes::KeyScopeLeaderboardMap mKeyScopeLeaderboardMap;
    LeaderboardIndexes::KeyScopeSingleMultiValueMap mKeyScopeSingleMultiValueMap;

    StatNameSet mStatNames;
};

class HistoricalLeaderboards
{
public:
    HistoricalLeaderboards()
    {
    }
    ~HistoricalLeaderboards();
    LeaderboardIndexes::PeriodIdMap* getPeriodIdMap() { return &mPeriodIdMap; }
    LeaderboardIndexes::LeaderboardList* getLeaderboardList() { return &mLeaderboardList; }

private:
    LeaderboardIndexes::PeriodIdMap mPeriodIdMap;
    LeaderboardIndexes::LeaderboardList mLeaderboardList;
};


class LeaderboardIndexBase
{
public:
    LeaderboardIndexBase(const StatLeaderboard* leaderboard)
        : mLeaderboard(leaderboard) {}
    virtual ~LeaderboardIndexBase() {}

    const StatLeaderboard* getLeaderboard() const { return mLeaderboard; }

    virtual void reset() = 0;
    virtual void remove(EntityId entityId, UpdateLeaderboardResult& result) = 0;
    virtual bool populate(const EA::TDF::TdfBlob& data) = 0;
    virtual uint32_t extract(EA::TDF::TdfBlob* data, uint32_t start, uint32_t count) const = 0;
    virtual uint32_t getValueSize() const = 0;

    virtual BlazeRpcError getRows(int32_t startRank, int32_t count,
        LeaderboardStatValues::RowList& rows, EntityIdList& keys) const = 0;
    virtual BlazeRpcError getRowsForUserSet(int32_t startRank, int32_t count, EA::TDF::ObjectId userSetId,
        LeaderboardStatValues::RowList& rows, EntityIdList& keys, bool& sorted) const = 0;
    virtual BlazeRpcError getCenteredRows(EntityId entityId, int32_t count, bool showAtBottomIfNotFound,
        LeaderboardStatValues::RowList& rows, EntityIdList& keys) const = 0;
    virtual BlazeRpcError getCenteredRowsForUserSet(EntityId entityId, int32_t count, EA::TDF::ObjectId userSetId,
        LeaderboardStatValues::RowList& rows, EntityIdList& keys, bool& sorted) const = 0;
    virtual BlazeRpcError getFilteredRows(const EntityIdList& ids, uint32_t limit,
        LeaderboardStatValues::RowList& rows, EntityIdList& keys) const = 0;
    virtual uint32_t getRankByEntity(EntityId entityId) const = 0;
    virtual bool containsEntity(EntityId entityId) const = 0;
    virtual int32_t getTiedRankByEntity(EntityId entityId) const =0;
    virtual uint32_t getCount() const = 0;
    virtual uint64_t getMemoryBudget() const = 0;
    virtual uint64_t getMemoryAllocated() const = 0;

protected:
    const StatLeaderboard* mLeaderboard;
};

template <typename T>
class LeaderboardIndex : public LeaderboardIndexBase
{
public:
    // TBD perhaps extra rows should be explicitly specified in config file
    LeaderboardIndex(const StatLeaderboard* leaderboard);
    ~LeaderboardIndex() override {}

    void update(EntityId entityId, T value, UpdateLeaderboardResult& result);
    void reset() override { mLeaderboardData.reset(); }
    void remove(EntityId entityId, UpdateLeaderboardResult& result) override { mLeaderboardData.remove(entityId, result); }
    bool populate(const EA::TDF::TdfBlob& data) override;
    uint32_t extract(EA::TDF::TdfBlob* data, uint32_t start, uint32_t count) const override;
    uint32_t getValueSize() const override;

    BlazeRpcError getRows(int32_t startRank, int32_t count,
        LeaderboardStatValues::RowList& rows, EntityIdList& keys) const override;
    BlazeRpcError getRowsForUserSet(int32_t startRank, int32_t count, EA::TDF::ObjectId userSetId,
        LeaderboardStatValues::RowList& rows, EntityIdList& keys, bool& sorted) const override;
    BlazeRpcError getCenteredRows(EntityId entityId, int32_t count, bool showAtBottomIfNotFound,
        LeaderboardStatValues::RowList& rows, EntityIdList& keys) const override;
    BlazeRpcError getCenteredRowsForUserSet(EntityId entityId, int32_t count, EA::TDF::ObjectId userSetId,
        LeaderboardStatValues::RowList& rows, EntityIdList& keys, bool& sorted) const override;
    BlazeRpcError getFilteredRows(const EntityIdList& ids, uint32_t limit,
        LeaderboardStatValues::RowList& rows, EntityIdList& keys) const override;
    uint32_t getRankByEntity(EntityId entityId) const override { return mLeaderboardData.getRank(entityId); }
    bool containsEntity(EntityId entityId) const override { return mLeaderboardData.contains(entityId); }
    uint32_t getCount() const override { return mLeaderboardData.getCount(); }
    uint64_t getMemoryBudget() const override { return mLeaderboardData.getMemoryBudget(); }
    uint64_t getMemoryAllocated() const override { return mLeaderboardData.getMemoryAllocated(); }
    int32_t getTiedRank(int32_t rank, const Entry<T>& entry) const;
    int32_t getTiedRankByEntity(EntityId entityId) const override;
    
private:
    void populateValue(Entry<float_t>& entry, const uint8_t* blob);
    void populateValue(Entry<int64_t>& entry, const uint8_t* blob);
    void populateValue(Entry<int32_t>& entry, const uint8_t* blob);
    void populateValue(Entry<MultiRankData>& entry, const uint8_t* blob);

    void extractValue(uint8_t* blob, const Entry<float_t>& entry) const;
    void extractValue(uint8_t* blob, const Entry<int64_t>& entry) const;
    void extractValue(uint8_t* blob, const Entry<int32_t>& entry) const;
    void extractValue(uint8_t* blob, const Entry<MultiRankData>& entry) const;

    bool isCutoff(int32_t value) const { return mLeaderboard->isIntCutoff(value); }
    bool isCutoff(int64_t value) const { return mLeaderboard->isIntCutoff(value); }
    bool isCutoff(float_t value) const { return mLeaderboard->isFloatCutoff(value); }
    bool isCutoff(const MultiRankData& value) const { return mLeaderboard->getRankingStats()[0]->getIsInt() ?
        mLeaderboard->isIntCutoff(value.data[0].intValue) : mLeaderboard->isFloatCutoff(value.data[0].floatValue); }

    LeaderboardData<T> mLeaderboardData;
};

/*************************************************************************************************/
/*!
    \brief update

    Update an entry in the leaderboard.  This will be called on the master when loading data
    from the database, as well as upon every stat update that affects this leaderboard while
    the server is running.  This method returns a simple boolean flag to indicate whether the
    entity is in the leaderboard or not following the update, to allow the caller to determine
    whether they should index additional info for this entity or not.

    \param[in]  entityId - the entity to update the leaderboard for
    \param[in]  value - the value of the stat for the entity
    \param[out]  result - the result of the update operation
*/
/*************************************************************************************************/
template <typename T>
void LeaderboardIndex<T>::update(EntityId entityId, T value, UpdateLeaderboardResult& result)
{
    if (isCutoff(value))
    {
        remove(entityId, result);
        return;
    }

    Entry<T> entry;
    entry.entityId = entityId;
    entry.value = value;
    mLeaderboardData.update(entry, result);   
}

/*************************************************************************************************/
/*!
    \brief getValueSize

    extract()/populate() helper to return the size of the 'value' component for an Entry of float_t type.

    \return - size of the 'value' component for the entry
*/
/*************************************************************************************************/
template <>
inline uint32_t LeaderboardIndex<float_t>::getValueSize() const
{
    return (uint32_t) sizeof(float_t);
}

/*************************************************************************************************/
/*!
    \brief getValueSize

    extract()/populate() helper to return the size of the 'value' component for an Entry of int64_t type.

    \return - size of the 'value' component for the entry
*/
/*************************************************************************************************/
template <>
inline uint32_t LeaderboardIndex<int64_t>::getValueSize() const
{
    return (uint32_t) sizeof(int64_t);
}

/*************************************************************************************************/
/*!
    \brief getValueSize

    extract()/populate() helper to return the size of the 'value' component for an Entry of int32_t type.

    \return - size of the 'value' component for the entry
*/
/*************************************************************************************************/
template <>
inline uint32_t LeaderboardIndex<int32_t>::getValueSize() const
{
    return (uint32_t) sizeof(int32_t);
}

/*************************************************************************************************/
/*!
    \brief getValueSize

    extract()/populate() helper to return the size of the 'value' component for an Entry of MultiRankData type.

    \return - size of the 'value' component for the entry
*/
/*************************************************************************************************/
template <>
inline uint32_t LeaderboardIndex<MultiRankData>::getValueSize() const
{
    const RankingStatList& stats = mLeaderboard->getRankingStats();

    return (uint32_t) stats.size() * sizeof(MultiRankData::LeaderboardValue);
}

/*************************************************************************************************/
/*!
    \brief populate

    This method is executed on the slave during the startup phase to quickly populate the index.
    As the input data should be prepared for us on the master via the extract() method from the
    corresponding leaderboard on the master, we are simply inserting the in order elements into
    the leaderboard without any need to search for insertion points.

    \param[in]  data - a chunk of index data given to the slave by the master

    \return - true if successful, false if there is insufficient room to populate all data
*/
/*************************************************************************************************/
template <typename T>
bool LeaderboardIndex<T>::populate(const EA::TDF::TdfBlob& data)
{
    const uint8_t* stream = data.getData();

    if (stream == nullptr)
    {
        // nothing to do ... early out
        return true;
    }

    uint32_t count = *(uint32_t *) stream;
    stream += sizeof(uint32_t);

    if (mLeaderboardData.getTotalCount() + count > mLeaderboardData.getCapacity())
    {
        ERR_LOG("[LeaderboardIndex].populate(): Received " << count
            << " entries from master for leaderboard " << getLeaderboard()->getBoardName()
            << " but we already have " << mLeaderboardData.getTotalCount() << " entries and are configured for "
            << mLeaderboardData.getCapacity() << " in total");
        return false;
    }

    uint32_t valueSize = getValueSize();

    for (; count != 0; --count)
    {
        Entry<T> entry;
        entry.entityId = *(EntityId *) stream;
        stream += sizeof(EntityId);
        populateValue(entry, stream);
        stream += valueSize;
        mLeaderboardData.populate(entry);
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief populateValue

    populate() helper to set an entry of float_t type.

    \param[out] entry - a leaderboard structure (sub-array) entry
    \param[in]  blob - encoded value from broadcast
*/
/*************************************************************************************************/
template <typename T>
void LeaderboardIndex<T>::populateValue(Entry<float_t>& entry, const uint8_t* blob)
{
    entry.value = *(float_t *) blob;
}

/*************************************************************************************************/
/*!
    \brief populateValue

    populate() helper to set an entry of int64_t type.

    \param[out] entry - a leaderboard structure (sub-array) entry
    \param[in]  blob - encoded value from broadcast
*/
/*************************************************************************************************/
template <typename T>
void LeaderboardIndex<T>::populateValue(Entry<int64_t>& entry, const uint8_t* blob)
{
    entry.value = *(int64_t *) blob;
}

/*************************************************************************************************/
/*!
    \brief populateValue

    populate() helper to set an entry of int32_t type.

    \param[out] entry - a leaderboard structure (sub-array) entry
    \param[in]  blob - encoded value from broadcast
*/
/*************************************************************************************************/
template <typename T>
void LeaderboardIndex<T>::populateValue(Entry<int32_t>& entry, const uint8_t* blob)
{
    entry.value = *(int32_t *) blob;
}

/*************************************************************************************************/
/*!
    \brief populateValue

    populate() helper to set an entry of MultiRankData type.

    \param[out] entry - a leaderboard structure (sub-array) entry
    \param[in]  blob - encoded value from broadcast
*/
/*************************************************************************************************/
template <typename T>
void LeaderboardIndex<T>::populateValue(Entry<MultiRankData>& entry, const uint8_t* blob)
{
    const RankingStatList& stats = mLeaderboard->getRankingStats();
    int32_t statsSize = stats.size();
    for (int32_t pos = 0; pos < statsSize; ++pos)
    {
        const RankingStatData* curStat = stats.at(pos);
        if (curStat->getIsInt())
        {
            entry.value.data[pos].intValue = *(int64_t *) blob;
        }
        else
        {
            entry.value.data[pos].floatValue = *(float_t *) blob;
        }
        blob += sizeof(MultiRankData::LeaderboardValue);
    }
}

/*************************************************************************************************/
/*!
    \brief extract

    This method is executed on the master when requested by a slave to prepare a chunk of index
    data in sorted leaderboard order.  This data will be consumed on the slave by the populate()
    method in this class.

    \param[out] data - a chunk of index data to be given to the slave by the master
    \param[in]  start - the starting index of the chunk to extract
    \param[in]  count - the number of rows to extract

    \return - actual number/count of rows extracted
*/
/*************************************************************************************************/
template <typename T>
uint32_t LeaderboardIndex<T>::extract(EA::TDF::TdfBlob* data, uint32_t start, uint32_t count) const
{
    uint32_t actualCount = 0;
    uint32_t end = start + count;
    if (end > mLeaderboardData.getTotalCount())
        end = mLeaderboardData.getTotalCount();

    uint32_t valueSize = getValueSize();

    data->resize(sizeof(uint32_t) + (sizeof(EntityId) + valueSize)*(end - start));
    uint8_t* stream = data->getData();
    stream += sizeof(uint32_t); // set the count after
    for (uint32_t i = start; i < end; ++i, ++actualCount)
    {
        const Entry<T>& entry = mLeaderboardData.getEntryByRank(i + 1);
        *(EntityId *) stream = entry.entityId;
        stream += sizeof(EntityId);
        extractValue(stream, entry);
        stream += valueSize;
    }

    // now set the count
    stream = data->getData();
    *(uint32_t *) stream = actualCount;

    return actualCount;
}

/*************************************************************************************************/
/*!
    \brief extractValue

    extract() helper to set an entry of float_t type.

    \param[out] blob - encoded value for broadcast
    \param[in]  entry - a leaderboard structure (sub-array) entry
*/
/*************************************************************************************************/
template <typename T>
void LeaderboardIndex<T>::extractValue(uint8_t* blob, const Entry<float_t>& entry) const
{
    *(float_t *) blob = entry.value;
}

/*************************************************************************************************/
/*!
    \brief extractValue

    extract() helper to set an entry of int64_t type.

    \param[out] blob - encoded value for broadcast
    \param[in]  entry - a leaderboard structure (sub-array) entry
*/
/*************************************************************************************************/
template <typename T>
void LeaderboardIndex<T>::extractValue(uint8_t* blob, const Entry<int64_t>& entry) const
{
    *(int64_t *) blob = entry.value;
}

/*************************************************************************************************/
/*!
    \brief extractValue

    extract() helper to set an entry of int32_t type.

    \param[out] blob - encoded value for broadcast
    \param[in]  entry - a leaderboard structure (sub-array) entry
*/
/*************************************************************************************************/
template <typename T>
void LeaderboardIndex<T>::extractValue(uint8_t* blob, const Entry<int32_t>& entry) const
{
    *(int32_t *) blob = entry.value;
}

/*************************************************************************************************/
/*!
    \brief extractValue

    extract() helper to set an entry of MultiRankData type.

    \param[out] blob - encoded value for broadcast
    \param[in]  entry - a leaderboard structure (sub-array) entry
*/
/*************************************************************************************************/
template <typename T>
void LeaderboardIndex<T>::extractValue(uint8_t* blob, const Entry<MultiRankData>& entry) const
{
    const RankingStatList& stats = mLeaderboard->getRankingStats();
    int32_t statsSize = stats.size();
    for (int32_t pos = 0; pos < statsSize; ++pos)
    {
        const RankingStatData* curStat = stats.at(pos);
        if (curStat->getIsInt())
        {
            *(int64_t *) blob = entry.value.data[pos].intValue;
        }
        else
        {
            *(float_t *) blob = entry.value.data[pos].floatValue;
        }
        blob += sizeof(MultiRankData::LeaderboardValue);
    }
}

/*************************************************************************************************/
/*!
    \brief getRows

    Fetch a sequence of rows from the leaderboard.  Will fetch the requested number of rows
    from the starting rank specified, or fewer if the leaderboard does not contain enough.

    \param[in]  startRank - the starting rank
    \param[in]  count - the max number of rows to fetch
    \param[out] rows - list of rows to be filled in by this method
    \param[out] keys - list of entity ids to be filled in by this method

    \return - ERR_OK or appropriate error code
*/
/*************************************************************************************************/
template <typename T>
BlazeRpcError LeaderboardIndex<T>::getRows(int32_t startRank, int32_t count,
    LeaderboardStatValues::RowList& rows, EntityIdList& keys) const
{
    int32_t size = mLeaderboardData.getCount();

    if (size == 0)
        return Blaze::ERR_OK;
    
    if ( startRank > size ) 
        return Blaze::ERR_OK;

    int32_t endRank = startRank + count - 1;
    if ( endRank > size )
        endRank = size;

    int32_t displayRank = 0;
    T prevValue = 0;
    keys.reserve(endRank - startRank + 1);
    rows.reserve(endRank - startRank + 1);
    for (int32_t rank = startRank; rank <= endRank; ++rank)
    {
        const Entry<T>& entry = mLeaderboardData.getEntryByRank(rank);
        keys.push_back(entry.entityId);
        
        if (mLeaderboard->isTiedRank())
        {
            if (rank == startRank)
            {
                displayRank = getTiedRank(rank, entry);
                prevValue = entry.value; 
            }
            else if (entry.value != prevValue)
            {
                displayRank = rank;
                prevValue = entry.value; 
            }
        }
        else
        {
            displayRank = rank;
        }    
        
        LeaderboardStatValuesRow* statRow = rows.pull_back();
        statRow->setRank(displayRank);
        statRow->getUser().setBlazeId(entry.entityId);
    }

    return Blaze::ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief getRowsForUserSet

    Fetch a sequence of rows from the leaderboard filtered by the user set.  Will fetch the requested 
    number of rows from the starting rank specified, or fewer if the user set does not contain enough.

    \param[in]  startRank - the starting non-tied rank relative to the user set
    \param[in]  count - the max number of rows to fetch
    \param[in]  userSetId - id of user set to filter leaderboard on
    \param[out] rows - list of rows to be filled in by this method
    \param[out] keys - list of entity ids to be filled in by this method
    \param[out] sorted - whether the list of rows is sorted

    \return - ERR_OK or appropriate error code
*/
/*************************************************************************************************/
template <typename T>
BlazeRpcError LeaderboardIndex<T>::getRowsForUserSet(int32_t startRank, int32_t count, EA::TDF::ObjectId userSetId,
    LeaderboardStatValues::RowList& rows, EntityIdList& keys, bool& sorted) const
{
    BlazeRpcError error = ERR_OK;
    uint32_t uss = 0;
    error = gUserSetManager->countUsers(userSetId, uss);
    if (error != ERR_OK)
        return error;
    int32_t userSetSize = static_cast<int32_t>(uss);
    if (userSetSize == 0)
        return ERR_OK;

    // startRank passed in, as well as ranks returned in request, are relative to the user set,
    // not overall/global ranking table
    if (startRank > userSetSize)
        return ERR_OK;

    int32_t endRank = startRank + count - 1;
    if (endRank > userSetSize)
        endRank = userSetSize;

    int32_t rankTableSize = mLeaderboardData.getCount();
    keys.reserve(endRank - startRank + 1);
    rows.reserve(endRank - startRank + 1);

    // If size of user set is comparable with a size of ranking table or bigger then it is very
    // likely that every entry in the overall leaderboard will have matching entry in the user set
    // and scan of ranking table may be more efficient. Although it should take into account number
    // of user sets using this ranking table. The ranking table contains sorted entries therefore
    // leaderboard may be filled in with a single pass through the ranking table. The user set is
    // unsorted and it would require a pass through the user set to pick up a single entry for the
    // leaderboard, although solution with a sorting of entire user set may also be considered.
    // To decide which container to scan, the number of db rows should be considered, but that is
    // an expensive operation. For now, just consider the sizes of the user set and ranking table.

    /// @todo better heuristics to determine whether to scan the user set or ranking table first
    if (userSetSize < rankTableSize)
    {
        // scan through the user set and sort it
        RankedUserSetMap rankedUserSetMap;
        UnrankedUserSetList unrankedUserSetList;

        BlazeIdList ids;
        error = gUserSetManager->getUserBlazeIds(userSetId, ids);
        if (error != ERR_OK)
            return error;

        unrankedUserSetList.reserve(ids.size());
        for (BlazeIdList::const_iterator idItr = ids.begin(); idItr != ids.end(); ++idItr)
        {
            EntityId id = *idItr;
            uint32_t globalRank = mLeaderboardData.getRank(id);

            if (globalRank == 0)
            {
                unrankedUserSetList.push_back(id);
            }
            else
            {
                rankedUserSetMap[globalRank] = id;
            }
        }

        int32_t rank = 1;

        // fill the request from the sorted users
        int32_t rankedUserSetSize = static_cast<int32_t>(rankedUserSetMap.size());
        if (startRank <= rankedUserSetSize)
        {
            // go to start rank
            RankedUserSetMap::iterator rankedItr = rankedUserSetMap.begin();
            RankedUserSetMap::iterator rankedEnd = rankedUserSetMap.end();
            for (; rank < startRank; ++rank, ++rankedItr)
                ; // with earlier rankedUserSetSize check, itr should never reach end here

            for (; rankedItr != rankedEnd && rank <= endRank; ++rankedItr, ++rank)
            {
                keys.push_back(rankedItr->second);
                LeaderboardStatValuesRow* statRow = rows.pull_back();
                statRow->setRank(rank);
                statRow->getUser().setBlazeId(rankedItr->second);
            }
        }
        else
        {
            // nothing from the sorted users
            rank += rankedUserSetSize;
        }

        // fill remaining request from the unranked users
        if (rank <= endRank)
        {
            // go to start rank (request may start in middle of unranked list)
            UnrankedUserSetList::iterator unrankedItr = unrankedUserSetList.begin();
            UnrankedUserSetList::iterator unrankedEnd = unrankedUserSetList.end();
            for (; rank < startRank; ++rank, ++unrankedItr)
                ; // with earlier userSetSize check, itr should never reach end here

            if (unrankedItr != unrankedEnd && rank <= endRank)
                sorted = false;

            for (; unrankedItr != unrankedEnd && rank <= endRank; ++unrankedItr, ++rank)
            {
                keys.push_back(*unrankedItr);
                LeaderboardStatValuesRow* statRow = rows.pull_back();
                statRow->setRank(0);
                statRow->getUser().setBlazeId(*unrankedItr);
            }
        }
    }
    else
    {
        BlazeIdList ids;
        error = gUserSetManager->getUserBlazeIds(userSetId, ids);
        if (error != ERR_OK)
            return error;

        int32_t rank = 0;

        // scan through the ranking table
        uint32_t globalRank;
        for (globalRank = 1; globalRank <= mLeaderboardData.getCount(); ++globalRank)
        {
            Entry<T> entry = mLeaderboardData.getEntryByRank(globalRank);

            /// @todo need a fast lookup into the user set--can use own hash_set here if UserSetManager won't do it
            for (BlazeIdList::const_iterator idItr = ids.begin(); idItr != ids.end(); ++idItr)
            {
                if (entry.entityId == *idItr)
                {
                    ++rank;
                    if (rank >= startRank)
                    {
                        keys.push_back(entry.entityId);
                        LeaderboardStatValuesRow* statRow = rows.pull_back();
                        statRow->setRank(rank);
                        statRow->getUser().setBlazeId(entry.entityId);
                    }
                    break; // this lookup is done
                }
            }

            if (rank >= endRank)
                break; // this request is done
        }
    }

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief getTiedRank

    Gets tied rank by entity

    \param[in]  entityId - entityId
    \return - tied rank
*/
/*************************************************************************************************/
template <typename T>
int32_t LeaderboardIndex<T>::getTiedRankByEntity(EntityId entityId) const
{
    uint32_t rank = mLeaderboardData.getRank(entityId);
    if (rank == 0) 
        return rank;

    const Entry<T>& entry = mLeaderboardData.getEntryByRank(rank);
    return getTiedRank(rank, entry);
}

/*************************************************************************************************/
/*!
    \brief getTiedRank

    Gets tied rank for the linear one

    \param[in]  rank - linear rank
    \param[in]  entry - reference to the ranking table Entry
    \return - tied rank
*/
/*************************************************************************************************/
template <typename T>
int32_t LeaderboardIndex<T>::getTiedRank(int32_t rank, const Entry<T>& entry) const
{
    if (rank == 1)
        return 1; 

    // assuming most common case is no ties, 
    // if previous one is the same then we start full scale binary search
    const Entry<T>& prevEntry = mLeaderboardData.getEntryByRank(rank - 1);
    if (prevEntry.value != entry.value)
        return rank;
    
    int32_t start = 1;
    int32_t end = rank - 1;
    int32_t d; 
    bool eq;
    
    while (1)
    {
        d = end + start;
        d >>= 1;
    
        const Entry<T>& midEntry = mLeaderboardData.getEntryByRank(d);
        eq = midEntry.value == entry.value;
        if (end - start < 2) break;

        if (eq)
            end = d;
        else 
            start = d;
    }
        
    if (eq) 
        return start;
    return end;
}

/*************************************************************************************************/
/*!
    \brief getCenteredRows

    Fetch a sequence of rows from the leaderboard.  Will fetch the requested number of rows
    around the entity specified, or fewer if the leaderboard does not contain enough.
    If the entity cannot be found in the leaderboard, no rows are returned.

    \param[in]  centerId - the entity id to fetch rows around
    \param[in]  count - the max number of rows to fetch
    \param[in]  showAtBottomIfNotFound - if entity not found, show it at the bottom of the leaderboard
    \param[out] rows - list of rows to be filled in by this method
    \param[out] keys - list of entity ids to be filled in by this method

    \return - ERR_OK or appropriate error code
*/
/*************************************************************************************************/
template <typename T>
BlazeRpcError LeaderboardIndex<T>::getCenteredRows(EntityId centerId, int32_t count, bool showAtBottomIfNotFound,
    LeaderboardStatValues::RowList& rows, EntityIdList& keys) const
{
    if (mLeaderboardData.getCount() == 0)
        return ERR_OK;

    uint32_t entityRank = mLeaderboardData.getRank(centerId);
    int32_t rankTemp = entityRank;

    if (rankTemp == 0)
    {
        if (!showAtBottomIfNotFound)
        {
            TRACE_LOG("[LeaderboardIndexes].getCenteredRows() entity: " << centerId << " is not in the ranking table");
            return ERR_OK;
        }
        rankTemp = mLeaderboardData.getCount();
        --count;
    }

    int32_t size = mLeaderboardData.getCount();
    int32_t startRank = rankTemp - count/2;
    if (startRank < 1)
        startRank = 1;
    int32_t endRank = startRank + count - 1;
    if (endRank > size)
    {
        // offset start by how much end exceeds size
        startRank -= endRank - size;
        if (startRank < 1)
            startRank = 1;

        endRank = size;
    }

    int32_t displayRank = 0;
    T prevValue = 0;

    keys.reserve(endRank - startRank + 2);
    rows.reserve(endRank - startRank + 2);
    for (int32_t rank = startRank; rank <= endRank; ++rank)
    {
        const Entry<T>& entry = mLeaderboardData.getEntryByRank(rank);
        keys.push_back(entry.entityId);

        if (mLeaderboard->isTiedRank())
        {
            if (rank == startRank)
            {
                displayRank = getTiedRank(rank, entry);
                prevValue = entry.value;
            }
            else if (entry.value != prevValue)
            {
                displayRank = rank;
                prevValue = entry.value;
            }
        }
        else
        {
            displayRank = rank;
        }

        LeaderboardStatValuesRow* statRow = rows.pull_back();
        statRow->setRank(displayRank);
        statRow->getUser().setBlazeId(entry.entityId);
    }

    if (entityRank == 0)
    {
        keys.push_back(centerId);

        LeaderboardStatValuesRow* statRow = rows.pull_back();
        statRow->setRank(0);
        statRow->getUser().setBlazeId(centerId);
    }
    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief getCenteredRowsForUserSet

    Fetch a sequence of rows from the leaderboard filtered by the user set.  Will fetch the requested
    number of rows around the entity specified, or fewer if the user set does not contain enough.
    If the entity cannot be found in the user set, no rows are returned.

    \param[in]  centerId - the entity id to fetch rows around
    \param[in]  count - the max number of rows to fetch
    \param[in]  userSetId - id of user set to filter leaderboard on
    \param[out] rows - list of rows to be filled in by this method
    \param[out] keys - list of entity ids to be filled in by this method
    \param[out] sorted - whether the list of rows is sorted

    \return - ERR_OK or appropriate error code
*/
/*************************************************************************************************/
template <typename T>
BlazeRpcError LeaderboardIndex<T>::getCenteredRowsForUserSet(EntityId centerId, int32_t count, EA::TDF::ObjectId userSetId,
    LeaderboardStatValues::RowList& rows, EntityIdList& keys, bool& sorted) const
{
    BlazeRpcError error = ERR_OK;
    uint32_t uss = 0;
    error = gUserSetManager->countUsers(userSetId, uss);
    if (error != ERR_OK)
        return error;
    int32_t userSetSize = static_cast<int32_t>(uss);
    if (userSetSize == 0)
        return ERR_OK;

    // we want ranks relative to the user set, not overall/global ranking table;
    // so scan through the user set and sort it
    // and while scanning the user set, also check that the entity is in the user set
    // (expect that the entity is usually in the user set)
    bool inUserSet = false;
    RankedUserSetMap rankedUserSetMap;
    UnrankedUserSetList unrankedUserSetList;

    BlazeIdList ids;
    error = gUserSetManager->getUserBlazeIds(userSetId, ids);
    if (error != ERR_OK)
        return error;

    int32_t centerRank = 0;
    unrankedUserSetList.reserve(ids.size());
    for (BlazeIdList::const_iterator idItr = ids.begin(); idItr != ids.end(); ++idItr)
    {
        EntityId id = *idItr;
        uint32_t globalRank = mLeaderboardData.getRank(id);

        if (globalRank == 0)
        {
            unrankedUserSetList.push_back(id);

            // remember for later
            if (id == centerId)
            {
                centerRank = static_cast<int32_t>(unrankedUserSetList.size());
            }
        }
        else
        {
            rankedUserSetMap[globalRank] = id;
        }

        if (id == centerId)
            inUserSet = true;
    }

    if (!inUserSet)
    {
        TRACE_LOG("[LeaderboardIndexes].getCenteredRowsForUserSet() entity:: " << centerId << " is not in the user set");
        return ERR_OK;
    }

    int32_t rankedUserSetSize = static_cast<int32_t>(rankedUserSetMap.size());

    // find the entity's rank (relative to user set)
    if (mLeaderboardData.getRank(centerId) != 0)
    {
        // somewhere in our sorted users
        RankedUserSetMap::iterator rankedItr = rankedUserSetMap.begin();
        for (centerRank = 1; rankedItr->second != centerId; ++centerRank, ++rankedItr)
            ; // should not need to check for itr end here
    }
    else
    {
        centerRank += rankedUserSetSize;
    }

    // setup the range of ranks
    int32_t startRank = centerRank - count/2;
    if (startRank < 1)
        startRank = 1;
    int32_t endRank = startRank + count - 1;
    if (endRank > userSetSize)
        endRank = userSetSize;

    int32_t rank = 1;
    keys.reserve(endRank - startRank + 1);
    rows.reserve(endRank - startRank + 1);

    // fill the request from the sorted users
    if (startRank <= rankedUserSetSize)
    {
        // go to start rank
        RankedUserSetMap::iterator rankedItr = rankedUserSetMap.begin();
        RankedUserSetMap::iterator rankedEnd = rankedUserSetMap.end();
        for (; rank < startRank; ++rank, ++rankedItr)
            ; // with earlier rankedUserSetSize check, itr should never reach end here

        for (; rankedItr != rankedEnd && rank <= endRank; ++rankedItr, ++rank)
        {
            keys.push_back(rankedItr->second);
            LeaderboardStatValuesRow* statRow = rows.pull_back();
            statRow->setRank(rank);
            statRow->getUser().setBlazeId(rankedItr->second);
        }
    }
    else
    {
        // nothing from the sorted users
        rank += rankedUserSetSize;
    }

    // fill remaining request from the unranked users
    if (rank <= endRank)
    {
        // go to start rank (request may start in middle of unranked list)
        UnrankedUserSetList::iterator unrankedItr = unrankedUserSetList.begin();
        UnrankedUserSetList::iterator unrankedEnd = unrankedUserSetList.end();
        for (; rank < startRank; ++rank, ++unrankedItr)
            ; // with earlier userSetSize check, itr should never reach end here

        if (unrankedItr != unrankedEnd && rank <= endRank)
            sorted = false;

        for (; unrankedItr != unrankedEnd && rank <= endRank; ++unrankedItr, ++rank)
        {
            keys.push_back(*unrankedItr);
            LeaderboardStatValuesRow* statRow = rows.pull_back();
            statRow->setRank(0);
            statRow->getUser().setBlazeId(*unrankedItr);
        }
    }

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief getFilteredRows

    Fetch a collection of rows from the leaderboard.  Will fetch rows for each requested entity,
    whether it's in the leaderboard or not.

    \param[in]  ids - the entity ids to fetch
    \param[in]  limit - maximum count of lb results to return
    \param[out] rows - list of rows to be filled in by this method
    \param[out] keys - list of entity ids to be filled in by this method

    \return - ERR_OK or appropriate error code
*/
/*************************************************************************************************/
template <typename T>
BlazeRpcError LeaderboardIndex<T>::getFilteredRows(const EntityIdList& ids, uint32_t limit,
    LeaderboardStatValues::RowList& rows, EntityIdList& keys) const
{
    if (0 == limit)
    {
        ERR_LOG("[LeaderboardIndex].getFilteredRows():The limit can not be set as 0.");
        return ERR_SYSTEM;
    }

    EntityIdList::const_iterator entityIter = ids.begin();
    EntityIdList::const_iterator entityEnd = ids.end();

    uint32_t size = limit;
    if (ids.size() < limit)
    {
        size = ids.size();
    }
    keys.reserve(size);
    rows.reserve(size);
    

    if (mLeaderboardData.getCount() == 0)
    {
        for (; (entityIter!=entityEnd) && (limit>0); ++entityIter)
        {
            LeaderboardStatValuesRow* statRow = rows.pull_back();
            statRow->setRank(0);
            statRow->getUser().setBlazeId(*entityIter);
            keys.push_back(*entityIter);
            limit--;
        }
        return ERR_OK;
    }

    for ( ; (entityIter!=entityEnd) && (limit>0); ++entityIter )
    {
        uint32_t rank = mLeaderboardData.getRank(*entityIter);

        LeaderboardStatValuesRow* statRow = rows.pull_back();
        // if tied ranks are enabled then translate linear to tied
        if (rank != 0 && mLeaderboard->isTiedRank())
        {
            rank = getTiedRank(rank, mLeaderboardData.getEntryByRank(rank));
        }
        statRow->setRank(rank);
        
        statRow->getUser().setBlazeId(*entityIter);
        keys.push_back(*entityIter);
        limit--;
    }

    return ERR_OK;
}

} // Stats
} // Blaze
#endif 
