/*************************************************************************************************/
/*!
    \file   statsbootstrap.h


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_STATS_BOOTSTRAP_H
#define BLAZE_STATS_BOOTSTRAP_H

/*** Include files *******************************************************************************/

#include "statscommontypes.h"
#include "stats/leaderboardindex.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
namespace Blaze
{
// Forward declarations
//class Event;
class DbConn;
class DbEvent;
class Query;

namespace Stats
{
// Forward declarations
class StatsMasterImpl;
class StatCategory;
class StatsConfigData;

class StatsBootstrap //: public EventHandler
{
public:

    StatsBootstrap(StatsMasterImpl* statsMasterImpl, const StatsConfig* configFile, const bool allowDestruct);
    virtual ~StatsBootstrap();

    bool configure(StatsConfigData& statsConfig);
    bool run(StatsConfigData& statsConfig, LeaderboardIndexes& lbIndexes);

private:

    typedef eastl::list<const StatCategory*> CategoryList;
    typedef eastl::hash_map<uint32_t, CategoryList> CategoryShardMap;

    struct OwnedStringSet : public StringSet
    {
        virtual ~OwnedStringSet()
        {
            for (StringSetIter itr = begin(); itr != end(); ++itr)
            {
                delete[] *itr;
            }
        }
    };

    typedef eastl::hash_map<const char8_t*, StringSet, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > TableColumnsMap;

    struct TableData
    {
        StatMap* statMap;
        ScopeNameSet* scopeNameSet;
        const ScopeNameSet* backupScopeNameSet;
        const StatCategory* statCategory;
        int32_t periodType;
        // Has in new table
        bool needPeriod;
        // Has in current, one to be potentially altered
        bool hasPrimary; // It always should have, but as a result of some incomplete operation it may not
        bool hasEntityKey;
        bool hasSecondry;
        bool hasPeriod;
        bool hasScopes;
        bool primaryDroped;
        StringSet oldScopes;
        eastl::vector<const char8_t*> primaryKey;
        eastl::vector<const char8_t*> newPrimaryKey;
        
        TableData()
        {
            hasSecondry = false;
            hasEntityKey = false;
            primaryDroped = false;
            hasPrimary = false;
            hasScopes = false;
            hasPeriod = false;
            needPeriod = false;
            
            statCategory = nullptr;
            periodType = STAT_PERIOD_ALL_TIME;
            statMap = nullptr;
            scopeNameSet = nullptr;
            backupScopeNameSet = nullptr;
        }

        ~TableData()
        {
            if (scopeNameSet != nullptr) delete scopeNameSet;
            StringSet::iterator strIt = oldScopes.begin();
            StringSet::iterator strEnd = oldScopes.end();
            for (; strIt != strEnd; ++strIt)
            {
                BLAZE_FREE((char8_t*)(*strIt));
            }
        }

        void init(const StatCategory* category, int32_t pType)
        {
            needPeriod = pType != STAT_PERIOD_ALL_TIME;
            
            statCategory = category;
            periodType = pType;
            statMap = BLAZE_NEW StatMap(*category->getStatsMap());

            const ScopeNameSet* scnSet = category->getScopeNameSet();
            if (scnSet != nullptr && scnSet->size() != 0)
            {
                scopeNameSet = BLAZE_NEW ScopeNameSet(*scnSet);
                backupScopeNameSet = scnSet;
            }
        }

        inline void insertKey(eastl::vector<const char8_t*>& vec, size_t pos, const char8_t* columnName)
        {
            if( vec.size() < pos) vec.resize(pos);
            vec[pos - 1] = columnName;
        } 
    };

    typedef eastl::hash_map<const char8_t*, TableData, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > TableMap;

    BlazeRpcError doDbBootStrap(const StatsConfigData* statsConfig, LeaderboardIndexes* lbIndexes);
    BlazeRpcError bootstrapCategoryTables(uint32_t dbId, const char8_t* dbName, DbConnPtr conn, const StatsConfigData* statsConfig, const CategoryList& categories);
    BlazeRpcError bootstrapLbIndexTables(uint32_t dbId, const char8_t* dbName, DbConnPtr conn, const StatsConfigData* statsConfig, LeaderboardIndexes* lbIndexes);
    BlazeRpcError bootstrapShardTables(uint32_t dbId, const char8_t* dbName, DbConnPtr conn, const StatsConfigData* statsConfig);
    BlazeRpcError fetchTableNames(uint32_t dbId, const char8_t* dbName, DbConnPtr conn, OwnedStringSet& tableNames);
    bool addTable(DbConnPtr conn, const StatsConfigData& statsConfig, const StatCategory *category, ColumnSet& statColumns, const char8_t *tableName, int32_t periodType);
    bool fetchAndUpdateColumns(DbConnPtr conn, const char8_t* dbName, const StatsConfigData& statsConfig, TableMap& existingTableMap);
    bool fetchAndUpdatePartitions(DbConnPtr conn, const char8_t* dbName, TableMap& existingTableMap);
    bool fetchAndUpdatePartitionsForTable(DbConnPtr conn, const char8_t *dbName, const char8_t *tableName, int32_t periodType);
    bool isLbIndexTableValid(DbConnPtr conn, const char8_t* lbTableName, const TableInfo& tableInfo) const;

    StatsMasterImpl* mStatsMasterImpl;
    const StatsConfig* mConfigFile;

    bool mAllowDestructiveActions;

    StringSet mExistingTableNames;
};

} // Stats
} // Blaze
#endif // BLAZE_STATS_BOOTSTRAP_H
