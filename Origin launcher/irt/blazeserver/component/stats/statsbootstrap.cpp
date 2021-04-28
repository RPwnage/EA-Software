/*************************************************************************************************/
/*!
    \file   statsbootstrap.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class StatsBootstrap

    Performs all the bootstrapping of the stat master on startup.  Upon startup the stats and
    leaderboard component needs to initialize a variety of structures to build an understanding
    of what stats are tracked, what leaderboards are built upon these stats, what meta-data
    describes these stats, and what data needs to be cached.  This data is all based upon what is
    read in from a configuration file.  In order to successfully communicate with the database to
    fetch and persist stats data, the stats and leaderboard component expects the database table
    structure to mirror the configuration file.  As game teams will often make additions or
    changes to the configuration, the task of this bootstrap class is to dynamically bring the
    database into line every time the server starts up, so that the database structure mirrors
    the server's understanding of how stats are persisted.

    The first stage of the bootstrapping process is to look for missing database tables.  If a
    new stat category is added to the configuration file, corresponding database tables will
    need to be created for it on server startup.  Conversely, existing database tables which are
    no longer required because a stat category has been removed from the config file will need
    to be dropped.

    The second stage of the bootstrapping process is to check existing database tables for
    consistency.  New stat columns may need to be added, obsolete ones may need to be dropped,
    and existing columns may need to be modified.

    Initially this class will attempt to perform all modifications to the database that are
    required in order to mirror the configuration file for development purposes.  Upon
    delivery to customers however, restrictions will need to be added such that any
    potentially destructive actions (dropping tables, deleting columns, modifiying columns in
    any fashion that may negatively affect row data) will require the server to be started up
    with a special command line flag indicating that destructive actions are expected.
    Otherwise, the stats component will simply not startup - this will prevent the accidental
    loss of precious data.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "statscomponentinterface.h"
#include "statscommontypes.h"
#include "statsbootstrap.h"
#include "statsmasterimpl.h"
#include "statsconfig.h"

#include "framework/util/shared/blazestring.h"


namespace Blaze
{
namespace Stats
{

// index key name for stats table
const char8_t* DB_INDEX_KEY_NAME = "ix_nonentity_id"; // everything in primary key except entity_id
const char8_t* DB_ENTITY_KEY_NAME = "ix_entity_id"; // not wanted anymore and will be dropped if exists

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief StatsBootstrap

    Constructs a bootstrap object that is used to do all the initial setup on server startup.

    \param[in]  statsMasterImpl - pointer to the stats master component
    \param[in]  configFile - pointer to the stats specific configuration data
*/
/*************************************************************************************************/
StatsBootstrap::StatsBootstrap(StatsMasterImpl* statsMasterImpl, const StatsConfig* configFile, const bool allowDestruct) :
    mStatsMasterImpl(statsMasterImpl), mConfigFile(configFile), mAllowDestructiveActions(allowDestruct)
{
    TRACE_LOG("[StatsBootstrap].StatsBootstrap(): Constructing the Stats bootstrap object");

    if (mAllowDestructiveActions == true)
    {
        INFO_LOG("[StatsBootstrap].StatsBootstrap(): Stats bootstrap set to allow destructive actions on DB.");
    }
}

/*************************************************************************************************/
/*!
    \brief ~StatsBootstrap

    Destroys a bootstrap object.
*/
/*************************************************************************************************/
StatsBootstrap::~StatsBootstrap()
{
    TRACE_LOG("[StatsBootstrap].~StatsBootstrap(): Destroying the Stats bootstrap object");
}

/* Private methods *******************************************************************************/    

