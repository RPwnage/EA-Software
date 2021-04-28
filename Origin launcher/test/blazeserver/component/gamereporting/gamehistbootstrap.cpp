/*************************************************************************************************/
/*!
    \file   gamehistbootstrap.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"

#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/util/shared/blazestring.h"

#include "gamereporting/gamehistbootstrap.h"

namespace Blaze
{
namespace GameReporting
{
const char8_t* ONLINE_COLUMN_NAME = "online";
const char8_t* FLAGS_COLUMN_NAME = "flags";
const char8_t* FLAG_REASON_COLUMN_NAME = "flag_reason";

/*** Public Methods ******************************************************************************/

GameHistoryBootstrap::GameHistoryBootstrap(const GameReportingSlaveImpl* component,
                                           const GameTypeMap* gameTypeMap,
                                           const GameReportQueryConfigMap* gameReportQueryConfigMap,
                                           const GameReportViewConfigMap* gameReportViewConfigMap,
                                           bool allowDestruct) :
mComponent(component),
mGameTypeMap(gameTypeMap),
mGameReportQueryConfigMap(gameReportQueryConfigMap),
mGameReportViewConfigMap(gameReportViewConfigMap),
mAllowDestructiveActions(allowDestruct)
{
    if (mAllowDestructiveActions)
    {
        TRACE_LOG("[GameHistBootstrap].GameHistoryBootstrap: set to allow destructive actions on game history tables.");
    }

    mGamekeyTableReservedColumnsMap[ONLINE_COLUMN_NAME] = "tinyint(1) unsigned DEFAULT 1";
    mGamekeyTableReservedColumnsMap[FLAGS_COLUMN_NAME] = "bigint(20) unsigned DEFAULT 0";
    mGamekeyTableReservedColumnsMap[FLAG_REASON_COLUMN_NAME] = "varchar(127)";
}

GameHistoryBootstrap::~GameHistoryBootstrap()
{
}

bool GameHistoryBootstrap::run()
{
    TRACE_LOG("[GameHistBootstrap].run: continuing bootstrap process, making connection to DB.");
    return (GameHistoryBootstrap::doDbBootStrap() == ERR_OK);
}

BlazeRpcError GameHistoryBootstrap::doDbBootStrap()
{
    if (!retrieveExistingTables() || !initializeGameHistoryTables())
    {
        return ERR_SYSTEM;
    }

    if (mGameReportQueryConfigMap != nullptr)
    {
        GameReportQueryConfigMap::const_iterator queryIter, queryEnd;
        queryIter = mGameReportQueryConfigMap->begin();
        queryEnd = mGameReportQueryConfigMap->end();

        for (; queryIter != queryEnd; ++queryIter)
        {
            const GameReportQuery* query = queryIter->second->getGameReportQuery();
            const GameType& gameType = *(mGameTypeMap->find(query->getTypeName())->second);
            const GameReportFilterList& filterList = query->getFilterList();
            buildIndexesInMemory(gameType, query->getName(), "query", filterList);
        }
    }

    if (mGameReportViewConfigMap != nullptr)
    {
        GameReportViewConfigMap::const_iterator viewIter, viewEnd;
        viewIter = mGameReportViewConfigMap->begin();
        viewEnd = mGameReportViewConfigMap->end();

        for (; viewIter != viewEnd; ++viewIter)
        {
            const GameReportView* view = viewIter->second->getGameReportView();
            const GameType& gameType = *(mGameTypeMap->find(view->getViewInfo().getTypeName())->second);
            const GameReportFilterList& filterList = view->getFilterList();
            buildIndexesInMemory(gameType, (view->getViewInfo()).getName(), "view", filterList);
        }
    }

    BlazeRpcError err = createIndexesFromMemory();

    mExistingTableNames.clear();
    return err;
}

bool GameHistoryBootstrap::retrieveExistingTables()
{
    const GameTypeConfigMap& gameTypes = mComponent->getConfig().getGameTypes();
    for (GameTypeConfigMap::const_iterator iter = gameTypes.begin(),
         end = gameTypes.end();
         iter != end;
         ++iter)
    {
        const GameManager::GameReportName& gameType = iter->first;
        DbConnPtr dbConn = mComponent->getDbConnPtr(gameType);
        if (dbConn == nullptr)
        {
            ERR_LOG("[GameHistoryBootstrap].retrieveExistingTables: failed to connect to database for gameType " << iter->first << ", cannot continue.");
            return false;
        }

        QueryPtr query = DB_NEW_QUERY_PTR(dbConn);

        if (query == nullptr)
        {
            ERR_LOG("[GameHistBootstrap].retrieveExistingTables: failed to grab new query object.");
            return false;
        }

        // Request all the table names from the database
        const char8_t* dbName = mComponent->getDbSchemaNameForGameType(gameType);
        query->append("SHOW TABLES FROM `$s`;", dbName);

        DbResultPtr tableListResult;
        BlazeRpcError dbErr = dbConn->executeQuery(query, tableListResult);

        if (dbErr != DB_ERR_OK)
        {
            ERR_LOG("[GameHistBootstrap].retrieveExistingTables: table list query failed (err=" << getDbErrorString(dbErr) << ").");
            return false;
        }

        if (!tableListResult->empty())
        {
            TRACE_LOG("[GameHistBootstrap].retrieveExistingTables: Found " << tableListResult->size() << " tables in the DB");
            DbResult::const_iterator resItr = tableListResult->begin();
            DbResult::const_iterator resEnd = tableListResult->end();
            for (; resItr != resEnd; ++resItr)
            {
                mExistingTableNames.insert((*resItr)->getString((uint32_t)0));
            }
        }
    }


    return true;
}

//create game history tables for all of the game types
bool GameHistoryBootstrap::initializeGameHistoryTables() const
{
    GameTypeMap::const_iterator iter, end;
    iter = mGameTypeMap->begin();
    end = mGameTypeMap->end();

    for (;iter != end; ++iter)
    {
        const GameType &gameType = *iter->second;
        if (!initializeGameTypeTables(gameType))
        {
            return false;
        }
    }

    return true;
}

