/*************************************************************************************************/
/*!
    \file   transformstats.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_TRANSFORMSTATS_H
#define BLAZE_STATS_TRANSFORMSTATS_H

/*** Include files *******************************************************************************/
#include "EASTL/vector_map.h"
#include "framework/database/dbresult.h"
#include "stats/statscommontypes.h"
#include "stats/tdf/stats_server.h"
#include "stats/statsprovider.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
// Forward declarations

namespace Stats
{
// Forward declarations

/**
 * UpdateRowKey - Used to search for a given UpdateRow within UpdateRowMap provided
 *                in the TransformStatsCb callback
 */
struct UpdateRowKey
{
public:
    UpdateRowKey();
    UpdateRowKey(const char8_t* category, EntityId entityId, 
        int32_t period, const Stats::ScopeNameValueMap* map);
    bool operator < (const UpdateRowKey& other) const;
    bool isDefaultScopeIndex(const RowScopesVector& rowScopes) const;

    const char8_t* category;
    EntityId entityId;
    int32_t period;
    const Stats::ScopeNameValueMap* scopeNameValueMap;
};

/**
 * CategoryUpdateRowKey - Very similar to UpdateRowKey, but contains no category.
 *                        It is used to key the collection of all updates for the
 *                        same entity, period, and scopes which need to be processed
 *                        together to handle derived stats across multiple categories.
 */
struct CategoryUpdateRowKey
{
public:
    CategoryUpdateRowKey();
    CategoryUpdateRowKey(EntityId entityId, int32_t period, const Stats::ScopeNameValueMap* map);
    bool operator < (const CategoryUpdateRowKey& other) const;

    EntityId entityId;
    int32_t period;
    const Stats::ScopeNameValueMap* scopeNameValueMap;
};

/**
* UpdateAggregateRowKey - Used to search for a given StatValMap within UpdateAggregateRowMap
*/
typedef enum { AGG_NEW, AGG_INSERT, AGG_UPDATE } AggStatus;

struct UpdateAggregateRowKey
{
public:
    UpdateAggregateRowKey(){}
    UpdateAggregateRowKey(const char8_t* category, EntityId entityId, 
                          int32_t period,
                          const RowScopesVector& rowScopesVector);
    bool operator < (const UpdateAggregateRowKey& other) const;

    const char8_t* mCategory;
    EntityId mEntityId;
    int32_t mPeriod;
    AggStatus mStatus;
    RowScopesVector mRowScopesVector;
};

/**
 * CategoryUpdateAggregateRowKey - Analogous to CategoryUpdateRowKey but used for
 *                                 aggregate rows.
 */
struct CategoryUpdateAggregateRowKey
{
public:
    CategoryUpdateAggregateRowKey() {}
    CategoryUpdateAggregateRowKey(EntityId entityId, int32_t period,
                                  const RowScopesVector& rowScopesVector);
    bool operator < (const CategoryUpdateAggregateRowKey& other) const;

    EntityId mEntityId;
    int32_t mPeriod;
    RowScopesVector mRowScopesVector;
};

class StatsProviderRow;
class StatsProviderResultSet;

/**
 * UpdateRow - This structure corresponds to a single category and period specific set of stats.
 *             With sharding, this class is now only used on the shard as a temporary data holder.
 *             The setValue functions are used when the data comes from the updateStatsHelper via commitStats.
 */
class UpdateRow
{
    friend class UpdateHelper;
    friend class CommitStatsTransactionCommand;
    friend class StatsTransactionContext;
public:
    ~UpdateRow();
    
    bool setValueInt(const char8_t* column, int64_t value);
    bool setValueFloat(const char8_t* column, float_t value);
    bool setValueString(const char8_t* column, const char8_t* value);

    StatValMap& getNewStatValMap() { return mNewStatValMap; }
    const StatValMap& getNewStatValMap() const { return mNewStatValMap; }

private:
    UpdateRow()
        : mHelper(nullptr), mCategory(nullptr), mScopes(nullptr), mUpdatesList(nullptr), 
            mScopeNameValueMap(nullptr), mProviderResultSet(nullptr) {}
    UpdateRow(class UpdateHelper* helper, const StatCategory* category,
        ScopesVectorForRows* scopes, StatRowUpdate::StatUpdateList* updates, const Stats::ScopeNameValueMap* scopeNameValueMap)
        : mHelper(helper), mCategory(category), mScopes(scopes), mUpdatesList(updates), 
            mScopeNameValueMap(scopeNameValueMap), mProviderResultSet(nullptr) {}
    const StatCategory* getCategory() const { return mCategory; }
    const ScopesVectorForRows* getScopes() const { return mScopes; }
    const StatRowUpdate::StatUpdateList* getUpdates() const { return mUpdatesList; }
    