/*************************************************************************************************/
/*!
    \brief configure

    Delegates to a StatsConfigData object to do all the config file parsing
    \return true if the operation completed successfully, false otherwise
*/
/*************************************************************************************************/
bool StatsBootstrap::configure(StatsConfigData& statsConfig)
{
    INFO_LOG("[StatsBootstrap].configure(): Starting bootstrap process, parsing configuration");

    if (!statsConfig.parseStatsConfig(*mConfigFile))
    {
        ERR_LOG("[StatsBootstrap].configure(): Bootstrap failed in configuration parsing, stats will not start");       
        return false;
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief run

    First method that gets called to kick off bootstrapping.  Ensures that the database tables for 
    stats and leaderboards exist and match the configuration and updates them if necessary.  Loads and
    writes out the leaderboard index database.
*/
/*************************************************************************************************/
bool StatsBootstrap::run(StatsConfigData& statsConfig, LeaderboardIndexes& lbIndexes)
{
    return (doDbBootStrap(&statsConfig, &lbIndexes) == Blaze::ERR_OK);
}

BlazeRpcError StatsBootstrap::doDbBootStrap(const StatsConfigData* statsConfig, LeaderboardIndexes* lbIndexes)
{
    INFO_LOG("[StatsBootstrap].doDbBootStrap(): Starting");
    BlazeRpcError err = ERR_OK;
    uint32_t mainDbId = mStatsMasterImpl->getDbId();

    // For each category we know the entity type and thus the shards we need to hit,
    // need to get the inverse here to make things efficient, for each sharded database,
    // find all the categories that need to be in that shard, so we can do everything
    // in that shard at once
    CategoryShardMap map;
    const CategoryMap* categoryMap = mStatsMasterImpl->getConfigData()->getCategoryMap();
    CategoryMap::const_iterator iter = categoryMap->begin();
    CategoryMap::const_iterator end = categoryMap->end();
    for (; iter != end; ++iter)
    {
        const StatCategory* cat = iter->second;
        const DbIdList& list = statsConfig->getShardConfiguration().getDbIds(cat->getCategoryEntityType());
        for (DbIdList::const_iterator dbIdItr = list.begin(); dbIdItr != list.end(); ++dbIdItr)
        {
            map[*dbIdItr].push_back(cat);
        }
    }

    // Now go hit each shard and do the main stats DB tables
    for (CategoryShardMap::const_iterator it = map.begin(); it != map.end(); ++it)
    {
        uint32_t dbId = it->first;
        const char8_t *dbName = gDbScheduler->getConfig(dbId)->getDatabase();
        DbConnPtr conn = gDbScheduler->getConnPtr(dbId);
        if (conn == nullptr)
        {
            ERR_LOG("[StatsBootstrap].doBootStrap(): Failed to connect to database, cannot continue");
            return Blaze::ERR_SYSTEM;
        }
        err = bootstrapCategoryTables(dbId, dbName, conn, statsConfig, it->second);
        if (err != ERR_OK)
            return err;
    }

    // Note that leaderboard index tables are not sharded, and also the main table that store shard
    // assignments of course are not shard, so we bootstrap them separately in the main DB
    const char8_t* dbName = gDbScheduler->getConfig(mainDbId)->getDatabase();
    {
        DbConnPtr conn = gDbScheduler->getConnPtr(mainDbId);
        if (conn == nullptr)
        {
            ERR_LOG("[StatsBootstrap].doBootStrap(): Failed to connect to database (to load & build LB index tables), cannot continue");
            return Blaze::ERR_SYSTEM;
        }
        err = bootstrapLbIndexTables(mainDbId, dbName, conn, statsConfig, lbIndexes);
        if (err != ERR_OK)
            return err;

        err = bootstrapShardTables(mainDbId, dbName, conn, statsConfig);
        if (err != ERR_OK && err != ERR_DB_DISCONNECTED)
            return err;
    }
    /// @hack to mitigate GOS-31182 where blaze loses connection to Galera cluster after large fetch.  Don't know why the connection is lost, but the mitigation is to retry with a new DB conn.
    if (err != ERR_OK)
    {
        WARN_LOG("[StatsBootstrap].doBootStrap(): Lost connection to database--will attempt to retry 'bootstrapShardTables' with new connection");
        DbConnPtr conn = gDbScheduler->getConnPtr(mainDbId);
        if (conn == nullptr)
        {
            ERR_LOG("[StatsBootstrap].doBootStrap(): Failed to connect to database (to load & build shard tables), cannot continue");
            return Blaze::ERR_SYSTEM;
        }
        err = bootstrapShardTables(mainDbId, dbName, conn, statsConfig);
        if (err != ERR_OK)
            return err;
    }
    return Blaze::ERR_OK;
}

BlazeRpcError StatsBootstrap::fetchTableNames(uint32_t dbId, const char8_t* dbName, DbConnPtr conn, OwnedStringSet& tableNames)
{
    // Request all the table names from the database
    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    query->append("SHOW TABLES FROM `$s`", dbName);

    DbResultPtr tableListResult;
    DbError tableListError = conn->executeQuery(query, tableListResult);

    if (tableListError != DB_ERR_OK)
    {
        ERR_LOG("[StatsBootstrap].fetchTableNames(): table list query failed: error = " << ErrorHelp::getErrorName(tableListError));
        return tableListError;
    }
    
    INFO_LOG("[StatsBootstrap].fetchTableNames(): Found " << tableListResult->size() << " tables in the DB");
    DbResult::const_iterator dbItr = tableListResult->begin();
    DbResult::const_iterator dbEnd = tableListResult->end();
    for (; dbItr != dbEnd; ++dbItr)
    {
        char8_t* tableName = BLAZE_NEW_ARRAY(char8_t, 64);
        blaze_strnzcpy(tableName, (*dbItr)->getString((uint32_t)0), 64);
        tableNames.insert(tableName);
    }

    return Blaze::ERR_OK;
}  

BlazeRpcError StatsBootstrap::bootstrapCategoryTables(uint32_t dbId, const char8_t* dbName, DbConnPtr conn, const StatsConfigData* statsConfig, const CategoryList& categories)
{
    BlazeRpcError err = ERR_OK;

    INFO_LOG("[StatsBootstrap].gotTableList(): Comparing configuration to database");

    OwnedStringSet tableNames;
    err = fetchTableNames(dbId, dbName, conn, tableNames);
    if (err != ERR_OK)
        return err;

    TableMap existingTableMap;
    CategoryList::const_iterator iter = categories.begin();
    CategoryList::const_iterator end = categories.end();
    for (; iter != end; ++iter)
    {
        const StatCategory* category = *iter;

        for (int32_t i = 0; i < STAT_NUM_PERIODS; ++i)
        {
            const char8_t* tableName = category->getTableName(i);
            if (*tableName == '\0')
                continue;

            if (tableNames.find(tableName) != tableNames.end())
            {
                INFO_LOG("[StatsBootstrap].gotTableList(): Configured table " << tableName << " was found in DB");
                //Since we found the table, fetch its columns - we do all the columns at once so just save
                //the name off to build a table later.
                existingTableMap[tableName].init(category, i);

                // In a multi-sharded case, we might occasionally have a case where a stats table exists
                // in some but not all shards, as long as it exists in at least one, it will get added
                // to this set, and that serves as the criteria to decide whether to rebuild the corresponding
                // lb table (if any) from scratch
                mExistingTableNames.insert(tableName);
            }
            else
            {
                INFO_LOG("[StatsBootstrap].gotTableList(): Configured table " << tableName << " was not found in DB");

                //create a list of stat colums that need to be included in the new table.
                ColumnSet categorySet;
                StatMap::const_iterator statIter = category->getStatsMap()->begin();
                StatMap::const_iterator statEnd = category->getStatsMap()->end();
                for (; statIter != statEnd; ++statIter)
                {
                    categorySet.insert(statIter->first);
                }

                if (!addTable(conn, *statsConfig, category, categorySet, tableName, i))
                {
                    //Error adding the table, bail
                    return Blaze::ERR_SYSTEM;
                }
                categorySet.clear();
            }
        }
    }

    //Now fetch any existing tables if we need them.
    if (existingTableMap.size() > 0)
    {    
        if (!fetchAndUpdateColumns(conn, dbName, *statsConfig, existingTableMap))
        {
            return Blaze::ERR_SYSTEM;
        }

        if (!fetchAndUpdatePartitions(conn, dbName, existingTableMap))
        {
            return Blaze::ERR_SYSTEM;
        }
    }
    return Blaze::ERR_OK;
}

BlazeRpcError StatsBootstrap::bootstrapLbIndexTables(uint32_t dbId, const char8_t* dbName, DbConnPtr conn, const StatsConfigData* statsConfig, LeaderboardIndexes* lbIndexes)
{
    BlazeRpcError err = ERR_OK;

    const CategoryMap* categoryMap = mStatsMasterImpl->getConfigData()->getCategoryMap();

    OwnedStringSet tableNames;
    err = fetchTableNames(dbId, dbName, conn, tableNames);
    if (err != ERR_OK)
        return err;

    //add the leaderboards index tables
    TableInfoMap& statTablesForLb = lbIndexes->getStatTablesForLeaderboards();

    //iterate the list of stats tables that we need leaderboards for and create a corresponding leaderboards table.
    TableInfoMap::iterator tableItr = statTablesForLb.begin();
    TableInfoMap::iterator tableEnd = statTablesForLb.end();

    for (; tableItr != tableEnd; ++tableItr)
    {
        TableInfo& tableInfo = tableItr->second;
        const char8_t* statTableName = tableItr->first;
        CategoryMap::const_iterator categoryItr = categoryMap->find(tableInfo.categoryName);
        if (categoryItr == categoryMap->end())
        {
            ERR_LOG("[StatsBootstrap].doDbBootStrap(): Leaderboard is defined with unknown stat category " << statTableName);
            return Blaze::ERR_SYSTEM;
        }
        
        char8_t lbTableName[LB_MAX_TABLE_NAME_LENGTH];
        LeaderboardIndexes::getLeaderboardTableName(statTableName, lbTableName, LB_MAX_TABLE_NAME_LENGTH);

        bool createLbIndexTable = true;
        if (tableNames.find(lbTableName) != tableNames.end())
        {
            createLbIndexTable = false;
            TRACE_LOG("[StatsBootstrap].doDbBootStrap(): leaderboard index table found (" << lbTableName << ")");
            bool isNewStatsTable = (mExistingTableNames.find(statTableName) == mExistingTableNames.end());

            if (isNewStatsTable || !isLbIndexTableValid(conn, lbTableName, tableInfo))
            {
                //configuration has changed, so drop the lb index table so that we can rebuild it.
                INFO_LOG("[StatsBootstrap].doDbBootStrap(): leaderboard index table (" << lbTableName << ") is out of date and is being rebuilt.");
                QueryPtr dropQuery = DB_NEW_QUERY_PTR(conn);
                dropQuery->append("DROP TABLE `$s`", lbTableName);

                DbResultPtr dropResult;
                DbError dropError = conn->executeQuery(dropQuery, dropResult);

                if (dropError != DB_ERR_OK)
                {
                    ERR_LOG("[StatsBootstrap].doDbBootStrap(): Unable to drop leaderboard table " << lbTableName);
                    return ERR_SYSTEM;
                }
                createLbIndexTable = true;
            }
            else if (tableInfo.periodType != STAT_PERIOD_ALL_TIME)
            {
                if (!fetchAndUpdatePartitionsForTable(conn, dbName, lbTableName, tableInfo.periodType))
                {
                    ERR_LOG("[StatsBootstrap].doDbBootStrap(): Unable to update partitions for leaderboard table " << lbTableName);
                    return ERR_SYSTEM;
                }
            }
        }
        if (createLbIndexTable)
        {
            TRACE_LOG("[StatsBootstrap].doDbBootStrap(): creating leaderboard index table (" << lbTableName << ")");
            tableInfo.loadFromDbTables = false;
            if (!addTable(conn, *statsConfig, categoryItr->second, tableInfo.statColumns, lbTableName, tableInfo.periodType))
            {
                ERR_LOG("[StatsBootstrap].doDbBootStrap(): Error creating leaderboard index table (" << lbTableName << ")");
                return Blaze::ERR_SYSTEM;
            }

            // add partitions for any historical periods
            if (tableInfo.periodType != STAT_PERIOD_ALL_TIME)
            {
                QueryPtr query = DB_NEW_QUERY_PTR(conn);
                query->append("SELECT PARTITION_NAME,PARTITION_DESCRIPTION FROM INFORMATION_SCHEMA.PARTITIONS"
                    " WHERE TABLE_SCHEMA LIKE '$L'"
                    " AND PARTITION_DESCRIPTION IS NOT NULL"
                    " AND TABLE_NAME = '$s';", dbName, statTableName);

                DbResultPtr dbResult;
                DbError dbError = conn->executeQuery(query, dbResult);
                if (dbError != DB_ERR_OK)
                {
                    ERR_LOG("[StatsBootstrap].doDbBootStrap(): error getting periods for table " << statTableName << ": " << getDbErrorString(dbError));
                    return Blaze::ERR_SYSTEM;
                }

                int32_t currentPeriodId = mStatsMasterImpl->getPeriodId(tableInfo.periodType);
                PeriodIdList existingPeriods;
                DbResult::const_iterator rowItr = dbResult->begin();
                DbResult::const_iterator rowEnd = dbResult->end();
                for (; rowItr != rowEnd; ++rowItr)
                {
                    const DbRow *row = *rowItr;
                    int32_t periodId = row->getInt("PARTITION_DESCRIPTION");
                    // any period >= current already should have partition created in addTable()
                    if (periodId < currentPeriodId)
                    {
                        existingPeriods.push_back(periodId);
                    }
                }

                if (!existingPeriods.empty())
                {
                    query = DB_NEW_QUERY_PTR(conn);
                    query->append("ALTER TABLE `$s` ADD PARTITION (", lbTableName);
                    mStatsMasterImpl->mRollover.addExistingPeriodIdsToQuery(query, lbTableName, existingPeriods,
                        tableInfo.periodType);

                    query->trim(1);
                    query->append("\n);");

                    dbError = conn->executeQuery(query);
                    if (dbError != DB_ERR_OK)
                    {
                        ERR_LOG("[StatsBootstrap].doDbBootStrap(): failed to create partitions for table " << lbTableName << ": " 
                                << getDbErrorString(dbError));
                        return Blaze::ERR_SYSTEM;
                    }
                }
            } // if adding partitions for historical periods
        }
    }

    if (lbIndexes->loadData(statTablesForLb) == Blaze::ERR_OK)
    {
        TRACE_LOG("[StatsBootstrap].onConfigure(): leaderboard index was loaded successfully");
    }
    else
    {
        ERR_LOG("[StatsBootstrap].onConfigure(): leaderboard index failed to load");
        return Blaze::ERR_SYSTEM;
    }
    //now that the leaderboards have been loaded from the stats tables in the db, write out to the corresponding leaderboard tables in the db
    lbIndexes->writeIndexesToDb(conn, statTablesForLb);

    return Blaze::ERR_OK;
}

BlazeRpcError StatsBootstrap::bootstrapShardTables(uint32_t dbId, const char8_t* dbName, DbConnPtr conn, const StatsConfigData* statsConfig)
{
    BlazeRpcError err = ERR_OK;

    OwnedStringSet tableNames;
    err = fetchTableNames(dbId, dbName, conn, tableNames);
    if (err != ERR_OK)
        return err;

    // Fairly simple, if we don't find a shard table for a given entity type, create it
    ShardMap::const_iterator itr = statsConfig->getShardConfiguration().getShardMap().begin();
    ShardMap::const_iterator end = statsConfig->getShardConfiguration().getShardMap().end();
    for (; itr != end; ++itr)
    {
        if (tableNames.find(itr->second.getTableName()) == tableNames.end())
        {
            QueryPtr query = DB_NEW_QUERY_PTR(conn);
            query->append("CREATE TABLE `$s` (\n", itr->second.getTableName());
            query->append("`entity_id` bigint(20) unsigned NOT NULL,\n");
            query->append("`shard` int(10) unsigned NOT NULL,\n");
            query->append("PRIMARY KEY (`entity_id`)\n");
            query->append(") ENGINE=InnoDB DEFAULT CHARSET=utf8 ");
            query->append("COMMENT='Blaze stats shard table';\n");
            err = conn->executeQuery(query);
            if (err != DB_ERR_OK)
            {
                ERR_LOG("[StatsBootstrap].bootstrapShardTables(): Failure adding table, cannot continue");   
                return ERR_SYSTEM;
            }
        }
    }

    return ERR_OK;
}

/*************************************************************************************************/
/*!
    \brief isLbIndexTableValid

    Checks if a leaderboard index table contains all of the appropriate columns and keys based on
    the configuration
    
    \param[in] - conn - database connection to use.  Owned by calling function, so don't release it.
    \param[in] - lbTableName - the name of the leaderboard index table
    \param[in] - tableInfo - information about the tables expected columns and indexes

    \return - true if the leaderboard index table is up to date, false if it doesn't match the configuration.
*/
/*************************************************************************************************/
bool StatsBootstrap::isLbIndexTableValid(DbConnPtr conn, const char8_t* lbTableName, const TableInfo& tableInfo) const
{
    //TODO: fetchAndUpdateColumns should be refactored with this method to pull out some common code/methods.

    //fetch a list of columns and make sure that all the required columns exist in the leaderboard index table.
    QueryPtr columnQuery = DB_NEW_QUERY_PTR(conn);
    columnQuery->append("SHOW COLUMNS FROM `$s`", lbTableName);
    DbResultPtr columnResult;
    DbError error = conn->executeQuery(columnQuery, columnResult);

    if (error != DB_ERR_OK)
    {
        ERR_LOG("[StatsBootstrap].isLbIndexTableValid() Failed to show columns for table: (" 
                << lbTableName << ") error = " << getDbErrorString(error));
        return false;
    }

    ColumnSet columnSet;
    if (columnResult->empty())
    {
        TRACE_LOG("[StatsBootstrap].isLbIndexTableValid() table " << lbTableName << " has no columns");
        return false;
    }
    else
    {
        DbResult::const_iterator itr = columnResult->begin();
        DbResult::const_iterator end = columnResult->end();
        for (; itr != end; ++itr)
        {
            const DbRow* row = *itr;
            columnSet.insert(row->getString("Field"));
        }

        if (columnSet.find("entity_id") == columnSet.end())
        {
            TRACE_LOG("[StatsBootstrap].isLbIndexTableValid() table " << lbTableName << " is missing entity_id column");
            return false;
        }

        if (tableInfo.periodType != STAT_PERIOD_ALL_TIME && columnSet.find("period_id") == columnSet.end())
        {
            TRACE_LOG("[StatsBootstrap].isLbIndexTableValid() table " << lbTableName << " is missing period_id column");
            return false;
        }

        //check keyscope columns
        if (tableInfo.keyScopeColumns != nullptr)
        {
            ScopeNameSet::const_iterator scopeItr = tableInfo.keyScopeColumns->begin();
            ScopeNameSet::const_iterator scopeEnd = tableInfo.keyScopeColumns->end();
            for (; scopeItr != scopeEnd; ++scopeItr)
            {
                const char8_t* keyscopeColumn = (*scopeItr).c_str();
                if (columnSet.find(keyscopeColumn) == columnSet.end())
                {
                    TRACE_LOG("[StatsBootstrap].isLbCacheTableValid() table " << lbTableName << " is missing keyscope column "
                              << keyscopeColumn);
                    return false;
                }
            }
        }

        //check stat columns
        ColumnSet::const_iterator statColIter = tableInfo.statColumns.begin();
        ColumnSet::const_iterator statColEnd = tableInfo.statColumns.end();
        for(; statColIter != statColEnd; ++statColIter)
        {
            const char8_t* statColumn = (*statColIter).c_str();
            if (columnSet.find(statColumn) == columnSet.end())
            {
                TRACE_LOG("[StatsBootstrap].isLbIndexTableValid() table " << lbTableName << " is missing stat column " << statColumn);
                return false;
            }
        }
    }

    //check if the table has the correct primary key.
    QueryPtr indexQuery = DB_NEW_QUERY_PTR(conn);
    indexQuery->append("SHOW INDEX FROM `$s`;", lbTableName);
    DbResultPtr indexResult;
    DbError indexError = conn->executeQuery(indexQuery, indexResult);
    
    if (indexError != DB_ERR_OK)
    {
        ERR_LOG("[StatsBootstrap].isLbIndexTableValid() Failed to show indices for table: (" << lbTableName << ") error = " << getDbErrorString(error));
        return false;
    }

    bool hasPrimaryKey = false;
    bool hasPrimaryEntityKey = false;
    bool hasPrimaryPeriodKey = tableInfo.periodType == STAT_PERIOD_ALL_TIME;
    if (indexResult->empty())
    {
        TRACE_LOG("[StatsBootstrap].isLbIndexTableValid() table " << lbTableName << " has no key");
        return false;
    }
    else
    {
        DbResult::const_iterator indexItr = indexResult->begin();
        DbResult::const_iterator indexEnd = indexResult->end();
        ColumnSet scopeColumns;
        for (; indexItr != indexEnd; ++indexItr)
        {
            const DbRow* row = *indexItr;

            const char8_t* key_name = row->getString("Key_name");
            //we only need to check the primary key
            if (blaze_stricmp(key_name, "PRIMARY") == 0)
            {
                hasPrimaryKey = true;
                const char8_t* columnName = row->getString("Column_name");
                if (blaze_stricmp(columnName, "entity_id") == 0)
                {
                    hasPrimaryEntityKey = true;
                }
                if (blaze_stricmp(columnName, "period_id") == 0)
                {
                    hasPrimaryPeriodKey = true;
                }
                //keyscope columns
                if (blaze_stricmp(columnName, "entity_id") != 0 && 
                    blaze_stricmp(columnName, "period_id") != 0)
                {
                    scopeColumns.insert(columnName);
                } 
            }
        }

        if (!hasPrimaryEntityKey || !hasPrimaryPeriodKey)
        {
            TRACE_LOG("[StatsBootstrap].isLbIndexTableValid() table " << lbTableName << " has out of date primary key");
            return false;
        }
        if (hasPrimaryKey && (tableInfo.keyScopeColumns != nullptr))
        {
            ScopeNameSet::const_iterator scopeItr = tableInfo.keyScopeColumns->begin();
            ScopeNameSet::const_iterator scopeEnd = tableInfo.keyScopeColumns->end();
            for (; scopeItr != scopeEnd; ++scopeItr)
            {
                const char8_t* keyscopeColumn = (*scopeItr).c_str();
                if (scopeColumns.find(keyscopeColumn) == scopeColumns.end())
                {
                    TRACE_LOG("[StatsBootstrap].isLbIndexTableValid() table " << lbTableName << " is missing keyscope column " 
                              << keyscopeColumn << " from primary keys");
                    return false;
                }
            }
        }
    }

    return true;
}

/*************************************************************************************************/
/*!
    \brief addTable

    Adds a table that was configured in the config file, but found to be missing from the DB.

    \param[in]  ev - event from db request handler callback
*/
/*************************************************************************************************/
bool StatsBootstrap::addTable(DbConnPtr conn, const StatsConfigData& statsConfig, const StatCategory *category, ColumnSet& statColumns, const char8_t *tableName, int32_t periodType)
{
    INFO_LOG("[StatsBoostrap].addTable(): Sending add table call to DB for table " << tableName);

    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    
    query->append("CREATE TABLE `$s` (\n", tableName);
    
    query->append("`entity_id` bigint(20) unsigned NOT NULL,\n");

    bool hasPeriod = periodType != STAT_PERIOD_ALL_TIME;
    if (hasPeriod)
        query->append("`period_id` int(11) NOT NULL,\n");
    
    const ScopeNameSet* keyScopeNameSet = category->getScopeNameSet();
   
    size_t size = 0;
    if (keyScopeNameSet)
    {
        size = keyScopeNameSet->size();
        for (size_t i = 0; i < size; i++)
        {
            StatTypeDb dbType = statsConfig.getScopeDbType((*keyScopeNameSet)[i].c_str());
            char8_t typeStr[64];
            dbType.getDbTypeString(typeStr, sizeof(typeStr));

            ScopeValue aggregateKeyValue = statsConfig.getAggregateKeyValue((*keyScopeNameSet)[i].c_str());
            if (aggregateKeyValue < 0)
                query->append("`$s` $s NOT NULL,\n", (*keyScopeNameSet)[i].c_str(), typeStr);
            else
                query->append("`$s` $s NOT NULL DEFAULT '$D',\n", (*keyScopeNameSet)[i].c_str(), typeStr, aggregateKeyValue);
        }
    }
        
    query->append("`last_modified` timestamp NOT NULL DEFAULT "
        "CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,\n");
  
    const StatMap* statMap = category->getStatsMap();    
    ColumnSet::const_iterator statColumnItr = statColumns.begin();
    ColumnSet::const_iterator statColumnEnd = statColumns.end();

    for (; statColumnItr != statColumnEnd; ++statColumnItr)
    {
        const char8_t* statName = (*statColumnItr).c_str();
        StatMap::const_iterator statItr = statMap->find(statName);
        if (statItr == statMap->end())
        {
            ERR_LOG("[StatsBoostrap].addTable(): stat " << statName << " not found when creating table " << tableName);
            return false;
        }
        const Stat* stat = statItr->second;
        char8_t typeStr[64];
        stat->getDbType().getDbTypeString(typeStr, sizeof(typeStr));
        if (stat->getDbType().getType() == STAT_TYPE_INT)
        {
            query->append("`$s` $s NOT NULL DEFAULT '$D',\n", 
                stat->getName(), typeStr, stat->getDefaultIntVal());
        }
        else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
        {
            query->append("`$s` $s NOT NULL DEFAULT '$f',\n",
                stat->getName(), typeStr, (double_t) stat->getDefaultFloatVal());
        }
        else if (stat->getDbType().getType() == STAT_TYPE_STRING)
        {
            query->append("`$s` $s NOT NULL DEFAULT '$s',\n",
                stat->getName(), typeStr, stat->getDefaultStringVal());
        }
    }

    // primary key columns
    query->append("PRIMARY KEY (`entity_id`");

    if (hasPeriod)
        query->append(",`period_id`");

    for (size_t i = 0; i < size; i++)
    {
        query->append(",`$s`", (*keyScopeNameSet)[i].c_str());
    }
    query->append(")\n");

    // if primary key is just an entity_id then we dont another one like that
    if (hasPeriod || size != 0)
    {
        query->append(", KEY `$s` USING BTREE (", DB_INDEX_KEY_NAME);

        if (hasPeriod)
        {
            query->append("`period_id`,");
        }
        
        for (size_t i = 0; i < size; i++)
        {
            query->append("`$s`,", (*keyScopeNameSet)[i].c_str(), 0);
        }
        query->trim(1);
        query->append(")\n");
    }

    query->append(") ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='Blaze stats table for category $s'", category->getName());

    if (hasPeriod)
    {
        int32_t currentPeriodId = mStatsMasterImpl->getPeriodId(periodType);
        query->append("\nPARTITION BY LIST(`period_id`) (\n");

        PeriodIdList existingPeriodIds;
        mStatsMasterImpl->mRollover.addNewPeriodIdsToQuery(query, tableName, existingPeriodIds, periodType, currentPeriodId);

        query->trim(1);
        query->append(")");
    }
    query->append(";\n");

    DbError err = conn->executeQuery(query);
    if (err != DB_ERR_OK)
    {
        ERR_LOG("[StatsBootstrap].tableAdded(): Failure adding table, cannot continue");   
        return false;
    }

    INFO_LOG("[StatsBootstrap].tableAdded(): Table addition was successful for table: " << tableName);
    return true;
}

/*************************************************************************************************/
/*!
    \brief fetchAndUpdateColumns

    Fetch the columns for all existing tables from the DB so that they can be compared against the
    config file entries.  The method then iterates through all the results and has
    to look for four things: new columns that need to be added, defunct columns which need to be
    dropped, columns with changed data types, and columns with other changes (e.g. default value).
    It then prepares all of the SQL required to make those changes.

    \param[in]  conn - The DB conn we use for our queries.
*/
/*************************************************************************************************/
bool StatsBootstrap::fetchAndUpdateColumns(DbConnPtr conn, const char8_t* dbName,
                                           const StatsConfigData& statsConfig, TableMap& existingTableMap)
{
    INFO_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Preparing request to fetch column data for all existing tables");

    // May need to redo keys, lets get current state of keys
    bool oldIndexName = false;
    QueryPtr modifyColumnsQuery = DB_NEW_QUERY_PTR(conn);
    
    TableMap::iterator tableIter = existingTableMap.begin();
    TableMap::iterator tableEnd = existingTableMap.end();

    for (; tableIter != tableEnd; ++tableIter)
    {
        const char8_t* tableName = tableIter->first;
        TableData& tableData = tableIter->second;
        
        QueryPtr fetchKeysQuery = DB_NEW_QUERY_PTR(conn);
        fetchKeysQuery->append("SHOW INDEX FROM `$s`;", tableName);
        DbResultPtr indexResult;
        DbError error = conn->executeQuery(fetchKeysQuery, indexResult);

        if (error != DB_ERR_OK)
        {
            ERR_LOG("[StatsBootstrap].fetchAndUpdateColumns() Failed to show indices for table: <" 
                      << tableName << "> error = " << getDbErrorString(error));
            return false;
        }
                
        if (!indexResult->empty())
        {
            int32_t seqNum;
            const char8_t* columnName;
            DbResult::const_iterator itr = indexResult->begin();
            DbResult::const_iterator end = indexResult->end();
            for (; itr != end; ++itr)
            {
                const DbRow *row = *itr;
                const char8_t* key_name = row->getString("Key_name");
                if (blaze_stricmp(key_name, DB_ENTITY_KEY_NAME) == 0)
                {
                    tableData.hasEntityKey = true;
                }
                else if (blaze_stricmp(key_name, "PRIMARY") == 0)
                {
                     seqNum = row->getInt("Seq_in_index");
                     columnName = row->getString("Column_name");
                     tableData.insertKey(tableData.primaryKey, seqNum, columnName);
                     tableData.hasPrimary = true;
                     // Seems only way to identify keyscopes in old table
                     if (blaze_stricmp(columnName, "entity_id") != 0 && 
                         /// @todo cleanup: deprecate...
                         blaze_stricmp(columnName, "context_id") != 0 &&
                         blaze_stricmp(columnName, "period_id") != 0)
                        {
                            tableData.oldScopes.insert(blaze_strdup(columnName));
                            tableData.hasScopes = true;
                        } 
                }
                else if (blaze_stricmp(key_name, DB_INDEX_KEY_NAME) == 0)
                {
                    tableData.hasSecondry = true;
                }
                else if (blaze_stricmp(key_name, "ix_period_id") == 0)
                {
                    // don't fail out just yet ... try to list all tables with old index name
                    ERR_LOG("[StatsBootstrap].fetchAndUpdateColumns() found old index name 'ix_period_id' for table: <" << tableName << ">");
                    oldIndexName = true;
                    break;
                }
            }
        }

        // check if we need to drop any indices
        tableData.newPrimaryKey.push_back("entity_id");
        if (tableData.needPeriod)
            tableData.newPrimaryKey.push_back("period_id");
        
        // Assuming that 'ScopeNameSet' keeps keyscopes sorted alphabetically
        const ScopeNameSet* bkScopeNameSet = tableData.backupScopeNameSet;
        if (bkScopeNameSet != nullptr)
        {
            ScopeNameSet::const_iterator scIt = bkScopeNameSet->begin();
            ScopeNameSet::const_iterator scEnd = bkScopeNameSet->end();
            for (; scIt != scEnd; ++scIt)
            {
                tableData.newPrimaryKey.push_back(scIt->c_str());
            }
        }
        
        // Check if primary key has changed
        size_t size = tableData.primaryKey.size();
        if (size != tableData.newPrimaryKey.size())
            tableData.primaryDroped = true;
        else
        {
            for (size_t i=0; i < size; ++i)
            {
                if (blaze_stricmp(tableData.newPrimaryKey[i], tableData.primaryKey[i]) != 0)
                {
                    tableData.primaryDroped = true;
                    break;
                }
            }
        }

        if (tableData.primaryDroped && tableData.hasPrimary)
        {
            modifyColumnsQuery->append("ALTER TABLE `$s` DROP PRIMARY KEY;\n", tableName);
        }

        // If primary was not there consider it dropped
        if (!tableData.hasPrimary) tableData.primaryDroped = true;
        
        // secondary is expected everything in primary except entity_id
        // so if primary changed secondary as well
        if (tableData.primaryDroped && tableData.hasSecondry)
        {
            modifyColumnsQuery->append("ALTER TABLE `$s` DROP KEY `$s`;\n", tableName, DB_INDEX_KEY_NAME);
        }
        
        if (tableData.hasEntityKey)
        {
            // not wanted anymore ...
            // This index contains entity_id which is already included in the leading portion of the primary key.
            // It may have helped with startup times, but DB LB table changes made this not really necessary anymore.
            modifyColumnsQuery->append("ALTER TABLE `$s` DROP KEY `$s`;\n", tableName, DB_ENTITY_KEY_NAME);
        }
    } // end for each existing table

    if (oldIndexName)
    {
        FATAL_LOG("[StatsBootstrap].fetchAndUpdateColumns() found old index name 'ix_period_id' for 1 or more tables\n"
            "*** manually change index name to '" << DB_INDEX_KEY_NAME << "' or drop those tables so they can be recreated ***");
        return false;
    }

    tableIter = existingTableMap.begin();

    QueryPtr fetchColumnsQuery = DB_NEW_QUERY_PTR(conn);
    fetchColumnsQuery->append("SELECT TABLE_NAME, COLUMN_NAME, COLUMN_DEFAULT, DATA_TYPE, CHARACTER_MAXIMUM_LENGTH "
        "FROM INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA LIKE '$L' AND TABLE_NAME IN (", dbName);
    for (; tableIter != tableEnd; ++tableIter)
    {
        const char8_t* tableName = tableIter->first;
        fetchColumnsQuery->append("'$s',", tableName);
    }
    fetchColumnsQuery->trim(1);
    fetchColumnsQuery->append(");");

    DbResultPtr fetchColumnsResult;
    DbError err = conn->executeQuery(fetchColumnsQuery, fetchColumnsResult);
   
    INFO_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Received reply");

    if (err != DB_ERR_OK)
    {
        ERR_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Error fetching columns from DB, cannot continue");
        return false;
    }

    if (!fetchColumnsResult->empty())
    {
        TableColumnsMap tableColumnsToDeleteMap;

        INFO_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Found " << fetchColumnsResult->size() << " results");
        DbResult::const_iterator itr = fetchColumnsResult->begin();
        DbResult::const_iterator end = fetchColumnsResult->end();
        for (; itr != end; ++itr)
        {
            const DbRow *row = *itr;
            const char8_t* tableName = row->getString("TABLE_NAME");
            const char8_t* columnName = row->getString("COLUMN_NAME");
            const char8_t* columnDefault = row->getString("COLUMN_DEFAULT");
            const char8_t* dataType = row->getString("DATA_TYPE");
            uint32_t dataLength = row->getUInt("CHARACTER_MAXIMUM_LENGTH");

            TableData& tableData = existingTableMap.find(tableName)->second;

            // Skip past any core columns
            if (blaze_strcmp("entity_id", columnName) == 0 ||
                blaze_strcmp("last_modified", columnName) == 0)
            {
                continue;
            }

            if (blaze_strcmp("period_id", columnName) == 0)
            {
                if (!tableData.needPeriod)
                {
                    tableColumnsToDeleteMap[tableName].insert(columnName);
                }                
                tableData.hasPeriod = true;   
                continue;
            }

            // if it is a keyscope column and we need it leave it there
            if (tableData.scopeNameSet != nullptr)
            {
                ScopeNameSet::iterator scIt = tableData.scopeNameSet->find(columnName);

                if (scIt != tableData.scopeNameSet->end())
                {
                    ScopeValue aggregateKeyValue = statsConfig.getAggregateKeyValue((*scIt).c_str());
                    char8_t typeStr[64];
                    StatTypeDb scopeDataType = statsConfig.getScopeDbType((*scIt).c_str());
                    scopeDataType.getDbTypeString(typeStr, sizeof(typeStr));

                    // Check keyscope column's existing default against the configured aggregate key.
                    if (columnDefault == nullptr)
                    {
                        if (aggregateKeyValue >= 0)
                        {
                            // customer should decide on potentially time-consuming action... e.g. add/insert aggregate rows accordingly?
                            ERR_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Adding aggregate key must be done manually for keyscope '" << columnName << "'.");
                            return false;
                        }
                    }
                    else
                    {
                        int64_t defVal = 0;
                        blaze_str2int(columnDefault, &defVal);
                        
                        if (aggregateKeyValue < 0)
                        {
                            // customer should decide on potentially time-consuming action... e.g. delete the 'orphaned' rows (those rows with the old aggregate key value)?
                            ERR_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Removing aggregate key must be done manually for keyscope '" << columnName << "'.");
                            return false;
                        }
                        else if (defVal != aggregateKeyValue)
                        {
                            // customer should decide on potentially time-consuming action... e.g. should rows using the old aggregate key value be changed to use the new value?
                            ERR_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Changing aggregate key value must be done manually for keyscope '" << columnName << "'.");
                            return false;
                        }
                    }

                    // Validate that the keyscope column's integer size matches
                    StatTypeDb dbDataType = StatTypeDb::getTypeFromDbString(dataType, dataLength);
                    if (scopeDataType != dbDataType)
                    {
                        // UPDATE A KEYSCOPE COLUMN, DESTRUCTIVE
                        WARN_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Warning, changing keyscope '" << columnName << "' type to " << typeStr << ".");
                              
                        if (aggregateKeyValue < 0)
                        {
                            modifyColumnsQuery->append(
                                "ALTER TABLE `$s` CHANGE COLUMN `$s` `$s` $s NOT NULL;\n",
                                tableName, columnName, columnName, typeStr);
                        }
                        else
                        {
                            modifyColumnsQuery->append(
                                "ALTER TABLE `$s` CHANGE COLUMN `$s` `$s` $s NOT NULL DEFAULT '$D';\n",
                                tableName, columnName, columnName, typeStr, aggregateKeyValue);
                        }

                        if (!mAllowDestructiveActions)
                        {
                            WARN_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Shutting down to prevent destructive DB action.");
                            return false;
                        }
                    }

                    tableData.scopeNameSet->erase(scIt);
                    continue;
                }
            }

            StatMap* statMap = tableData.statMap;
            StatMap::iterator iter = statMap->find(columnName);

            if (iter == statMap->end())
            {
                // REMOVE A COLUMN, DESTRUCTIVE

                // there is additional action if it's a scope column...
                if (tableData.hasScopes)
                {
                    if (tableData.oldScopes.find(columnName) != tableData.oldScopes.end())
                    {
                        // if the column to be removed is a keyscope column, customer should handle potentially time-consuming action...
                        // e.g. delete all rows where the keyscope column != aggregrate value (i.e. default_value), and then drop the column
                        ERR_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Removing keyscope '" << columnName << "' from " << tableName
                            << " must be done manually (you will probably need to drop any corresponding leaderboard index table as well).");
                        return false;
                    }
                }

                // drop the column
                WARN_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Warning, removing column " << columnName << " from " << tableName);

                tableColumnsToDeleteMap[tableName].insert(columnName);

                if (!mAllowDestructiveActions)
                {
                    WARN_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Shutting down to prevent destructive DB action.");
                    return false;
                }
            }
            else
            {
                Stat* stat = iter->second;
                statMap->erase(iter);
                StatTypeDb dbDataType = StatTypeDb::getTypeFromDbString(dataType, dataLength);
                char8_t typeStr[64];
                stat->getDbType().getDbTypeString(typeStr, sizeof(typeStr));

                if (stat->getDbType().getType() == STAT_TYPE_INT)
                {
                    if (stat->getDbType() != dbDataType)
                    {
                        WARN_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Modifying column type of " << columnName << " in " 
                                 << tableName << " to " << typeStr);
                        // UPDATE A COLUMN, POTENTIALLY DESTRUCTIVE
                        modifyColumnsQuery->append(
                                "ALTER TABLE `$s` CHANGE COLUMN `$s` `$s` $s NOT NULL DEFAULT '$D';\n",
                            tableName, columnName, columnName, typeStr, stat->getDefaultIntVal());

                        if (!mAllowDestructiveActions)
                        {
                            WARN_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Shutting down to prevent destructive DB action.");
                            return false;
                        }
                    }
                    else
                    {
                        int64_t defVal = 0;                                 
                        blaze_str2int(columnDefault, &defVal);
                        if (defVal != stat->getDefaultIntVal())
                        {
                            INFO_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Modifying default for " << columnName << " in " << tableName);
                            // UPDATE DEFAULT ONLY, NOT DESTRUCTIVE
                            modifyColumnsQuery->append(
                                "ALTER TABLE `$s` CHANGE COLUMN `$s` `$s` $s NOT NULL DEFAULT '$D';\n",
                                tableName, columnName, columnName, typeStr, stat->getDefaultIntVal());
                        }
                    }
                }
                else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                {
                    if (stat->getDbType() != dbDataType)
                    {
                        WARN_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Modifying column type of " << columnName << " in " << tableName << " to float");
                         // UPDATE A COLUMN, POTENTIALLY DESCTRUCTIVE
                        modifyColumnsQuery->append(
                            "ALTER TABLE `$s` CHANGE COLUMN `$s` `$s` $s NOT NULL DEFAULT '$f';\n",
                            tableName, columnName, columnName, typeStr, (double_t) stat->getDefaultFloatVal());

                        if (!mAllowDestructiveActions)
                        {
                            WARN_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Shutting down to prevent destructive DB action.");
                            return false;
                        }

                    }
                    else
                    {
                        float_t defVal = static_cast<float_t>(atof(columnDefault));
                        if (defVal != stat->getDefaultFloatVal())
                        {
                            INFO_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Modifying default for " << columnName << " in " << tableName);
                            // UPDATE DEFAULT ONLY, NOT DESTRUCTIVE
                            modifyColumnsQuery->append(
                                "ALTER TABLE `$s` CHANGE COLUMN `$s` `$s` $s NOT NULL DEFAULT '$f';\n",
                                tableName, columnName, columnName, typeStr, (double_t) stat->getDefaultFloatVal());

                        }
                    }
                }
                else if (stat->getDbType().getType() == STAT_TYPE_STRING)
                {
                    if (stat->getDbType() != dbDataType)
                    {
                        WARN_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Modifying column type of " << columnName << " in " << tableName << " to string");
                         // UPDATE A COLUMN, POTENTIALLY DESCTRUCTIVE
                        modifyColumnsQuery->append(
                            "ALTER TABLE `$s` CHANGE COLUMN `$s` `$s` $s NOT NULL DEFAULT '$s';\n",
                            tableName, columnName, columnName, typeStr, stat->getDefaultStringVal());

                        if (!mAllowDestructiveActions)
                        {
                            WARN_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Shutting down to prevent destructive DB action.");
                            return false;
                        }

                    }
                    else
                    {
                        if (blaze_strcmp(columnDefault, stat->getDefaultStringVal()) != 0)
                        {
                            INFO_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Modifying default for " << columnName << " in " << tableName);
                            // UPDATE DEFAULT ONLY, NOT DESTRUCTIVE
                            modifyColumnsQuery->append(
                                "ALTER TABLE `$s` CHANGE COLUMN `$s` `$s` $s NOT NULL DEFAULT '$s';\n",
                                tableName, columnName, columnName, typeStr, stat->getDefaultStringVal());

                        }
                    }
                }
                
            } // end column found
        }
        
        // delete columns for every table at once
        TableColumnsMap::const_iterator tcIter = tableColumnsToDeleteMap.begin();
        TableColumnsMap::const_iterator tcEnd = tableColumnsToDeleteMap.end();
        for (; tcIter != tcEnd; ++tcIter)
        {
            const StringSet& colSet = tcIter->second;
            if (colSet.empty())
                continue;

             modifyColumnsQuery->append("ALTER TABLE `$s`", tcIter->first);
             
             StringSet::const_iterator colIter = colSet.begin();
             StringSet::const_iterator colEnd = colSet.end();
             for (; colIter != colEnd; ++colIter)
             {
                modifyColumnsQuery->append(" DROP COLUMN `$s`,", *colIter);
             }
             
             modifyColumnsQuery->trim(1);
             modifyColumnsQuery->append(";\n");
        }             
    } // if fetchColumnsResult not empty

    // Now that we've gone through all the results from the DB, check for any columns that we didn't
    // find there, which means we need to add them
    TableMap::iterator iter = existingTableMap.begin();
    TableMap::iterator end = existingTableMap.end();

    while (iter != end)
    {
        TableData& tableData = iter->second;
        const char8_t* tableName = iter->first;
        StatMap* statMap = tableData.statMap;
        ++iter;

        if (tableData.scopeNameSet != nullptr && !tableData.scopeNameSet->empty())
        {
            // if the column to be added is a keyscope column, customer should handle potentially time-consuming action...
            // e.g. what is the appropriate key for existing rows?  and if there is an aggregrate key, what are appropriate values for new aggregrate rows?
            ERR_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Adding keyscope '" << (*(tableData.scopeNameSet->begin())).c_str() << "' to " << tableName << " must be done manually.");
            return false;
        }

        StatMap::const_iterator statIter = statMap->begin();
        StatMap::const_iterator statEnd = statMap->end();
        bool addPeriodId = !tableData.hasPeriod && tableData.needPeriod;
        bool commaAdded = false;

        if (addPeriodId || !statMap->empty())
        {
            modifyColumnsQuery->append("ALTER TABLE `$s`", tableName);
        }

        //If we need period_id and it is not there
        if (addPeriodId)
        {
            modifyColumnsQuery->append(" ADD COLUMN `period_id` int(11) NOT NULL AFTER `entity_id`,\n");
            commaAdded = true;
        }

        if (statIter != statEnd)
        {
            commaAdded = false;
            modifyColumnsQuery->append(" ADD COLUMN (", tableName);
            
            while (statIter != statEnd)
            {
                const Stat* stat = (statIter++)->second;
                char8_t typeStr[64];
                stat->getDbType().getDbTypeString(typeStr, sizeof(typeStr));

                INFO_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Adding new column " << stat->getName() << " to " << tableName);

                if (stat->getDbType().getType() == STAT_TYPE_INT)
                {
                    modifyColumnsQuery->append("`$s` $s NOT NULL DEFAULT '$D'",
                            stat->getName(), typeStr, stat->getDefaultIntVal());
                }
                else if (stat->getDbType().getType() == STAT_TYPE_FLOAT)
                {
                    modifyColumnsQuery->append("`$s` $s NOT NULL DEFAULT '$f'",
                        stat->getName(), typeStr, (double_t) stat->getDefaultFloatVal());
                }
                else if (stat->getDbType().getType() == STAT_TYPE_STRING)
                {
                    modifyColumnsQuery->append("`$s` $s NOT NULL DEFAULT '$s'",
                        stat->getName(), typeStr, stat->getDefaultStringVal());
                }

                if (statIter != statEnd)
                    modifyColumnsQuery->append(",\n");
            }
            
            modifyColumnsQuery->append(");\n");
        }
        
        if (commaAdded)
        {
                modifyColumnsQuery->trim(2);
                modifyColumnsQuery->append(";\n");
        }

        // primary key was dropped, create new one
        if (tableData.primaryDroped)
        {
            modifyColumnsQuery->append("ALTER TABLE `$s` ADD PRIMARY KEY (", tableName);
            size_t size = tableData.newPrimaryKey.size();
            for (size_t i = 0; i < size; ++i)
            {
                modifyColumnsQuery->append("`$s`,", tableData.newPrimaryKey[i]);
            }
            modifyColumnsQuery->trim(1);
            
            // if pramary key is entity_id only we dont need secondry key
            if (size > 1)
            {
                modifyColumnsQuery->append("), ADD KEY `$s` USING BTREE (", DB_INDEX_KEY_NAME);
                for (size_t i = 1; i < size; ++i)
                {
                    modifyColumnsQuery->append("`$s`,", tableData.newPrimaryKey[i]);
                }
                modifyColumnsQuery->trim(1);
            }
            modifyColumnsQuery->append(");\n");
        }    

        delete statMap;
    }

    if (modifyColumnsQuery->isEmpty())
    {
        INFO_LOG("[StatsBootstrap].fetchAndUpdateColumns(): No changes detected, all done");
        return true;
    }

    TimeValue queryTime = TimeValue::getTimeOfDay();
    INFO_LOG("[StatsBootstrap].fetchAndUpdateColumns(): executing ALTER TABLE query; may be a lengthy operation. Query:\r" << modifyColumnsQuery->get());
    DbError modifyColumnsErr = conn->executeMultiQuery(modifyColumnsQuery);

    queryTime = TimeValue::getTimeOfDay() - queryTime;
    INFO_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Query executed in " << queryTime.getSec() << " seconds.");

    if (modifyColumnsErr != DB_ERR_OK)
    {
        ERR_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Failed to ALTER TABLE(s), cannot continue");
        return false;
    }

    INFO_LOG("[StatsBootstrap].fetchAndUpdateColumns(): Successfully altered all tables");
    return true;
}