// create game history tables for a particular game type
bool GameHistoryBootstrap::initializeGameTypeTables(const GameType &gameType) const
{
    const HistoryTableDefinitions& historyTableDefinitions = gameType.getHistoryTableDefinitions();

    if (historyTableDefinitions.size() > 0)
    {
        // first create the built-in game table
        if (!createBuiltInGameTable(gameType) || 
            !createBuiltInParticipantTable(gameType) ||
            !createBuiltInSubmitterTable(gameType))
        {
            return false;
        }

        HistoryTableDefinitions::const_iterator iter = historyTableDefinitions.begin();
        HistoryTableDefinitions::const_iterator end = historyTableDefinitions.end();

        for (;iter != end; ++iter)
        {
            const char8_t *tableName = iter->first;
            HistoryTableDefinition *historyTableDefinition = iter->second;
            if (historyTableDefinition->getColumnList().size() == 0)
            {
                continue;
            }
            if (!initializeHistoryTable(gameType, tableName, historyTableDefinition))
            {
                return false;
            }
        }
    }

    return true;
}

bool GameHistoryBootstrap::initializeHistoryTable(const GameType &gameType, const char8_t *tableName, 
                                                  const HistoryTableDefinition* historyTableDefinition) const
{
    char8_t fullTableName[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(fullTableName, sizeof(fullTableName), "%s_hist_%s",
        gameType.getGameReportName().c_str(), tableName);
    blaze_strlwr(fullTableName);

    if (mExistingTableNames.find(fullTableName) == mExistingTableNames.end())
    {
        if (!createHistoryTable(gameType, fullTableName, historyTableDefinition))
        {
            return false;
        }
    }
    else
    {
        if (!verifyHistoryTableColumns(gameType, fullTableName, historyTableDefinition))
        {
            return false;
        }
    }

    char8_t refTableName[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(refTableName, sizeof(refTableName), "%s_ref", fullTableName);
    blaze_strlwr(refTableName);

    if (mExistingTableNames.find(refTableName) == mExistingTableNames.end())
    {
        if (!createGameRefTable(gameType, refTableName, historyTableDefinition))
        {
            return false;
        }
    }
    else
    {
        if (!verifyGameRefTableColumns(gameType, refTableName, historyTableDefinition))
        {
            return false;
        }
    }

    return true;
}

bool GameHistoryBootstrap::createBuiltInGameTable(const GameType &gameType) const
{
    char8_t tableName[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(tableName, sizeof(tableName), "%s_%s",
        gameType.getGameReportName().c_str(), RESERVED_GAMETABLE_NAME);
    blaze_strlwr(tableName);

    if (mExistingTableNames.find(tableName) != mExistingTableNames.end())
    {
        return verifyGameKeyTableColumns(gameType, tableName);
    }

    DbConnPtr dbConn = mComponent->getDbConnPtr(gameType.getGameReportName());
    if (dbConn == nullptr)
    {
        ERR_LOG("[GameHistoryBootstrap].createBuiltInGameTable: failed to connect to database, cannot continue.");
        return false;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(dbConn);

    if (query == nullptr)
    {
        ERR_LOG("[GameHistBootstrap].createBuiltInGameTable: could not grab new query object.");
        return false;
    }

    query->append("CREATE TABLE IF NOT EXISTS `$s` (\n", tableName);
    query->append("`game_id` bigint(20) unsigned NOT NULL\n");
    query->append(",`online` tinyint(1) unsigned DEFAULT 1\n");
    query->append(",`flags` bigint(20) unsigned DEFAULT 0\n");
    query->append(",`flag_reason` varchar(127) DEFAULT ''\n");
    query->append(",`total_gameref` int(6) unsigned NOT NULL DEFAULT 0, INDEX total_gameref_index (`game_id`,`total_gameref`)\n");
    query->append(",`timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP, INDEX timestamp_index (`timestamp`)\n");
    query->append(", PRIMARY KEY (`game_id`)\n");
    query->append(") ENGINE=InnoDB;");

    BlazeRpcError err = dbConn->executeQuery(query);

    if (err != DB_ERR_OK)
    {
        ERR_LOG("[GameHistBootstrap].createBuiltInGameTable: failed to add table '" << tableName << "', cannot continue.");
        return false;
    }

    TRACE_LOG("[GameHistBootstrap].createBuiltInGameTable: added table '" << tableName << "' successfully.");

    return true;
}

bool GameHistoryBootstrap::verifyGameKeyTableColumns(const GameType &gameType, const char8_t* tableName) const
{
    DbConnPtr dbConn = mComponent->getDbConnPtr(gameType.getGameReportName());
    if (dbConn == nullptr)
    {
        ERR_LOG("[GameHistoryBootstrap].verifyGameKeyTableColumns: failed to connect to database, cannot continue.");
        return false;
    }

    QueryPtr fetchColumnsQuery = DB_NEW_QUERY_PTR(dbConn);

    if (fetchColumnsQuery == nullptr)
    {
        ERR_LOG("[GameHistoryBootstrap].verifyGameKeyTableColumns: could not grab new query object.");
        return false;
    }

    fetchColumnsQuery->append("SELECT COLUMN_NAME from INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = '$s' and TABLE_NAME ='$s';",
        mComponent->getDbSchemaNameForGameType(gameType.getGameReportName()), tableName);

    DbResultPtr fetchColumnsResult;
    BlazeRpcError err = dbConn->executeQuery(fetchColumnsQuery, fetchColumnsResult);

    if (err != DB_ERR_OK)
    {
        ERR_LOG("[GameHistoryBootstrap].verifyGameKeyTableColumns: error fetching columns from DB, cannot continue");
        return false;
    }

    ColumnNameSet existingColumnNames;
    if (!fetchColumnsResult->empty())
    {
        DbResult::const_iterator itr = fetchColumnsResult->begin();
        DbResult::const_iterator end = fetchColumnsResult->end();

        for (; itr != end; ++itr)
        {
            const DbRow *row = *itr;
            const char8_t *columnName = row->getString("COLUMN_NAME");
            existingColumnNames.insert(columnName);
        }
    }

    QueryPtr addColumnsQuery = DB_NEW_QUERY_PTR(dbConn);
    if (addColumnsQuery == nullptr)
    {
        ERR_LOG("[GameHistBootstrap].verifyGameKeyTableColumns: failed to create new query object.");
        return false;
    }

    // Check if we need to add any reserved columns
    GamekeyTableReservedColumnsMap::const_iterator iter = mGamekeyTableReservedColumnsMap.begin();
    GamekeyTableReservedColumnsMap::const_iterator end = mGamekeyTableReservedColumnsMap.end();
    for (; iter != end; ++iter)
    {
        if (existingColumnNames.find(iter->first.c_str()) == existingColumnNames.end())
        {
            if (addColumnsQuery->isEmpty())
            {
                addColumnsQuery->append("ALTER TABLE `$s` ADD (`$s` $s", tableName, iter->first.c_str(), iter->second);
            }
            else
            {
                addColumnsQuery->append(", `$s` $s", iter->first.c_str(), iter->second);
            }
        }
    }

    if (!addColumnsQuery->isEmpty())
    {
        addColumnsQuery->append(");");
        err = dbConn->executeMultiQuery(addColumnsQuery);

        if (err != DB_ERR_OK)
        {
            ERR_LOG("[GameHistBootstrap].verifyGameKeyTableColumns: failed to add columns for table (" << tableName << "), cannot continue (err=" << getDbErrorString(err) << ")");
            return false;
        }
    }

    return true;
}

bool GameHistoryBootstrap::createBuiltInParticipantTable(const GameType &gameType) const
{
    char8_t tableName[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(tableName, sizeof(tableName), "%s_%s",
        gameType.getGameReportName().c_str(), RESERVED_PARTICIPANTTABLE_NAME);
    blaze_strlwr(tableName);

    char8_t builtInGameTable[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(builtInGameTable, sizeof(builtInGameTable), "%s_%s",
        gameType.getGameReportName().c_str(), RESERVED_GAMETABLE_NAME);
    blaze_strlwr(builtInGameTable);


    if (mExistingTableNames.find(tableName) != mExistingTableNames.end())
    {
        return true;
    }

    DbConnPtr dbConn = mComponent->getDbConnPtr(gameType.getGameReportName());
    if (dbConn == nullptr)
    {
        ERR_LOG("[GameHistoryBootstrap].createBuiltInParticipantTable: failed to connect to database, cannot continue.");
        return false;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(dbConn);

    if (query == nullptr)
    {
        ERR_LOG("[GameHistBootstrap].createBuiltInParticipantTable: could not grab new query object.");
        return false;
    }

    query->append("CREATE TABLE IF NOT EXISTS `$s` (\n", tableName);
    query->append("`game_id` bigint(20) unsigned NOT NULL\n");
    query->append(",`timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP, INDEX timestamp_index (timestamp)\n");
    query->append(",`entity_id` bigint(20) unsigned DEFAULT 0, INDEX entityid_timestamp_index (`entity_id`,`timestamp`)\n");
    query->append(", CONSTRAINT FOREIGN KEY (`game_id`) REFERENCES `$s` (`game_id`) ON DELETE CASCADE\n", builtInGameTable);
    query->append(", PRIMARY KEY (`game_id`, `entity_id`)\n");

    query->append(") ENGINE=InnoDB;");

    BlazeRpcError err = dbConn->executeQuery(query);

    if (err != DB_ERR_OK)
    {
        ERR_LOG("[GameHistBootstrap].createBuiltInParticipantTable: failed to add table '" << tableName << "', cannot continue.");
        return false;
    }

    TRACE_LOG("[GameHistBootstrap].createBuiltInParticipantTable: added table '" << tableName << "' successfully.");

    return true;
}

bool GameHistoryBootstrap::createBuiltInSubmitterTable(const GameType &gameType) const
{
    char8_t tableName[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(tableName, sizeof(tableName), "%s_%s",
        gameType.getGameReportName().c_str(), RESERVED_SUBMITTER_TABLE_NAME);
    blaze_strlwr(tableName);

    char8_t builtInGameTable[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(builtInGameTable, sizeof(builtInGameTable), "%s_%s",
        gameType.getGameReportName().c_str(), RESERVED_GAMETABLE_NAME);
    blaze_strlwr(builtInGameTable);


    if (mExistingTableNames.find(tableName) != mExistingTableNames.end())
    {
        return true;
    }

    DbConnPtr dbConn = mComponent->getDbConnPtr(gameType.getGameReportName());
    if (dbConn == nullptr)
    {
        ERR_LOG("[GameHistoryBootstrap].createBuiltInSubmitterTable: failed to connect to database, cannot continue.");
        return false;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(dbConn);

    if (query == nullptr)
    {
        ERR_LOG("[GameHistBootstrap].createBuiltInSubmitterTable: could not grab new query object.");
        return false;
    }

    query->append("CREATE TABLE IF NOT EXISTS `$s` (\n", tableName);
    query->append("`game_id` bigint(20) unsigned NOT NULL\n");
    query->append(",`timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP, INDEX timestamp_index (timestamp)\n");
    query->append(",`entity_id` bigint(20) unsigned NOT NULL\n");
    query->append(", CONSTRAINT FOREIGN KEY (`game_id`) REFERENCES `$s` (`game_id`) ON DELETE CASCADE\n", builtInGameTable);
    query->append(", PRIMARY KEY (`game_id`, `entity_id`)\n");

    query->append(") ENGINE=InnoDB;");

    BlazeRpcError err = dbConn->executeQuery(query);

    if (err != DB_ERR_OK)
    {
        ERR_LOG("[GameHistBootstrap].createBuiltInSubmitterTable: failed to add table '" << tableName << "', cannot continue.");
        return false;
    }

    TRACE_LOG("[GameHistBootstrap].createBuiltInSubmitterTable: added table '" << tableName << "' successfully.");

    return true;
}

bool GameHistoryBootstrap::createHistoryTable(const GameType &gameType, const char8_t *tableName,
                                              const HistoryTableDefinition* historyTableDefinition) const
{
    DbConnPtr dbConn = mComponent->getDbConnPtr(gameType.getGameReportName());
    if (dbConn == nullptr)
    {
        ERR_LOG("[GameHistoryBootstrap].createHistoryTable: failed to connect to database, cannot continue.");
        return false;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(dbConn);

    if (query == nullptr)
    {
        ERR_LOG("[GameHistBootstrap].createHistoryTable: could not grab new query object.");
        return false;
    }

    query->append("CREATE TABLE IF NOT EXISTS `$s` (\n", tableName);

    // first add built-in columns
    query->append("`game_id` bigint(20) unsigned NOT NULL\n");
    query->append(",`timestamp` timestamp NOT NULL default CURRENT_TIMESTAMP, INDEX timestamp_index (timestamp)\n");

    // now add the configuration based columns, if any
    HistoryTableDefinition::ColumnList::const_iterator columnIter, columnEnd;
    columnIter = historyTableDefinition->getColumnList().begin();
    columnEnd = historyTableDefinition->getColumnList().end();

    for (; columnIter != columnEnd; ++columnIter)
    {
        const char8_t* attributeName = columnIter->first.c_str();
        HistoryTableDefinition::ColumnType columnType = columnIter->second;
        switch (columnType)
        {
            case HistoryTableDefinition::INT:
                query->append(",`$s` int(11) NOT NULL DEFAULT 0\n", columnIter->first.c_str());
                break;
            case HistoryTableDefinition::UINT:
                query->append(",`$s` int(11) unsigned NOT NULL DEFAULT 0\n", attributeName);
                break;
            case HistoryTableDefinition::DEFAULT_INT:
                query->append(",`$s` bigint(20) NOT NULL DEFAULT 0\n", attributeName);
                break;
            case HistoryTableDefinition::ENTITY_ID:
                query->append(",`$s` bigint(20) unsigned DEFAULT 0\n", attributeName);
                break;
            case HistoryTableDefinition::FLOAT :
                query->append(",`$s` float NOT NULL DEFAULT 0\n", attributeName);
                break;
            case HistoryTableDefinition::STRING :
                query->append(",`$s` varchar(255) NOT NULL DEFAULT ''\n", attributeName);
                break;
            default : break;
        }
    }

    // key definitions
    char8_t builtInGameTable[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
    blaze_snzprintf(builtInGameTable, sizeof(builtInGameTable), "%s_%s", gameType.getGameReportName().c_str(), RESERVED_GAMETABLE_NAME);
    blaze_strlwr(builtInGameTable);

    query->append(", PRIMARY KEY (`game_id`");

    HistoryTableDefinition::PrimaryKeyList::const_iterator pkIter, pkEnd;
    pkIter = historyTableDefinition->getPrimaryKeyList().begin();
    pkEnd = historyTableDefinition->getPrimaryKeyList().end();

    for (; pkIter != pkEnd; ++pkIter)
    {
        query->append(", `$s`", pkIter->c_str());
    }

    query->append(")\n");

    // add keys/indexes for explicitly defined/configured "primary keys"
    // note that createIndexesFromMemory() will also add keys/indexes for filters used in queries and views
    for (pkIter = historyTableDefinition->getPrimaryKeyList().begin(); pkIter != pkEnd; ++pkIter)
    {
        query->append(", KEY `$s_index` (`$s`)", pkIter->c_str(), pkIter->c_str());
    }

    query->append(", CONSTRAINT FOREIGN KEY (`game_id`) REFERENCES `$s` (`game_id`) ON DELETE CASCADE\n", builtInGameTable);
    query->append(") ENGINE=InnoDB;");

    BlazeRpcError err = dbConn->executeQuery(query);

    if (err != DB_ERR_OK)
    {
        ERR_LOG("[GameHistBootstrap].createHistoryTable: failed to add table '" << tableName << "', cannot continue.");
        return false;
    }

    TRACE_LOG("[GameHistBootstrap].createHistoryTable: added table '" << tableName << "' successfully.");

    return true;
}

bool GameHistoryBootstrap::createGameRefTable(const GameType& gameType, const char8_t* tableName,
                                              const HistoryTableDefinition* historyTableDefinition) const
{
    DbConnPtr dbConn = mComponent->getDbConnPtr(gameType.getGameReportName());
    if (dbConn == nullptr)
    {
        ERR_LOG("[GameHistoryBootstrap].createGameRefTable: failed to connect to database, cannot continue.");
        return false;
    }

    if (historyTableDefinition->getPrimaryKeyList().empty())
    {
        TRACE_LOG("[GameHistBootstrap].createGameRefTable: game ref table for '" << tableName << "' is not required.");
        return true;
    }

    QueryPtr query = DB_NEW_QUERY_PTR(dbConn);

    if (query == nullptr)
    {
        ERR_LOG("[GameHistBootstrap].createGameRefTable: could not grab new query object.");
        return false;
    }

    query->append("CREATE TABLE IF NOT EXISTS `$s` (\n", tableName);

    HistoryTableDefinition::PrimaryKeyList::const_iterator pkIter, pkBegin, pkEnd;
    pkIter = pkBegin = historyTableDefinition->getPrimaryKeyList().begin();
    pkEnd = historyTableDefinition->getPrimaryKeyList().end();

    for (; pkIter != pkEnd; ++pkIter)
    {
        HistoryTableDefinition::ColumnList::const_iterator columnIter, columnEnd;
        columnIter = historyTableDefinition->getColumnList().find(pkIter->c_str());
        columnEnd = historyTableDefinition->getColumnList().end();

        if (pkIter != pkBegin)
        {
            query->append(",");
        }

        if (columnIter != columnEnd)
        {
            const char8_t* attributeName = columnIter->first.c_str();
            HistoryTableDefinition::ColumnType columnType = columnIter->second;
            switch (columnType)
            {
                case HistoryTableDefinition::INT:
                    query->append("`$s` int(11) NOT NULL DEFAULT 0\n", columnIter->first.c_str());
                    break;
                case HistoryTableDefinition::UINT:
                    query->append("`$s` int(11) unsigned NOT NULL DEFAULT 0\n", attributeName);
                    break;
                case HistoryTableDefinition::DEFAULT_INT:
                    query->append("`$s` bigint(20) NOT NULL DEFAULT 0\n", attributeName);
                    break;
                case HistoryTableDefinition::ENTITY_ID:
                    query->append("`$s` bigint(20) unsigned DEFAULT 0\n", attributeName);
                    break;
                case HistoryTableDefinition::FLOAT :
                    query->append("`$s` float NOT NULL DEFAULT 0\n", attributeName);
                    break;
                case HistoryTableDefinition::STRING :
                    query->append("`$s` varchar(255) NOT NULL DEFAULT ''\n", attributeName);
                    break;
                default : break;
            }
        }
    }

    uint32_t limit = historyTableDefinition->getHistoryLimit();

    for (uint32_t i = 0; i < limit; ++i)
    {
        query->append(",`$s$u` bigint(20) unsigned NULL DEFAULT 0\n", GAMEID_COLUMN_PREFIX, i);
    }

    pkIter = historyTableDefinition->getPrimaryKeyList().begin();

    query->append(", PRIMARY KEY (");

    for (; pkIter != pkEnd; ++pkIter)
    {
        query->append(" `$s`,", pkIter->c_str());
    }

    query->trim(1);
    query->append(")\n");
    query->append(") ENGINE=InnoDB;");

    BlazeRpcError err = dbConn->executeQuery(query);

    if (err != DB_ERR_OK)
    {
        ERR_LOG("[GameHistBootstrap].createGameRefTable: failed to add table, cannot continue.");
        return false;
    }

    TRACE_LOG("[GameHistBootstrap].createGameRefTable: created game ref table '" << tableName << "' successfully");

    return true;
}

bool GameHistoryBootstrap::verifyHistoryTableColumns(const GameType& gameType, const char8_t* tableName,
                                                     const HistoryTableDefinition* historyTableDefinition) const
{
    DbConnPtr dbConn = mComponent->getDbConnPtr(gameType.getGameReportName());
    if (dbConn == nullptr)
    {
        ERR_LOG("[GameHistoryBootstrap].verifyHistoryTableColumns: failed to connect to database, cannot continue.");
        return false;
    }

    QueryPtr fetchColumnsQuery = DB_NEW_QUERY_PTR(dbConn);

    if (fetchColumnsQuery == nullptr)
    {
        ERR_LOG("[GameHistoryBootstrap].verifyHistoryTableColumns: could not grab new query object.");
        return false;
    }

    fetchColumnsQuery->append("SELECT TABLE_NAME, COLUMN_NAME, COLUMN_DEFAULT, DATA_TYPE, COLUMN_KEY "
        "from INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = '$s' and TABLE_NAME = '$s';", mComponent->getDbSchemaNameForGameType(gameType.getGameReportName()), tableName);

    DbResultPtr fetchColumnsResult;
    BlazeRpcError error = dbConn->executeQuery(fetchColumnsQuery, fetchColumnsResult);

    if (error != DB_ERR_OK)
    {
        ERR_LOG("[GameHistoryBootstrap].verifyHistoryTableColumns: error fetching columns from DB, cannot continue");
        return false;
    }

    ColumnNameSet existingColumnNames;
    ColumnNameSet existingPrimaryKeys;

    if (!fetchColumnsResult->empty())
    {
        DbResult::const_iterator itr = fetchColumnsResult->begin();
        DbResult::const_iterator end = fetchColumnsResult->end();

        for (; itr != end; ++itr)
        {
            const DbRow *row = *itr;
            const char8_t *columnName = row->getString("COLUMN_NAME");
            const char8_t *columnKey = row->getString("COLUMN_KEY");

            // Skip past any core columns
            if (blaze_strcmp("game_id", columnName) == 0 ||
                blaze_strcmp("timestamp", columnName) == 0)
            {
                continue;
            }

            existingColumnNames.insert(columnName);

            if (blaze_strcmp("PRI", columnKey) == 0)
            {
                existingPrimaryKeys.insert(columnName);
            }
        }
    }

    QueryPtr modifyColumnsQuery = DB_NEW_QUERY_PTR(dbConn);

    if (modifyColumnsQuery == nullptr)
    {
        ERR_LOG("[GameHistBootstrap].verifyHistoryTableColumns: failed to create new query object.");
        return false;
    }

    // Check if we need to add any new columns
    HistoryTableDefinition::ColumnList::const_iterator columnIter = historyTableDefinition->getColumnList().begin();
    HistoryTableDefinition::ColumnList::const_iterator columnEnd = historyTableDefinition->getColumnList().end();

    for (; columnIter != columnEnd; ++columnIter)
    {
        if (existingColumnNames.find(columnIter->first.c_str()) == existingColumnNames.end())
        {
            modifyColumnsQuery->append("ALTER TABLE `$s` ADD COLUMN ", tableName);

            const char8_t *attributeName = columnIter->first.c_str();
            HistoryTableDefinition::ColumnType columnType = columnIter->second;

            switch (columnType)
            {
                case HistoryTableDefinition::INT:
                    modifyColumnsQuery->append("`$s` INT(11) NOT NULL DEFAULT 0;", attributeName);
                    break;
                case HistoryTableDefinition::UINT:
                    modifyColumnsQuery->append("`$s` INT(11) unsigned NOT NULL DEFAULT 0;", attributeName);
                    break;
                case HistoryTableDefinition::DEFAULT_INT:
                    modifyColumnsQuery->append("`$s` BIGINT(20) NOT NULL DEFAULT 0;", attributeName);
                    break;
                case HistoryTableDefinition::ENTITY_ID:
                    modifyColumnsQuery->append("`$s` BIGINT(20) unsigned;", attributeName);
                    break;
                case HistoryTableDefinition::FLOAT :
                    modifyColumnsQuery->append("`$s` FLOAT NOT NULL DEFAULT 0;", attributeName);
                    break;
                case HistoryTableDefinition::STRING :
                    modifyColumnsQuery->append("`$s` VARCHAR(255) NOT NULL DEFAULT '';", attributeName);
                    break;
                default : break;
            }
        }
    }

    if (!modifyColumnsQuery->isEmpty())
    {
        BlazeRpcError err = dbConn->executeMultiQuery(modifyColumnsQuery);

        if (err != DB_ERR_OK)
        {
            ERR_LOG("[GameHistBootstrap].verifyHistoryTableColumns: failed to add columns, cannot continue (err=" << getDbErrorString(err) << ")");
            return false;
        }
    }

    columnIter = historyTableDefinition->getColumnList().begin();
    columnEnd = historyTableDefinition->getColumnList().end();

    for (; columnIter != columnEnd; ++columnIter)
    {
        ColumnNameSet::const_iterator existedColIter = existingColumnNames.find(columnIter->first.c_str());
        if (existedColIter != existingColumnNames.end())
        {
            existingColumnNames.erase(existedColIter);
        }
    }

    // Check if we have to delete any columns
    if (!existingColumnNames.empty())
    {
        if (!mAllowDestructiveActions)
        {
            WARN_LOG("[GameHistBootstrap].verifyHistoryTableColumns: shutting down to prevent destructive DB action.");
            return false;
        }

        ColumnNameSet::const_iterator columnIt = existingColumnNames.begin();
        ColumnNameSet::const_iterator columnItEnd = existingColumnNames.end();

        QueryPtr dropColumnsQuery = DB_NEW_QUERY_PTR(dbConn);

        if (dropColumnsQuery == nullptr)
        {
            ERR_LOG("[GameHistBootstrap].verifyHistoryTableColumns: failed to create new query object.");
            return false;
        }

        for (; columnIt != columnItEnd; ++columnIt)
        {
            dropColumnsQuery->append("ALTER TABLE `$s` DROP COLUMN `$s`;", tableName, columnIt->c_str());
        }

        BlazeRpcError err = dbConn->executeMultiQuery(dropColumnsQuery);

        if (err != DB_ERR_OK)
        {
            ERR_LOG("[GameHistBootstrap].verifyHistoryTableColumns(): failed to drop removed columns (err=" << getDbErrorString(err) << ")");
            return false;
        }
    }

    existingColumnNames.clear();

    bool newKeysFound = false;

    // Check if any primary keys changes
    HistoryTableDefinition::PrimaryKeyList::const_iterator pkIter, pkEnd;
    pkIter = historyTableDefinition->getPrimaryKeyList().begin();
    pkEnd = historyTableDefinition->getPrimaryKeyList().end();

    for (; pkIter != pkEnd; ++pkIter)
    {
        ColumnNameSet::const_iterator existedPkIter = existingPrimaryKeys.find(pkIter->c_str());
        if (existedPkIter != existingPrimaryKeys.end())
        {
            existingPrimaryKeys.erase(pkIter->c_str());
        }
        else
        {
            newKeysFound = true;
        }
    }

    if (!existingPrimaryKeys.empty() || newKeysFound)
    {
        if (!mAllowDestructiveActions)
        {
            WARN_LOG("[GameHistBootstrap].verifyHistoryTableColumns: shutting down to prevent destructive DB action.");
            return false;
        }

        pkIter = historyTableDefinition->getPrimaryKeyList().begin();
        pkEnd = historyTableDefinition->getPrimaryKeyList().end();

        QueryPtr alterQuery = DB_NEW_QUERY_PTR(dbConn);

        if (alterQuery == nullptr)
        {
            ERR_LOG("[GameHistBootstrap].verifyHistoryTableColumns: failed to create new query object.");
            return false;
        }

        alterQuery->append("ALTER TABLE `$s` DROP PRIMARY KEY, ADD PRIMARY KEY (game_id,", tableName);

        for (; pkIter != pkEnd; ++pkIter)
        {
            alterQuery->append("`$s`,", pkIter->c_str());
        }

        alterQuery->trim(1);
        alterQuery->append(");");

        BlazeRpcError err = dbConn->executeQuery(alterQuery);

        if (err != DB_ERR_OK)
        {
            ERR_LOG("[GameHistBootstrap].verifyHistoryTableColumns(): failed to add or remove primary keys (err=" << getDbErrorString(err) << ")");
            return false;
        }
    }

    existingPrimaryKeys.clear();

    return true;
}

bool GameHistoryBootstrap::verifyGameRefTableColumns(const GameType& gameType, const char8_t* tableName,
                                                     const HistoryTableDefinition* historyTableDefinition) const
{
    DbConnPtr dbConn = mComponent->getDbConnPtr(gameType.getGameReportName());
    if (dbConn == nullptr)
    {
        ERR_LOG("[GameHistoryBootstrap].verifyGameRefTableColumns: failed to connect to database, cannot continue.");
        return false;
    }

    QueryPtr fetchColumnsQuery = DB_NEW_QUERY_PTR(dbConn);

    if (fetchColumnsQuery == nullptr)
    {
        ERR_LOG("[GameHistoryBootstrap].verifyHistoryTableColumns: could not grab new query object.");
        return false;
    }

    fetchColumnsQuery->append("SELECT TABLE_NAME, COLUMN_NAME, COLUMN_DEFAULT, DATA_TYPE, COLUMN_KEY "
        "from INFORMATION_SCHEMA.COLUMNS WHERE TABLE_SCHEMA = '$s' and TABLE_NAME = '$s';", mComponent->getDbSchemaNameForGameType(gameType.getGameReportName()), tableName);

    DbResultPtr fetchColumnsResult;
    BlazeRpcError error = dbConn->executeQuery(fetchColumnsQuery, fetchColumnsResult);

    if (error != DB_ERR_OK)
    {
        ERR_LOG("[GameHistoryBootstrap].verifyHistoryTableColumns: error fetching columns from DB, cannot continue");
        return false;
    }

    StringHashSet gameIdColumnNames, primaryKeyNames, extraColumnNames;

    uint32_t limit = historyTableDefinition->getHistoryLimit();

    for (uint32_t i = 0; i < limit; ++i)
    {
        eastl::string gameIdColumnName;
        gameIdColumnName.sprintf("%s%d", GAMEID_COLUMN_PREFIX, i);
        gameIdColumnNames.insert(gameIdColumnName.c_str());
    }

    HistoryTableDefinition::PrimaryKeyList::const_iterator pkIter, pkEnd;
    pkIter = historyTableDefinition->getPrimaryKeyList().begin();
    pkEnd = historyTableDefinition->getPrimaryKeyList().end();

    for (; pkIter != pkEnd; ++pkIter)
    {
        primaryKeyNames.insert(pkIter->c_str());
    }

    bool needPrimaryKeyUpdate = false;
    bool dropPrimaryKey = false;

    if (!fetchColumnsResult->empty())
    {
        DbResult::const_iterator itr = fetchColumnsResult->begin();
        DbResult::const_iterator end = fetchColumnsResult->end();

        for (; itr != end; ++itr)
        {
            const DbRow *row = *itr;
            const char8_t* columnName = row->getString("COLUMN_NAME");
            const char8_t *columnKey = row->getString("COLUMN_KEY");

            StringHashSet::const_iterator columnIter, gameIdIter;
            columnIter = primaryKeyNames.find(columnName);
            gameIdIter = gameIdColumnNames.find(columnName);

            // check if this column name is in the primary key list
            if (columnIter != primaryKeyNames.end())
            {
                primaryKeyNames.erase(columnIter);
                // check whether this column is already a primary key
                if (blaze_strcmp("PRI", columnKey) != 0)
                {
                    needPrimaryKeyUpdate = true;
                }
                continue;
            }

            // this column should not be a primary key
            if (blaze_strcmp("PRI", columnKey) == 0)
            {
                dropPrimaryKey = true;
            }

            // check if this column is game_id_x
            if (gameIdIter != gameIdColumnNames.end())
            {
                gameIdColumnNames.erase(gameIdIter);
            }
            // otherwise, this column is no longer needed
            else
            {
                extraColumnNames.insert(columnName);
            }
        }
    }

    QueryPtr modifyColumnsQuery = DB_NEW_QUERY_PTR(dbConn);

    if (modifyColumnsQuery == nullptr)
    {
        ERR_LOG("[GameHistBootstrap].verifyGameRefTableColumns: failed to create new query object.");
        return false;
    }

    StringHashSet::const_iterator columnIter, columnEnd;
    columnIter = primaryKeyNames.begin();
    columnEnd = primaryKeyNames.end();

    if (!primaryKeyNames.empty() || needPrimaryKeyUpdate)
    {
        modifyColumnsQuery->append("ALTER TABLE `$s` ", tableName);

        // no primary keys exist in the table
        if (dropPrimaryKey)
            modifyColumnsQuery->append("DROP PRIMARY KEY, ");

        for (; columnIter != columnEnd; ++columnIter)
        {
            HistoryTableDefinition::ColumnList::const_iterator colDefIter = 
                historyTableDefinition->getColumnList().find(columnIter->c_str());

            if (colDefIter != historyTableDefinition->getColumnList().end())
            {
                modifyColumnsQuery->append("ADD COLUMN ");

                const char8_t *attributeName = colDefIter->first.c_str();
                HistoryTableDefinition::ColumnType columnType = colDefIter->second;

                switch (columnType)
                {
                    case HistoryTableDefinition::INT:
                        modifyColumnsQuery->append("`$s` INT(11) NOT NULL DEFAULT 0", attributeName);
                        break;
                    case HistoryTableDefinition::UINT:
                        modifyColumnsQuery->append("`$s` INT(11) unsigned NOT NULL DEFAULT 0", attributeName);
                        break;
                    case HistoryTableDefinition::DEFAULT_INT:
                        modifyColumnsQuery->append(",`$s` BIGINT(20) NOT NULL DEFAULT 0", attributeName);
                        break;
                    case HistoryTableDefinition::ENTITY_ID:
                        modifyColumnsQuery->append("`$s` BIGINT(20) unsigned", attributeName);
                        break;
                    case HistoryTableDefinition::FLOAT :
                        modifyColumnsQuery->append("`$s` FLOAT NOT NULL DEFAULT 0", attributeName);
                        break;
                    case HistoryTableDefinition::STRING :
                        modifyColumnsQuery->append("`$s` VARCHAR(255) NOT NULL DEFAULT ''", attributeName);
                        break;
                    default : break;
                }

                modifyColumnsQuery->append(" FIRST, ");
            }
        }

        modifyColumnsQuery->append("ADD PRIMARY KEY (");

        pkIter = historyTableDefinition->getPrimaryKeyList().begin();
        pkEnd = historyTableDefinition->getPrimaryKeyList().end();

        for (; pkIter != pkEnd; ++pkIter)
        {
            modifyColumnsQuery->append("`$s`,", pkIter->c_str());
        }

        modifyColumnsQuery->trim(1);
        modifyColumnsQuery->append(");");
    }

    for (columnIter = gameIdColumnNames.begin(); columnIter != gameIdColumnNames.end(); ++columnIter)
    {
        modifyColumnsQuery->append("ALTER TABLE `$s` ADD COLUMN `$s` bigint(20) unsigned NULL DEFAULT 0;", tableName, columnIter->c_str());
    }

    if (!extraColumnNames.empty())
    {
        if (!mAllowDestructiveActions)
        {
            WARN_LOG("[GameHistBootstrap].verifyGameRefTableColumns: shutting down to prevent destructive DB action.");
            return false;
        }

        char8_t builtInGameTable[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
        blaze_snzprintf(builtInGameTable, sizeof(builtInGameTable), "%s_%s", gameType.getGameReportName().c_str(), RESERVED_GAMETABLE_NAME);

        QueryPtr updateGameRefQuery = DB_NEW_QUERY_PTR(dbConn);

        for (columnIter = extraColumnNames.begin(); columnIter != extraColumnNames.end(); ++columnIter)
        {
            // remove extra columns
            modifyColumnsQuery->append("ALTER TABLE `$s` DROP COLUMN `$s`;", tableName, columnIter->c_str());

            // update the total_gameref in gamekey table if the game_id columns are going to be dropped
            if (blaze_strncmp(columnIter->c_str(), GAMEID_COLUMN_PREFIX, strlen(GAMEID_COLUMN_PREFIX)) == 0)
            {
                updateGameRefQuery->append("UPDATE `$s` gk, (SELECT COUNT(`$s`) cnt, `$s` FROM `$s` GROUP BY `$s`) t SET total_gameref=total_gameref-t.cnt WHERE t.`$s`=gk.game_id;",
                    builtInGameTable, columnIter->c_str(), columnIter->c_str(), tableName, columnIter->c_str(), columnIter->c_str());
            }
        }

        if (!updateGameRefQuery->isEmpty())
        {
            BlazeRpcError err = dbConn->executeMultiQuery(updateGameRefQuery);

            if (err != DB_ERR_OK)
            {
                ERR_LOG("[GameHistBootstrap].verifyGameRefTableColumns: failed to update the game ref of game ids in dropped columns (err=" << getDbErrorString(err) << ")");
                return false;
            }
        }
    }

    if (!modifyColumnsQuery->isEmpty())
    {
        BlazeRpcError err = dbConn->executeMultiQuery(modifyColumnsQuery);

        if (err != DB_ERR_OK)
        {
            ERR_LOG("[GameHistBootstrap].verifyGameRefTableColumns: failed to add columns, cannot continue (err=" << getDbErrorString(err) << ")");
            return false;
        }
    }

    gameIdColumnNames.clear();
    primaryKeyNames.clear();
    extraColumnNames.clear();

    return true;
}

void GameHistoryBootstrap::buildIndexesInMemory(const GameType& gameType, const char8_t* filterName,
                                                const char8_t* returnType, const GameReportFilterList& filterList)
{
    // exit early if the filter list is empty
    if (filterList.empty())
        return;

    IndexesByName indexByTable;

    // group columns in filter list by game type name
    GameReportFilterList::const_iterator filterIter = filterList.begin();
    GameReportFilterList::const_iterator filterEnd = filterList.end();
    for (; filterIter != filterEnd; ++filterIter)
    {
        char8_t tableName[GameTypeReportConfig::GameHistory::MAX_TABLENAME_LEN];
        blaze_snzprintf(tableName, sizeof(tableName), "%s_%s%s", gameType.getGameReportName().c_str(),
            (blaze_stricmp((*filterIter)->getTable(), RESERVED_GAMETABLE_NAME) != 0) ? "hist_" : "", (*filterIter)->getTable());
        blaze_strlwr(tableName);

        IndexesByName::insert_return_type inserter = indexByTable.insert(tableName);
        Index& index = inserter.first->second;

        index.push_back((*filterIter)->getAttributeName());
    }

    // add the indexes needed for this filter to memory
    IndexesByName::iterator idxIter = indexByTable.begin();
    IndexesByName::iterator idxEnd = indexByTable.end();
    for (; idxIter != idxEnd; ++idxIter)
    {
        Index& newIndex = idxIter->second;

        // remove duplicate columns as index does not take duplicates
        newIndex.unique();

        if (!newIndex.empty())
        {
            IndexesByGameType::insert_return_type inserter = mIndexesInMemory.insert((eastl::string)gameType.getGameReportName());
            IndexesByTableName& indexByTableName = inserter.first->second;
            IndexesByName& indexesByName = indexByTableName.insert(idxIter->first).first->second;
            if (removeExtraIndexFromMap(indexesByName, newIndex))
            {
                eastl::string indexName;
                indexName.append_sprintf("%s-%s", filterName, returnType);
                indexesByName.insert(eastl::pair<eastl::string, Index>(indexName, newIndex));
            }
        }
    }
}

bool GameHistoryBootstrap::removeExtraIndexFromMap(IndexesByName& indexes, const Index& newIndex, bool* indexUsed/* = nullptr*/)
{
    IndexesByName::iterator idxIter = indexes.begin();
    while (idxIter != indexes.end())
    {
        bool discrepancyFound = false;

        Index& index = idxIter->second;
        Index::const_iterator colIter, newColIter;
        for (colIter = index.begin(), newColIter = newIndex.begin();
            colIter != index.end() && newColIter != newIndex.end(); ++colIter, ++newColIter)
        {
            if (blaze_stricmp((*colIter).c_str(), (*newColIter).c_str()) != 0)
            {
                discrepancyFound = true;
                break;
            }
        }

        if (!discrepancyFound)
        {
            // leftmost prefixes are matched
            if (newIndex.size() >= index.size())
            {
                // remove the existing index if the new index contains more than the required columns
                idxIter = indexes.erase(idxIter);
                if (indexUsed != nullptr)
                    *indexUsed = true;
                continue;
            }
            else
            {
                // the new index is not required as an existing index already covers the required columns
                return false;
            }
        }

        ++idxIter;
    }

    return true;
}

BlazeRpcError GameHistoryBootstrap::createIndexesFromMemory()
{
    IndexesByGameType::iterator idxMapIter = mIndexesInMemory.begin();
    for (; idxMapIter != mIndexesInMemory.end(); ++idxMapIter)
    {
        // Lookup shard by gameType
        DbConnPtr dbConn = mComponent->getDbConnPtr(idxMapIter->first.c_str());
        if (dbConn == nullptr)
        {
            ERR_LOG("[GameHistoryBootstrap].createIndexesFromMemory: Failed to connect to database for game type " << idxMapIter->first << ", cannot continue.");
            return ERR_DB_SYSTEM;
        }

        IndexesByTableName& tableMap = idxMapIter->second;
        IndexesByTableName::iterator idxTableMapIter = tableMap.begin();
        for (; idxTableMapIter != tableMap.end(); ++idxTableMapIter)
        {
            const char8_t* tableName = idxTableMapIter->first.c_str();
            IndexesByName& indexByName = idxTableMapIter->second;

            QueryPtr query = DB_NEW_QUERY_PTR(dbConn);
            DbResultPtr indexResult;

            query->append("SHOW INDEX FROM `$s`;", tableName);
            BlazeRpcError error = dbConn->executeQuery(query, indexResult);

            if (error != DB_ERR_OK)
            {
                WARN_LOG("[GameHistBootstrap].createIndexesFromMemory: Failed to show index from " 
                    << tableName << ">, err(" << getDbErrorString(error) << ")");
                return error;
            }

            IndexesByName indexesFromDb;

            // store the fetched indexes in a container grouped by key name for easy lookup
            DbResult::const_iterator resultIter = indexResult->begin();
            for (; resultIter != indexResult->end(); ++resultIter)
            {
                const DbRow *row = *resultIter;
                const char8_t* keyName = row->getString("Key_name");
                const char8_t* columnName = row->getString("Column_name");
                IndexesByName::insert_return_type inserter = indexesFromDb.insert(keyName);
                Index& indexedCols = inserter.first->second;
                indexedCols.push_back(columnName);
            }

            for (IndexesByName::const_iterator imIter = indexesFromDb.begin(); imIter != indexesFromDb.end(); ++imIter)
            {
                bool indexUsed = false;
                removeExtraIndexFromMap(indexByName, imIter->second, &indexUsed);

                // primary and timestamp index are built-in so they cannot be removed
                if (blaze_strcmp(imIter->first.c_str(), "PRIMARY") != 0 &&
                    blaze_strcmp(imIter->first.c_str(), "timestamp_index") != 0 && !indexUsed)
                {
                    // drop the current index and create a new index with more columns
                    DB_QUERY_RESET(query);
                    query->append("DROP INDEX `$s` ON `$s`;", imIter->first.c_str(), tableName);
                    error = dbConn->executeQuery(query);

                    if (error != DB_ERR_OK)
                    {
                        WARN_LOG("[GameHistBootstrap].createIndexesFromMemory: Failed to drop index (" <<
                            imIter->first.c_str() << ") on <" << tableName << "> due to err(" << getDbErrorString(error) << ")");
                        return error;
                    }
                }
            }

            // create a new index if no index exists on columns used in filter
            for (IndexesByName::iterator ilIter = indexByName.begin(); ilIter != indexByName.end(); ++ilIter)
            {
                DB_QUERY_RESET(query);
                query->append("CREATE INDEX `$s` ON `$s` (", ilIter->first.c_str(), tableName);

                Index& index = ilIter->second;
                for (Index::const_iterator sIter = index.begin(); sIter != index.end(); ++sIter)
                {
                    query->append("`$s`, ", (*sIter).c_str());
                }
                query->trim(2);
                query->append(");");
                error = dbConn->executeQuery(query);

                if (error != DB_ERR_OK)
                {
                    WARN_LOG("[GameHistBootstrap].createIndexesFromMemory: Failed to create index (" <<
                        ilIter->first.c_str() << " on <" << tableName << ">, err(" << getDbErrorString(error) << ")");
                    return error;
                }
            }
        }
    }

    return ERR_OK;
}

} // GameReporting
} // Blaze