    const StatsProviderRow* getProviderRow() const;
    void setProviderResultSet(const StatsProviderResultSet* providerResultSet) { mProviderResultSet = providerResultSet; }
    const StatsProviderResultSet* getProviderResultSet() const { return mProviderResultSet; }

    StatPtrValMap& getModifiedStatValueMap() { return mModifiedStatValueMap; }
    const StatPtrValMap& getModifiedStatValueMap() const { return mModifiedStatValueMap; }

private:
    class UpdateHelper* mHelper;
    const StatCategory* mCategory;                            // category name is stored in the UpdateRowKey, but we cache the pointer for faster lookup
    ScopesVectorForRows* mScopes;
    StatRowUpdate::StatUpdateList* mUpdatesList;        // the original set of updates to apply. Applied when evaluateStats is called
    StatValMap mNewStatValMap;                          // Working set of stats. Starts out as defaults or DB values, is modified with setValue* and evaluateStats
    StatPtrValMap mModifiedStatValueMap;                // vales to write to the DB. Set by the call to evaluateStats and setValue* functions. 
    const Stats::ScopeNameValueMap* mScopeNameValueMap;

    // link to data in stats provider
    const StatsProviderResultSet* mProviderResultSet;
    mutable StatsProviderRowPtr mProviderRow;           // Original stats from the DB (may be empty if no entry existed)

    StringStatValueList mStrings;
};

typedef eastl::vector<EA::TDF::tdf_ptr<StatRowUpdate::StatUpdateList> > UpdatesTable;
typedef eastl::vector<ScopesVectorForRows*> ScopesTable;
typedef eastl::vector_map<UpdateRowKey, UpdateRow> UpdateRowMap;
typedef eastl::vector_map<UpdateAggregateRowKey, StatValMap> UpdateAggregateRowMap;
typedef eastl::vector_map<UpdateRowKey, Stats::StatRowUpdate*> StatRowUpdateMap;

typedef eastl::hash_set<const CategoryDependency*> DependencySet;
typedef eastl::vector_map<CategoryUpdateRowKey, DependencySet> CategoryUpdateRowMap;
typedef eastl::vector_map<CategoryUpdateAggregateRowKey, DependencySet> CategoryUpdateAggregateRowMap;

/**
 * UpdateHelper - This data structure is used to build and track an indexable database stats snapshot 
 *                while inside the call to StatsSlaveImpl::updateStats().
 *                This allows TransformStatsCb callback to be called from within StatsSlaveImpl::updateStats()
 *                and enables the caller to 'massage' the stats data retrieved and transactionally locked 
 *                by the DB, before it is subsequently written back out into said DB as part of an UPDATE query
 *                which occurs at the end of StatsSlaveImpl::updateStats().
 */
class UpdateHelper
{
public:
    ~UpdateHelper() { clear(); }
    void clear();
    void addUpdates(StatRowUpdate::StatUpdateList* updates) { mUpdatesTable.push_back(updates); }
    void buildRow(const StatCategory* category, StatRowUpdate* update, int32_t period, ScopesVectorForRows* scopes);
    UpdateRowMap* getMap() { return &mUpdateRowMap; }
    CategoryUpdateRowMap* getCategoryMap() { return &mCategoryUpdateRowMap; }
    UpdateAggregateRowMap::iterator getStatValMapIter(const UpdateRowKey& updateRowKey, const RowScopesVector& rowScopesVector, const StatCategory* category);
    UpdateAggregateRowMap* getStatValMaps() {return &mUpdateAggregateRowMap;}
    CategoryUpdateAggregateRowMap* getCategoryAggregateMap() { return &mCategoryUpdateAggregateRowMap; }
private:
    UpdatesTable mUpdatesTable;
    UpdateRowMap mUpdateRowMap;
    CategoryUpdateRowMap mCategoryUpdateRowMap;
    UpdateAggregateRowMap mUpdateAggregateRowMap; // a map holding cached(current) statname/value map for an aggregate
                                                  // row or total row in the table
    CategoryUpdateAggregateRowMap mCategoryUpdateAggregateRowMap;
};

} // Stats
} // Blaze
#endif // BLAZE_STATS_TRANSFORMSTATS_H