bool StatsBootstrap::fetchAndUpdatePartitions(DbConnPtr conn, const char8_t* dbName, TableMap& existingTableMap)
{
    INFO_LOG("[StatsBootstrap].fetchAndUpdatePartitions(): Preparing to re-partition all existing tables");

    TableMap::const_iterator tableItr = existingTableMap.begin();
    TableMap::const_iterator tableEnd = existingTableMap.end();
    for (; tableItr != tableEnd; ++tableItr)
    {
        const TableData& tableData = tableItr->second;
        if (!tableData.hasPeriod)
        {
            continue;
        }

        if (!fetchAndUpdatePartitionsForTable(conn, dbName, tableItr->first, tableData.periodType))
        {
            return false;
        }
    }

    INFO_LOG("[StatsBootstrap].fetchAndUpdatePartitions(): Successfully altered all tables (if necessary)");
    return true;
}

bool StatsBootstrap::fetchAndUpdatePartitionsForTable(DbConnPtr conn, const char8_t *dbName, const char8_t *tableName, int32_t periodType)
{
    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    query->append("SELECT PARTITION_NAME,PARTITION_DESCRIPTION FROM INFORMATION_SCHEMA.PARTITIONS"
        " WHERE TABLE_SCHEMA LIKE '$L'"
        " AND PARTITION_DESCRIPTION IS NOT NULL"
        " AND TABLE_NAME = '$s';", dbName, tableName);

    DbResultPtr dbResult; 
    DbError dbError = conn->executeQuery(query, dbResult);

    if (dbError != DB_ERR_OK)
    {
        ERR_LOG("[StatsBootstrap].fetchAndUpdatePartitions(): error getting partitions for table " << tableName << ": " << getDbErrorString(dbError));
        return false;
    }

    if (dbResult->size() > 0)
    {
        // this table is already using partitions; update the partitions/periods
        PeriodIdList existingPeriods;
        DbResult::const_iterator rowItr = dbResult->begin();
        DbResult::const_iterator rowEnd = dbResult->end();
        for (; rowItr != rowEnd; ++rowItr)
        {
            const DbRow *row = *rowItr;
            int32_t periodId = row->getInt("PARTITION_DESCRIPTION");
            existingPeriods.push_back(periodId);

            // Take note of all the partitions that exist at startup time
            mStatsMasterImpl->mRollover.cachePartitionName(tableName, periodId, row->getString("PARTITION_NAME"));
        }

        // add any missing partitions/periods
        query = DB_NEW_QUERY_PTR(conn);
        query->append("ALTER TABLE `$s` ADD PARTITION (", tableName);
        int32_t numAdds = mStatsMasterImpl->mRollover.addNewPeriodIdsToQuery(query, tableName, existingPeriods,
            periodType, mStatsMasterImpl->getPeriodId(periodType));

        if (numAdds == 0)
        {
            TRACE_LOG("[StatsBootstrap].fetchAndUpdatePartitions(): no partitions to add for table " << tableName);
        }
        else
        {
            query->trim(1);
            query->append("\n);");
            dbError = conn->executeQuery(query);

            if (dbError != DB_ERR_OK)
            {
                ERR_LOG("[StatsBootstrap].fetchAndUpdatePartitions(): failed to add partitions for table " << tableName << ": " 
                        << getDbErrorString(dbError));
                return false;
            }

            TRACE_LOG("[StatsBootstrap].fetchAndUpdatePartitions(): " << numAdds << " partitions added for table " << tableName);
        }
    }
    else
    {
        // this table is not using partitions; but needs to...

        // fetch the existing period ids
        query = DB_NEW_QUERY_PTR(conn);
        query->append("SELECT DISTINCT(`period_id`) FROM `$s`;", tableName);

        dbError = conn->executeQuery(query, dbResult);
        if (dbError != DB_ERR_OK)
        {
            ERR_LOG("[StatsBootstrap].fetchAndUpdatePartitions(): error getting periods for table " << tableName << ": " << getDbErrorString(dbError));
            return false;
        }

        PeriodIdList existingPeriods;
        DbResult::const_iterator rowItr = dbResult->begin();
        DbResult::const_iterator rowEnd = dbResult->end();
        for (; rowItr != rowEnd; ++rowItr)
        {
            const DbRow *row = *rowItr;
            int32_t periodId = row->getInt("period_id");
            existingPeriods.push_back(periodId);
        }

        query = DB_NEW_QUERY_PTR(conn);
        query->append("ALTER TABLE `$s` PARTITION BY LIST(`period_id`) (", tableName);
        int32_t numAdds = mStatsMasterImpl->mRollover.addExistingPeriodIdsToQuery(query, tableName, existingPeriods,
            periodType);
        numAdds += mStatsMasterImpl->mRollover.addNewPeriodIdsToQuery(query, tableName, existingPeriods,
            periodType, mStatsMasterImpl->getPeriodId(periodType));

        // as a new partition specification, we must have partitions to add
        query->trim(1);
        query->append("\n);");

        TimeValue queryTime = TimeValue::getTimeOfDay();
        INFO_LOG("[StatsBootstrap].fetchAndUpdatePartitions(): query to add partitioning for table " << tableName << "; may be a lengthy operation.");

        dbError = conn->executeQuery(query);

        if (dbError != DB_ERR_OK)
        {
            ERR_LOG("[StatsBootstrap].fetchAndUpdatePartitions(): failed to create partitions for table " << tableName 
                    << ": " << getDbErrorString(dbError));
            return false;
        }

        queryTime = TimeValue::getTimeOfDay() - queryTime;
        INFO_LOG("[StatsBootstrap].fetchAndUpdatePartitions(): New partitioning for table " << tableName << " executed in " 
                 << queryTime.getSec() << " seconds.");
    }
    return true;
}

} // Stats
} // Blaze
