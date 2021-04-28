/*************************************************************************************************/
/*!
    \file   gdprcomplianceslaveimpl.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GdprComplianceSlaveImpl

    GdprCompliance Slave implementation.

    \note

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
// main include
#include "framework/blaze.h"
#include "gdprcomplianceslaveimpl.h"

// blaze includes
#include "framework/controller/controller.h"

#include "framework/replication/replicator.h"
#include "framework/replication/replicatedmap.h"

#include "framework/usersessions/usersessionmanager.h"
#include "framework/taskscheduler/taskschedulerslaveimpl.h"


#ifdef TARGET_stats
#include "stats/statsconfig.h"
#include "stats/statstypes.h"
#include "stats/tdf/stats_server.h"
#include "stats/rpc/statsslave.h"
#endif

#ifdef TARGET_associationlists
#include "associationlists/tdf/associationlists_server.h"
#include "associationlists/tdf/associationlists.h"
#include "associationlists/rpc/associationlistsslave.h"
#endif

#ifdef TARGET_gamereporting
#include "component/gamereporting/rpc/gamereportingslave.h"
#include "gamereporting/tdf/gamereporting_server.h"
#endif

#ifdef TARGET_clubs
#include "clubs/clubsslaveimpl.h"
#include "component/clubs/tdf/clubs_server.h"
#include "clubs/tdf/clubs.h"
#endif

#ifdef TARGET_util
#include "util/rpc/utilslave.h"
#include "component/util/tdf/util_server.h"
#endif

#ifdef TARGET_gamemanager
#include "gamemanager/tdf/gamemanagerconfig_server.h"
#include "gamemanager/rpc/gamemanagermaster.h"
#endif

#include "framework/connection/outboundconnectionmanager.h"
#include "framework/connection/outboundhttpservice.h"
#include "framework/oauth/accesstokenutil.h"

#include "proxycomponent/nucleusidentity/rpc/nucleusidentityslave.h"
#include "proxycomponent/nucleusidentity/tdf/nucleusidentity.h"
#include "framework/usersessions/userinfodb.h"

namespace Blaze
{
namespace GdprCompliance
{

const char8_t* GDPR_STATUS_IN_PROGRESS = "IN_PROGRESS";
const char8_t* GDPR_STATUS_FAILED = "FAILED";
const char8_t* GDPR_STATUS_COMPLETED = "COMPLETED";
const char8_t* GDPR_PULL_STR = "PULL";
const char8_t* GDPR_DEL_STR = "DELETE";

// static
GdprComplianceSlave* GdprComplianceSlave::createImpl()
{
    return BLAZE_NEW_NAMED("GdprComplianceSlaveImpl") GdprComplianceSlaveImpl();
}


/*** Public Methods ******************************************************************************/
GdprComplianceSlaveImpl::GdprComplianceSlaveImpl()
    : mPeriodicDeleteProcessTimerId(INVALID_TIMER_ID)
{
}

GdprComplianceSlaveImpl::~GdprComplianceSlaveImpl()
{
    gSelector->cancelTimer(mPeriodicDeleteProcessTimerId);
    mPeriodicDeleteProcessTimerId = INVALID_TIMER_ID;
}

bool GdprComplianceSlaveImpl::onConfigure()
{
    TRACE_LOG("[GdprComplianceSlaveImpl].configure: configuring component.");

    return (createGDPRTable() == ERR_OK);
}

bool GdprComplianceSlaveImpl::onActivate()
{
    schedulePeriodicDeleteProcess();
    return true;
}

#ifdef TARGET_gamereporting
bool findAllGameHistoryTables(const Blaze::GameReporting::GameTypeReportConfig& report, Blaze::GameReporting::GameTypeReportConfig::GameHistoryList& gameHistoryList)
{
    for (Blaze::GameReporting::GameTypeReportConfig::GameHistoryList::const_iterator it = report.getGameHistory().begin(); it != report.getGameHistory().end(); ++it)
    {        
        if (blaze_stricmp((*it)->getTable(), "player") == 0)
        {
            (*it)->copyInto(*gameHistoryList.pull_back());
        }
    }

    for (Blaze::GameReporting::GameTypeReportConfig::SubreportsMap::const_iterator it = report.getSubreports().begin(); it != report.getSubreports().end(); ++it)
    {
        if (findAllGameHistoryTables(*(it->second), gameHistoryList))
            return true;
    }
    return false;
}
#endif

#ifdef TARGET_stats
bool getStatsDb(const Blaze::Stats::StatsConfig& statsConfig, BlazeId id, eastl::string& statsDb)
{
    statsDb = statsConfig.getDbName();

#ifdef BLAZE_STATS_SHARD_H        
    eastl::vector<eastl::string> cacheList;
    const Blaze::Stats::ShardInfoList& shardInfoList = statsConfig.getShardInfoList();
    // Go through the config, if shard is enabled, and type is for user, save db names into cache
    Blaze::Stats::ShardInfoList::const_iterator itr = shardInfoList.begin();
    Blaze::Stats::ShardInfoList::const_iterator end = shardInfoList.end();
    for (; itr != end; ++itr)
    {
        const Blaze::Stats::ShardInfo* shardInfo = *itr;
        EA::TDF::ObjectType entityType = shardInfo->getEntityType();
        const Blaze::Stats::DbNameList& list = shardInfo->getDbNames();
        if (list.empty())
        {
            WARN_LOG("[StatsConfigData].initShards(): empty shard db name list");
            continue;
        }

        if (entityType == ENTITY_TYPE_USER)
        {
            for (Blaze::Stats::DbNameList::const_iterator dbItr = list.begin(); dbItr != list.end(); ++dbItr)
            {
                cacheList.push_back(dbItr->c_str());
            }
            break;
        }
    }

    if (!cacheList.empty())
    {
        // find shard number from stats default db
        DbConnPtr readConn = gDbScheduler->getLagFreeReadConnPtr(gDbScheduler->getDbId(statsConfig.getDbName()));
        if (readConn != nullptr)
        {
            QueryPtr query = DB_NEW_QUERY_PTR(readConn);
            if (query != nullptr)
            {
                DbResultPtr dbResult;
                query->append("SELECT `shard` from `stats_shard_usersessions_user` where `entity_id` = $D", id);
                BlazeRpcError err = readConn->executeQuery(query, dbResult);
                if (err != ERR_OK)
                {
                    ERR_LOG("[GdprComplianceSlaveImpl].exeDataPullQueries: Error occurred executing find stats shard query [" << getDbErrorString(err) << "]");
                    return false;
                }

                if (!dbResult->empty())
                {
                    DbRow *row = *dbResult->begin();
                    uint32_t shard = row->getUInt("shard");
                    if (shard >= cacheList.size())
                    {
                        ERR_LOG("[GdprComplianceSlaveImpl].getStatsDb: Illegal shard value " << shard
                            << " for entityId " << id);
                        return false;
                    }
                    statsDb = cacheList.at(shard);
                }
            }
            else
            {
                ERR_LOG("[GdprComplianceSlaveImpl].exeDataPullQueries: Failed to create new query");
                return false;
            }
        }
        else
        {
            ERR_LOG("[GdprComplianceSlaveImpl].exeDataPullQueries: Failed to obtain connection");
            return false;
        }
    }

#endif
    return true;
}
#endif

BlazeRpcError GdprComplianceSlaveImpl::pullUserDataFromDBsByBlazeId(BlazeId blazeId, UserData* userData)
{
    DataPullQueriesByDBIdMap dataPullQueriesByDBIdMap;
    ClientPlatformType platform = userData->getPlatform();
    Component* component = nullptr;

#ifdef TARGET_gamereporting
    if (gController->getComponent(Blaze::GameReporting::GameReportingSlave::COMPONENT_ID, false) == nullptr)
    {
        TRACE_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: No gamereporting component found.");
    }
    else 
    {
        // generate GameHistory Queries from config
        // getConfigTdf will fetch the latest config from configMaster each time
        Blaze::GameReporting::GameReportingConfig gameReportingConfig;
        BlazeRpcError rc = gController->getConfigTdf("gamereporting", false, gameReportingConfig);
        if (rc != ERR_OK)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: Failure fetching config for game history, error=" << ErrorHelp::getErrorName(rc));
            return ERR_SYSTEM;
        }

        const Blaze::GameReporting::GameTypeConfigMap& gameTypes = gameReportingConfig.getGameTypes();
        for (Blaze::GameReporting::GameTypeConfigMap::const_iterator iter = gameTypes.begin(),
            end = gameTypes.end();
            iter != end;
            ++iter)
        {
            const GameManager::GameReportName& gameType = iter->first;
            Blaze::GameReporting::GameTypeReportConfig::GameHistoryList gameHistoryList;
            const Blaze::GameReporting::GameTypeReportConfig& gameTypeCfg = iter->second->getReport();
            findAllGameHistoryTables(gameTypeCfg, gameHistoryList);

            if (gameHistoryList.empty())
            {
                // this gameType doesn't track any "player" data
                continue;
            }

            // just found all the "player" table components (report sections) for this gameType
            char8_t tableName[128];
            blaze_snzprintf(tableName, sizeof(tableName), "%s_hist_player", gameType.c_str());

            eastl::string blazeIdColumn;
            const char8_t* grDefaultDb = gameReportingConfig.getDbName();
            if (*(iter->second->getDbName()) != '\0')
            {
                grDefaultDb = iter->second->getDbName(); //GameReport DB sharding
            }
            uint32_t dbId = gDbScheduler->getDbId(grDefaultDb);
            DataPullQuery& dataPullQuery = dataPullQueriesByDBIdMap[dbId].push_back();

            // note that the column descriptions here are the same as the DB column names
            dataPullQuery.columnDescList.push_back("timestamp");
            eastl::vector<eastl::string>::iterator colItr, colEnd;

            // because a gameType only has one "player" table, collect the columns from all the separate report sections (for "player") into one data pull for this gameType
            for (Blaze::GameReporting::GameTypeReportConfig::GameHistoryList::const_iterator it = gameHistoryList.begin(); it != gameHistoryList.end(); ++it)
            {
                for (Blaze::GameReporting::GameTypeReportConfig::ReportValueByAttributeMap::const_iterator varItr = (*it)->getColumns().begin(); varItr != (*it)->getColumns().end(); ++varItr)
                {
                    if (blaze_stricmp(varItr->second, "$INDEX$") == 0)
                    {
                        blazeIdColumn = varItr->first;
                        continue;
                    }

                    // add if not already in list
                    colItr = dataPullQuery.columnDescList.begin();
                    colEnd = dataPullQuery.columnDescList.end();
                    for (; colItr != colEnd; ++colItr)
                    {
                        if (blaze_strcmp((*colItr).c_str(), varItr->first.c_str()) == 0)
                        {
                            break;
                        }
                    }
                    if (colItr == colEnd)
                    {
                        dataPullQuery.columnDescList.push_back(varItr->first.c_str());
                    }
                }
            }

            if (blazeIdColumn.empty())
            {
                WARN_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: could not find blazeId column for game history table " << tableName);
                dataPullQueriesByDBIdMap[dbId].pop_back();
                continue;
            }

            StringBuilder queryStr("SELECT ");

            colItr = dataPullQuery.columnDescList.begin();
            colEnd = dataPullQuery.columnDescList.end();
            for (; colItr != colEnd; ++colItr)
            {
                queryStr.append("`%s`,", (*colItr).c_str());
            }
            queryStr.trim(1); // trim last comma

            queryStr.append(" FROM %s WHERE %s=", tableName, blazeIdColumn.c_str());

            dataPullQuery.tableDesc.append_sprintf("Game History for %s", gameType.c_str());
            dataPullQuery.queryStr = queryStr.get();
        }
    }
#endif   

#ifdef TARGET_stats
    if (gController->getComponent(Blaze::Stats::StatsSlave::COMPONENT_ID, false) == nullptr)
    {
        TRACE_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: No Stats component found.");
    }
    else
    {
        // generate Stats Queries from config
        Blaze::Stats::StatsConfig statsConfig;
        BlazeRpcError rc = gController->getConfigTdf("stats", false, statsConfig);
        if (rc != ERR_OK)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: Failure fetching config for stats, error=" << ErrorHelp::getErrorName(rc));
            return ERR_SYSTEM;
        }
        else
        {
            eastl::string statsDB;
            bool success = getStatsDb(statsConfig, blazeId, statsDB);
            if (!success)
            {
                return ERR_SYSTEM;
            }
            uint32_t dbId = gDbScheduler->getDbId(statsDB.c_str());

            // generate table names, and columns
            Blaze::Stats::CategoriesList::const_iterator categoryItr = statsConfig.getCategories().begin();
            for (; categoryItr != statsConfig.getCategories().end(); categoryItr++)
            {
                // category 
                int32_t periodTypes = (*categoryItr)->getPeriodTypes();
                if (blaze_stricmp((*categoryItr)->getEntityType(), "usersessions.user") == 0) {
                    // get columns
                    eastl::vector<const char8_t*> columns;
                    Blaze::Stats::StatList::const_iterator colItr = (*categoryItr)->getStats().begin();
                    for (; colItr != (*categoryItr)->getStats().end(); colItr++)
                    {
                        const char8_t* statname = (*colItr)->getName();
                        if (statname[0] != '\0')
                        {
                            columns.push_back(statname);
                        }
                    }

                    // get scopes
                    Blaze::Stats::ScopeList::const_iterator scopeItr = (*categoryItr)->getScope().begin();
                    for (; scopeItr != (*categoryItr)->getScope().end(); scopeItr++)
                    {
                        columns.push_back(*scopeItr);
                    }

                    // construct queries
                    // get full names for a usersession category
                    for (int32_t i = 0; i < Blaze::Stats::STAT_NUM_PERIODS; ++i)
                    {
                        if ((1 << i) & periodTypes)
                        {
                            char8_t tableName[Blaze::Stats::STATS_MAX_TABLE_NAME_LENGTH];
                            blaze_snzprintf(tableName, Blaze::Stats::STATS_MAX_TABLE_NAME_LENGTH, "stats_%s_%s", Blaze::Stats::STAT_PERIOD_NAMES[i], (*categoryItr)->getName());
                            blaze_strlwr(tableName);

                            DataPullQuery dataPullQuery;
                            dataPullQuery.tableDesc.append_sprintf("Player %s %s Stats", Blaze::Stats::STAT_PERIOD_NAMES[i], (*categoryItr)->getName());

                            StringBuilder concatenatedColumns;
                            for (size_t j = 0; j < columns.size(); ++j)
                            {
                                dataPullQuery.columnDescList.push_back(columns.at(j));
                                concatenatedColumns.append("`%s`,", columns.at(j));
                            }
                            concatenatedColumns.trim(1); // trim last comma

                            StringBuilder queryStr;
                            queryStr.append("SELECT %s FROM %s WHERE `entity_id`=", concatenatedColumns.get(), tableName);

                            dataPullQuery.queryStr = queryStr.get();
                            dataPullQueriesByDBIdMap[dbId].push_back(dataPullQuery);
                        }
                    }
                }
            }
        }
    }
#endif

#ifdef TARGET_clubs
    component = gController->getComponent(Blaze::Clubs::ClubsSlave::COMPONENT_ID, false);
    if (component == nullptr)
    {
        TRACE_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: No Clubs component found.");
    }
    else
    {
        // generate Club Queries from config
        Blaze::Clubs::ClubsConfig clubsCfg;
        BlazeRpcError rc = gController->getConfigTdf("clubs", false, clubsCfg);
        if (rc != ERR_OK)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: Failure fetching config for club, error=" << ErrorHelp::getErrorName(rc));
            return ERR_SYSTEM;
        }

        // Clubs code uses a single DB for all info, even in a shared cluster setup. 
        if (clubsCfg.getDbNamesByPlatform().find(gController->getDefaultClientPlatform()) == clubsCfg.getDbNamesByPlatform().end())
        {
            WARN_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: No Clubs DB config found for platform "<< ClientPlatformTypeToString(gController->getDefaultClientPlatform()) << ".");
        }
        else
        {
            uint32_t dbId = gDbScheduler->getDbId(clubsCfg.getDbNamesByPlatform().find(gController->getDefaultClientPlatform())->second.c_str());

            // Player's clubs
            DataPullQuery clubsQuery;
            clubsQuery.tableDesc = "Player's clubs";
            clubsQuery.columnDescList.push_back("club name");
            clubsQuery.columnDescList.push_back("membership status: 0:Member, 1:GM, 2:Owner");
            clubsQuery.columnDescList.push_back("member since");

            StringBuilder clubStr;
            clubStr.append("SELECT clubs_clubs.`name`, clubs_members.`membershipStatus`, clubs_members.`memberSince` FROM clubs_clubs INNER JOIN clubs_members ON clubs_clubs.`clubId` = clubs_members.`clubId` AND clubs_members.`blazeId`=");
            clubsQuery.queryStr = clubStr.get();
            dataPullQueriesByDBIdMap[dbId].push_back(clubsQuery);

            // Player's Ban Club
            DataPullQuery banQuery;
            banQuery.tableDesc = "Player's ban clubs";
            banQuery.columnDescList.push_back("club name");
            banQuery.columnDescList.push_back("Ban status");

            StringBuilder banStr;
            banStr.append("SELECT clubs_clubs.`name`, clubs_bans.`banStatus` FROM clubs_clubs INNER JOIN clubs_bans ON clubs_clubs.`clubId` = clubs_bans.`clubId` AND clubs_bans.`userId`=");
            banQuery.queryStr = banStr.get();
            dataPullQueriesByDBIdMap[dbId].push_back(banQuery);

            // Player's Domain
            DataPullQuery& domainQuery = dataPullQueriesByDBIdMap[dbId].push_back();
            domainQuery.tableDesc = "Player's club domain";

            // Populate clubs domain description
            const Blaze::Clubs::ClubDomainList &domainList = clubsCfg.getDomains();
            Blaze::Clubs::ClubDomainList::const_iterator it = domainList.begin();
            Blaze::Clubs::ClubDomainList::const_iterator itend = domainList.end();
            StringBuilder domainDesc;
            for (int cnt = 1; it != itend; ++it, ++cnt)
            {
                uint32_t domainId = static_cast<uint32_t>((*it)->getClubDomainId());
                domainDesc.append("%d:%s,", domainId, (*it)->getDomainName());
            }
            domainDesc.trim(1); // trim last comma
            domainQuery.columnDescList.push_back(domainDesc.get());

            StringBuilder domainStr;
            domainStr.append("SELECT `domainId` FROM clubs_memberdomains where `blazeId`=");
            domainQuery.queryStr = domainStr.get();
        }

    }
#endif

#ifdef TARGET_util
    component = gController->getComponent(Blaze::Util::UtilSlave::COMPONENT_ID, false);
    if (component == nullptr)
    {
        TRACE_LOG("[GdprComplianceSlaveImpl].genQueriesFromCfg: No Util component found.");
    }
    else
    {
        // user small storage
        Blaze::Util::UtilConfig utilCfg;
        BlazeRpcError rc = gController->getConfigTdf("util", false, utilCfg);
        if (rc != ERR_OK)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: Failure fetching config for util, error=" << ErrorHelp::getErrorName(rc));
            return ERR_SYSTEM;
        }

        // The Util Config should have platform information for each platform. 
        if (utilCfg.getDbNamesByPlatform().find(platform) == utilCfg.getDbNamesByPlatform().end())
        {
            ERR_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: No Util DB config found for platform " << ClientPlatformTypeToString(platform) << ".");
            return ERR_SYSTEM;
        }

        uint32_t dbId = gDbScheduler->getDbId(utilCfg.getDbNamesByPlatform().find(platform)->second.c_str());

        DataPullQuery& smallStorageQuery = dataPullQueriesByDBIdMap[dbId].push_back();
        smallStorageQuery.tableDesc = "User Small Storage";
        smallStorageQuery.columnDescList.push_back("key");
        smallStorageQuery.columnDescList.push_back("settings");
        smallStorageQuery.columnDescList.push_back("last modified date");
        smallStorageQuery.columnDescList.push_back("created date");
        StringBuilder storageStr;
        storageStr.append("SELECT `key`, `data`, `last_modified_date`, `created_date` FROM `util_user_small_storage` WHERE `id`=");
        smallStorageQuery.queryStr = storageStr.get();

        // Get the value from the common DB:   (if this is not a single platform server)
        if (utilCfg.getDbNamesByPlatform().find(gController->getDefaultClientPlatform()) != utilCfg.getDbNamesByPlatform().end())
        {
            dbId = gDbScheduler->getDbId(utilCfg.getDbNamesByPlatform().find(gController->getDefaultClientPlatform())->second.c_str());

            DataPullQuery& telemetyOptInQuery = dataPullQueriesByDBIdMap[dbId].push_back();
            telemetyOptInQuery.tableDesc = "Util User Options";
            telemetyOptInQuery.columnDescList.push_back("telemetry opt-in");
            StringBuilder telemStr;
            telemStr.append("SELECT `telemetryopt` FROM `util_user_options` WHERE `blazeid`=");
            telemetyOptInQuery.queryStr = telemStr.get();
        }
    }
#endif    

    // config tables
    const DataPullDbTableList& list = getConfig().getDataPullDbTables();
    for (DataPullDbTableList::const_iterator itr = list.begin(); itr != list.end(); itr++)
    {
        if ((*itr)->getBlazeIdColumnAliasAsTdfString().empty())
            continue;

        // For shared DB systems, try to use the default client platform:
        ClientPlatformType tempPlatform = platform;
        if ((*itr)->getDbNamesByPlatform().size() == 1 && (*itr)->getDbNamesByPlatform().begin()->first == gController->getDefaultClientPlatform())
            tempPlatform = gController->getDefaultClientPlatform();

        DbNameByPlatformTypeMap::const_iterator nameItr = (*itr)->getDbNamesByPlatform().find(tempPlatform);
        if (nameItr == (*itr)->getDbNamesByPlatform().end())
        {
            INFO_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: Skipping data pull from table '" << (*itr)->getTableName() << "' for blazeId " << blazeId << " - no db configured for platform '" << ClientPlatformTypeToString(tempPlatform) << "'");
            continue;
        }

        uint32_t dbId = gDbScheduler->getDbId(nameItr->second.c_str());
        DataPullQuery& table = dataPullQueriesByDBIdMap[dbId].push_back();
        table.tableDesc = (*itr)->getDesc();
        const ColumnDescriptionByNameMap& colList = (*itr)->getColumnDescriptionByNameMap();

        // Build the query in the data pull map. Avoid these escaped characters: \x00 (ASCII 0, NUL), \n, \r, \, ', ", and \x1a (Control-Z)
        StringBuilder qStr("SELECT ");
        for (ColumnDescriptionByNameMap::const_iterator colItr = colList.begin(); colItr != colList.end(); colItr++)
        {
            table.columnDescList.push_back(colItr->second.c_str());
            qStr.append("`%s`,", colItr->first.c_str());
        }
        qStr.trim(1);
        qStr.append(" FROM `%s` WHERE `%s`=", (*itr)->getTableName(), (*itr)->getBlazeIdColumnAlias());
        table.queryStr = qStr.get();
    }

    BlazeRpcError err = pullFromUserInfo(blazeId, userData);
    if (err == ERR_OK)
        err = runDataPullQueries(blazeId, dataPullQueriesByDBIdMap, userData);

    if (err != ERR_OK)
    {
        return err;
    }

#ifdef TARGET_associationlists
    err = pullFromAssociationlist(blazeId, userData);
#endif

    return err;
}

BlazeRpcError GdprComplianceSlaveImpl::pullUserDataFromDBsByAccountId(AccountId accountId, UserData* userData)
{
    DataPullQueriesByDBIdMap dataPullQueriesByDBIdMap;
    ClientPlatformType platform = userData->getPlatform();

    // Account Info DB:
    {
        uint32_t dbId = gUserSessionManager->getDbId();

        DataPullQuery& query = dataPullQueriesByDBIdMap[dbId].push_back();
        query.tableDesc = "Account Info";
        query.columnDescList.push_back("last platform used");
        
        // Other info like first/last login dates can be found by min/maxing he per-platform times.  No need to dupe here. 
        StringBuilder storageStr;
        storageStr.append("SELECT `lastplatformused` FROM `accountinfo` WHERE `accountid`="); 
        query.queryStr = storageStr.get();
    }


    // config tables
    const DataPullDbTableList& list = getConfig().getDataPullDbTables();
    for (DataPullDbTableList::const_iterator itr = list.begin(); itr != list.end(); itr++)
    {
        if ((*itr)->getAccountIdColumnAliasAsTdfString().empty())
            continue;

        // For shared DB systems, try to use the default client platform:
        ClientPlatformType tempPlatform = platform;
        if ((*itr)->getDbNamesByPlatform().size() == 1 && (*itr)->getDbNamesByPlatform().begin()->first == gController->getDefaultClientPlatform())
            tempPlatform = gController->getDefaultClientPlatform();

        DbNameByPlatformTypeMap::const_iterator nameItr = (*itr)->getDbNamesByPlatform().find(tempPlatform);
        if (nameItr == (*itr)->getDbNamesByPlatform().end())
        {
            INFO_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: Skipping data pull from table '" << (*itr)->getTableName() << "' for accountId " << accountId << " - no db configured for platform '" << ClientPlatformTypeToString(tempPlatform) << "'");
            continue;
        }

        uint32_t dbId = gDbScheduler->getDbId(nameItr->second.c_str());
        DataPullQuery& table = dataPullQueriesByDBIdMap[dbId].push_back();
        table.tableDesc = (*itr)->getDesc();
        const ColumnDescriptionByNameMap& colList = (*itr)->getColumnDescriptionByNameMap();

        // Build the query in the data pull map. Avoid these escaped characters: \x00 (ASCII 0, NUL), \n, \r, \, ', ", and \x1a (Control-Z)
        StringBuilder qStr("SELECT ");
        for (ColumnDescriptionByNameMap::const_iterator colItr = colList.begin(); colItr != colList.end(); colItr++)
        {
            table.columnDescList.push_back(colItr->second.c_str());
            qStr.append("`%s`,", colItr->first.c_str());
        }
        qStr.trim(1);
        qStr.append(" FROM `%s` WHERE `%s`=", (*itr)->getTableName(), (*itr)->getAccountIdColumnAlias());
        table.queryStr = qStr.get();
    }

    return runDataPullQueries(accountId, dataPullQueriesByDBIdMap, userData);
}
BlazeRpcError GdprComplianceSlaveImpl::pullFromUserInfo(BlazeId blazeId, UserData* userData)
{
    DbConnPtr readConn = gDbScheduler->getLagFreeReadConnPtr(gUserSessionManager->getDbId(userData->getPlatform()));
    if (readConn == nullptr)
    {
        ERR_LOG("[GdprComplianceSlaveImpl].pullFromUserInfo: Failed to get database connection for user (" << blazeId << "), platform '" << ClientPlatformTypeToString(userData->getPlatform()) << "'");
        return ERR_SYSTEM;
    }

    QueryPtr queryBuf = DB_NEW_QUERY_PTR(readConn);
    queryBuf->append("SELECT `persona`,`accountlocale`,`accountcountry`,`canonicalpersona`,`latitude`,`longitude`,`geoopt`,`firstlogindatetime`,`lastlogindatetime`,`previouslogindatetime`,`lastlocaleused`,`lastcountryused`,`childaccount`,`crossplatformopt`, `isprimarypersona` from `userinfo` where `blazeid`= $D;", blazeId);
    DbResultPtr result;
    BlazeRpcError err = readConn->executeQuery(queryBuf, result);
    if (err == ERR_OK)
    {
        if (result->size() != 1)
        {
            WARN_LOG("[GdprComplianceSlaveImpl].pullFromUserInfo: Found " << result->size() << " userinfo entries for user (" << blazeId << "), platform '" << ClientPlatformTypeToString(userData->getPlatform()) << "'");
        }
        for (DbResult::const_iterator it = result->begin(), end = result->end(); it != end; ++it)
        {
            DbRow *row = *it;
            TableData* tableData = userData->getTableDataList().pull_back();
            tableData->setTableDesc("Player Information");
            RowPtr colmnRow = tableData->getRows().pull_back();
            colmnRow->getRowData().push_back("Player Name");
            colmnRow->getRowData().push_back("Account Locale");
            colmnRow->getRowData().push_back("Account Country");
            colmnRow->getRowData().push_back("Player Canonical Name");
            colmnRow->getRowData().push_back("Latitude");
            colmnRow->getRowData().push_back("Longitude");
            colmnRow->getRowData().push_back("Geo Opt-In");
            colmnRow->getRowData().push_back("First Login Time");
            colmnRow->getRowData().push_back("Last Login Time");
            colmnRow->getRowData().push_back("Previous Login Time");
            colmnRow->getRowData().push_back("Last Locale Used");
            colmnRow->getRowData().push_back("Last Country Used");
            colmnRow->getRowData().push_back("Is Child Account");
            colmnRow->getRowData().push_back("Crossplay Opt In");
            colmnRow->getRowData().push_back("Is Primary Persona");

            RowPtr dataRow = tableData->getRows().pull_back();
            size_t colSize = colmnRow->getRowData().size();
            //loop count should be equal to query size 
            for (size_t i = 0; i < colSize; i++)
            {
                dataRow->getRowData().push_back(row->getString(i));
            }
            colmnRow->getRowData().push_back("Player Platform");
            dataRow->getRowData().push_back(ClientPlatformTypeToString(userData->getPlatform()));
        }
    }
    else
    {
        ERR_LOG("[GdprComplianceSlaveImpl].pullFromUserInfo: Error occurred executing query [" << getDbErrorString(err) << "]");
        return ERR_SYSTEM;
    }
    return ERR_OK;
}

#ifdef TARGET_associationlists
BlazeRpcError GdprComplianceSlaveImpl::pullFromAssociationlist(BlazeId blazeId, UserData* userData)
{
    Component* component = gController->getComponent(Blaze::Association::AssociationListsSlave::COMPONENT_ID, false);
    if (component == nullptr)
    {
        TRACE_LOG("[GdprComplianceSlaveImpl].pullFromAssociationlist: No Association component found.");
    }
    else
    {
        // generate Association list Queries from config    
        Blaze::Association::AssociationlistsConfig associationListCfg;
        BlazeRpcError rc = gController->getConfigTdf("associationlists", false, associationListCfg);
        if (rc != ERR_OK)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].pullFromAssociationlist: Failure fetching config for associationlists, error="
                << ErrorHelp::getErrorName(rc));
            return ERR_SYSTEM;
        }

        if (associationListCfg.getDbNamesByPlatform().find( gController->getDefaultClientPlatform() ) == associationListCfg.getDbNamesByPlatform().end())
        {
            ERR_LOG("[GdprComplianceSlaveImpl].pullFromAssociationlist: No AssocList DB config found for platform " << ClientPlatformTypeToString(gController->getDefaultClientPlatform()) << ".");
            return ERR_SYSTEM;
        }

        uint32_t dbId = gDbScheduler->getDbId(associationListCfg.getDbNamesByPlatform().find(gController->getDefaultClientPlatform())->second.c_str());

        // Fetch player's association lists, and lookup persona from userinfo table for each player's friend.
        const Blaze::Association::ListDataVector& assocLists = associationListCfg.getLists();
        for (Blaze::Association::ListDataVector::const_iterator it = assocLists.begin(); it != assocLists.end(); ++it)
        {
            const Blaze::Association::ListData* listData = *it;
            // list name
            if (listData->getName()[0] != '\0')
            {
                BlazeIdVector blazeids;
                DbConnPtr readConn = gDbScheduler->getLagFreeReadConnPtr(dbId);
                if (readConn != nullptr)
                {
                    QueryPtr queryBuf = DB_NEW_QUERY_PTR(readConn);
                    queryBuf->append("SELECT `memberblazeid`, `dateadded`, `lastupdated` FROM `user_association_list_$s` WHERE `blazeid`=$D", listData->getName(), blazeId);
                    DbResultPtr result;
                    BlazeRpcError err = readConn->executeQuery(queryBuf, result);
                    if (err == ERR_OK)
                    {
                        // Fetch a list of blaze ids 
                        for (DbResult::const_iterator rowIt = result->begin(); rowIt != result->end(); ++rowIt)
                        {
                            DbRow *row = *rowIt;
                            blazeids.push_back(row->getInt64("memberblazeid"));
                        }

                        if (!blazeids.empty())
                        {
                            UserDataResultMap userMap;
                            err = getUserDataResults(userMap, &blazeids);
                            if (err != ERR_OK)
                            {
                                ERR_LOG("[GdprComplianceSlaveImpl].pullFromAssociationlist: Error occurred executing query to userinfo [" << ErrorHelp::getErrorName(err) << "]");
                                return ERR_SYSTEM;
                            }

                            // Populate friends info
                            TableData* tableData = userData->getTableDataList().pull_back();
                            tableData->setTableDesc(listData->getName());
                            RowPtr colmnRow = tableData->getRows().pull_back();
                            colmnRow->getRowData().push_back("friend name");
                            colmnRow->getRowData().push_back("date added");
                            colmnRow->getRowData().push_back("last updated time");
                            colmnRow->getRowData().push_back("friend platform");

                            StringBuilder buf;
                            for (DbResult::const_iterator itr = result->begin(), end = result->end(); itr != end; ++itr)
                            {
                                RowPtr dataRow = tableData->getRows().pull_back();
                                DbRow *row = *itr;

                                BlazeId id = row->getInt64("memberblazeid");
                                UserDataResultMap::const_iterator friendItr = userMap.find(id);
                                if (friendItr != userMap.end())
                                {
                                    dataRow->getRowData().push_back(friendItr->second.persona.c_str());
                                    // convert timeValue to date format
                                    TimeValue dateAdded = row->getInt64("dateadded");
                                    char8_t strTime[256];
                                    dateAdded.toDateFormat(strTime, 256);
                                    dataRow->getRowData().push_back(strTime);
                                    dataRow->getRowData().push_back(row->getString("lastupdated"));
                                    dataRow->getRowData().push_back(ClientPlatformTypeToString(friendItr->second.platform));
                                }
                            }
                        }
                    }
                    else
                    {
                        ERR_LOG("[GdprComplianceSlaveImpl].pullFromAssociationlist: Error occurred executing query [" << getDbErrorString(err) << "]");
                        return ERR_SYSTEM;
                    }
                }
                else
                {
                    ERR_LOG("[GdprComplianceSlaveImpl].pullFromAssociationlist: Failed to create query");
                    return ERR_SYSTEM;
                }
            }
        }
    }
    return ERR_OK;
}
#endif

BlazeRpcError GdprComplianceSlaveImpl::runDataPullQueries(int64_t userId, const DataPullQueriesByDBIdMap& dataPullMap, UserData* userData)
{
    BlazeRpcError dbErr = ERR_OK;
    DataPullQueriesByDBIdMap::const_iterator queriesItr = dataPullMap.begin();
    for (; queriesItr != dataPullMap.end(); queriesItr++) 
    {
        DbConnPtr readConn = gDbScheduler->getLagFreeReadConnPtr(queriesItr->first);
        if (readConn != nullptr)
        {
            const eastl::vector<DataPullQuery>& entryList = queriesItr->second;
            eastl::vector<DataPullQuery>::const_iterator entryItr = entryList.begin();
            for (; entryItr != entryList.end(); entryItr++) 
            {
                QueryPtr query = DB_NEW_QUERY_PTR(readConn);
                if (query != nullptr)
                {
                    DbResultPtr dbResult;

                    // Note that the query in the data pull map is populated by this class and does not contain any characters affected by the escapeString function.
                    query->append("$s$D", entryItr->queryStr.c_str(), userId);

                    dbErr = readConn->executeQuery(query, dbResult);
                    if (dbErr != ERR_OK)
                    {
                        ERR_LOG("[GdprComplianceSlaveImpl].runDataPullQueries: Error occurred executing pull query [" << getDbErrorString(dbErr) << "] from " << entryItr->tableDesc);
                        return dbErr;
                    }

                    if (!dbResult->empty())
                    {
                        TableData* tableData = userData->getTableDataList().pull_back();
                        tableData->setTableDesc(entryItr->tableDesc.c_str());
                        RowPtr colmnRow = tableData->getRows().pull_back();
                        eastl::vector<eastl::string>::const_iterator colDesc = entryItr->columnDescList.begin();
                        for (; colDesc != entryItr->columnDescList.end(); colDesc++)
                        {
                            colmnRow->getRowData().push_back(colDesc->c_str());
                        }                        

                        StringBuilder buf;
                        for (DbResult::const_iterator itr = dbResult->begin(), end = dbResult->end(); itr != end; ++itr)
                        {
                            RowPtr dataRow = tableData->getRows().pull_back();
                            DbRow *row = *itr;

                            size_t colSize = entryItr->columnDescList.size();
                            for (size_t i = 0; i < colSize; i++)
                            {
                                dataRow->getRowData().push_back(row->getString(i));
                            }
                        }
                    }
                }
                else
                {
                    // failed to query
                    ERR_LOG("[GdprComplianceSlaveImpl].runDataPullQueries: Failed to create query for " << entryItr->queryStr.c_str() << userId);
                    dbErr = ERR_SYSTEM;
                }
            }        
        }
        else
        {
            // failed to obtain connection
            ERR_LOG("[GdprComplianceSlaveImpl].runDataPullQueries: Failed to obtain connection for DB ID " << queriesItr->first);
            dbErr = ERR_SYSTEM;
        } 
    }
    return dbErr;
}

uint64_t GdprComplianceSlaveImpl::insertGDPRRecord(int64_t accountId, const char8_t* status, const char8_t* requestType, const char8_t* requester)
{
    BlazeRpcError rc = Blaze::ERR_OK;
    // Query the DB
    DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
    if (conn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        if (query != nullptr)
        {
            // INSERT INTO gdpr_compliance_record
            query->append("INSERT INTO `gdpr_compliance_record` (`accountid`, `status`, `jobtype`, `requester`) VALUES ($D, '$s', '$s', '$s');", accountId, status, requestType, requester);
            query->append("SELECT max(ID) FROM `gdpr_compliance_record` WHERE `accountid` = $D AND `jobtype` = '$s';", accountId, requestType);

            DbResultPtrs dbResults;
            rc = conn->executeMultiQuery(query, dbResults);

            if (rc == DB_ERR_OK)
            {
                if (dbResults.size() != 2)
                {
                    ERR_LOG("[GdprComplianceSlaveImpl].insertGDPRRecord: Incorrect number of results returned from multi query");
                    return 0;
                }

                DbResultPtr dbResult = dbResults[1]; // get 2nd query result
                if (dbResult->size() != 1)
                {
                    ERR_LOG("[GdprComplianceSlaveImpl].insertGDPRRecord: Incorrect number of rows returned for result");
                    return 0;
                }

                DbRow *row = *dbResult->begin();
                uint64_t recordId = row->getUInt64(uint32_t(0)); // get auto-increment counter for our record
                return recordId;
            }
            else
            {
                ERR_LOG("[GdprComplianceSlaveImpl].insertGDPRRecord: Failed to execute query" << getDbErrorString(rc));
            }
        }
        else
        {
            // failed to query
            ERR_LOG("[GdprComplianceSlaveImpl].insertGDPRRecord: Failed to create query");
        }
    }
    else
    {
        // failed to obtain connection
        ERR_LOG("[GdprComplianceSlaveImpl].insertGDPRRecord: Failed to obtain connection");
    }

    // The starting value for auto_increment recordId is 1. Return 0 here indicates error occurred.
    return 0;
}

bool GdprComplianceSlaveImpl::isUserDeleteAlreadyInProgress(int64_t accountId)
{
    DbConnPtr readConn = gDbScheduler->getLagFreeReadConnPtr(getDbId());
    if (readConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(readConn);
        if (query != nullptr)
        {
            DbResultPtr dbResult;
            query->append("SELECT `status` FROM `gdpr_compliance_record` WHERE `accountid` = $D and `jobtype` = '$s'", accountId, GDPR_DEL_STR);
            BlazeRpcError err = readConn->executeQuery(query, dbResult);
            if (err != ERR_OK)
            {
                ERR_LOG("[GdprComplianceSlaveImpl].isUserDeleteAlreadyInProgress: Error occurred executing count query [" << getDbErrorString(err) << "]");
                return false;
            }

            if (dbResult != nullptr && !dbResult->empty())
            {
                DbResult::const_iterator rowIter = dbResult->begin();
                for (; rowIter != dbResult->end(); ++rowIter)
                {
                    const DbRow *row = *rowIter;
                    if (blaze_stricmp(row->getString("status"), GDPR_STATUS_IN_PROGRESS) == 0)
                    {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

#ifdef TARGET_gamemanager
void GdprComplianceSlaveImpl::expireGameManagerData()
{
    // use fetchComponentConfiguration RPC instead of getConfigTdf to retrieve gamemanager's config from actual instance
    // running GM to avoid ConfigDecoder errors: "Unable to get type name for generic default with map keys..."
    Blaze::FetchComponentConfigurationRequest req;
    req.getFeatures().push_back("gamemanager");
    Blaze::FetchComponentConfigurationResponse rsp;
    Blaze::RpcCallOptions opt;
    opt.routeTo.setSliverNamespace(Blaze::GameManager::GameManagerMaster::COMPONENT_ID);
    BlazeRpcError rc = gController->fetchComponentConfiguration(req, rsp, opt);
    if (rc != ERR_OK || rsp.getConfigTdfs().find("gamemanager") == rsp.getConfigTdfs().end())
    {
        ERR_LOG("[GdprComplianceSlaveImpl].expireGameManagerData: not found or error fetching gamemanager config: " << ErrorHelp::getErrorName(rc));
        return;
    }

    Blaze::GameManager::GameManagerServerConfig& gmCfg = *(Blaze::GameManager::GameManagerServerConfig *)(rsp.getConfigTdfs()["gamemanager"]->get());

    uint32_t deleteBatchSize = getConfig().getExpiredDataBatchSize();
    if (deleteBatchSize == 0)
    {
        INFO_LOG("[GdprComplianceSlaveImpl].expireGameManagerData: configured delete batch size of 0 will disable this occurrence of expiring GM data");
        return;
    }

    ClientPlatformType platform = gController->getDefaultClientPlatform();
    DbNameByPlatformTypeMap::const_iterator platItr = gmCfg.getDbNamesByPlatform().find(platform);
    if (platItr == gmCfg.getDbNamesByPlatform().end())
    {
        ERR_LOG("[GdprComplianceSlaveImpl].expireGameManagerData: failed to obtain DB config name for platform '" << ClientPlatformTypeToString(platform) << "'");
        return;
    }
    DbConnPtr dbConn = gDbScheduler->getConnPtr(gDbScheduler->getDbId(platItr->second.c_str()));
    if (dbConn == nullptr)
    {
        ERR_LOG("[GdprComplianceSlaveImpl].expireGameManagerData: failed to obtain DB connection");
        return;
    }

    QueryPtr queryBuf = DB_NEW_QUERY_PTR(dbConn);
    queryBuf->append("DELETE FROM `gm_user_connection_metrics` WHERE `conn_term_time` < $D LIMIT $u;",
        TimeValue::getTimeOfDay().getMicroSeconds() - getConfig().getDataExpiryDuration().getMicroSeconds(), deleteBatchSize);

    uint32_t total = 0;
    uint32_t affected = 0;
    do
    {
        DbResultPtr dbResult;
        rc = dbConn->executeQuery(queryBuf, dbResult);
        if (rc != ERR_OK)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].expireGameManagerData: error executing delete query: " << ErrorHelp::getErrorName(rc));
            return;
        }

        affected = dbResult->getAffectedRowCount();
        total += affected;

    } while (affected > 0);

    TRACE_LOG("[GdprComplianceSlaveImpl].expireGameManagerData: purged " << total << " expired entries in gm_user_connection_metrics");
}
#endif

bool GdprComplianceSlaveImpl::onValidateConfig(GdprComplianceConfig& config, const GdprComplianceConfig* referenceConfigPtr, ConfigureValidationErrors& validationErrors) const
{
    const DataPullDbTableList& list = config.getDataPullDbTables();
    for (DataPullDbTableList::const_iterator itr = list.begin(); itr != list.end(); itr++)
    {
        const char8_t *tableName = (*itr)->getTableName();
        if (*tableName == '\0')
        {
            StringBuilder strBuilder;
            strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): Table Name is empty in dataPullDbTables.";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
        }

        if ((*itr)->getBlazeIdColumnAlias()[0] == '\0' && (*itr)->getAccountIdColumnAlias()[0] == '\0')
        {
            StringBuilder strBuilder;
            strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): Blaze id and Account Id columns are empty for table[" << tableName << "]";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
        }
        if ((*itr)->getBlazeIdColumnAlias()[0] != '\0' && (*itr)->getAccountIdColumnAlias()[0] != '\0')
        {
            StringBuilder strBuilder;
            strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): Blaze id and Account Id columns are both set for table[" << tableName << "]";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
        }

        if ((*itr)->getColumnDescriptionByNameMap().empty())
        {
            StringBuilder strBuilder;
            strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): No columns specified for table["<< tableName << "]";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
        }
    }

    const DataEraseDbTableList& eraseList = config.getDataEraseDbTables();
    for (DataEraseDbTableList::const_iterator itr = eraseList.begin(); itr != eraseList.end(); itr++)
    {
        const char8_t *tableName = (*itr)->getTableName();
        if (*tableName == '\0')
        {
            StringBuilder strBuilder;
            strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): Table Name is empty in dataEraseDbTables.";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
        }

        if ((*itr)->getBlazeIdColumnAlias()[0] == '\0' && (*itr)->getAccountIdColumnAlias()[0] == '\0')
        {
            StringBuilder strBuilder;
            strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): Blaze id and Account Id columns are empty for erase table[" << tableName << "]";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
        }
        if ((*itr)->getBlazeIdColumnAlias()[0] != '\0' && (*itr)->getAccountIdColumnAlias()[0] != '\0')
        {
            StringBuilder strBuilder;
            strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): Blaze id and Account Id columns are both set for erase table[" << tableName << "]";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
        }

        if (!(*itr)->getDeleteRow() && 
            (*itr)->getColumnsToSetToEmptyString().empty() && (*itr)->getColumnsToSetToNULL().empty() && (*itr)->getColumnsToSetToZero().empty())
        {
            StringBuilder strBuilder;
            strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): No columns or delete row specified for erase table[" << tableName << "]";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
        }

        if ((*itr)->getDeleteRow() &&
            (!(*itr)->getColumnsToSetToEmptyString().empty() || !(*itr)->getColumnsToSetToNULL().empty() || !(*itr)->getColumnsToSetToZero().empty()))
        {
            StringBuilder strBuilder;
            strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): Columns and Delete row both specified for erase table[" << tableName << "]";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
        }

        for (const auto& curStr : (*itr)->getColumnsToSetToEmptyString())
        {
            if ((*itr)->getColumnsToSetToNULL().findAsSet(curStr) != (*itr)->getColumnsToSetToNULL().end() ||
                (*itr)->getColumnsToSetToZero().findAsSet(curStr) != (*itr)->getColumnsToSetToZero().end())
            {
                StringBuilder strBuilder;
                strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): Columns " << curStr.c_str() << " repeated for erase table[" << tableName << "]";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }
        }

        for (const auto& curStr : (*itr)->getColumnsToSetToNULL())
        {
            if ((*itr)->getColumnsToSetToZero().findAsSet(curStr) != (*itr)->getColumnsToSetToZero().end())
            {
                StringBuilder strBuilder;
                strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): Columns " << curStr.c_str() << " repeated for erase table[" << tableName << "]";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }
        }
    }

    for (auto component : getConfig().getCustomComponentsWithGDPR())
    {
        const ComponentData* compData = ComponentData::getComponentDataByName(component.c_str());
        if (compData == nullptr)
        {
            StringBuilder strBuilder;
            strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): Missing component data for [" << component << "]";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
            continue;
        }

        const CommandInfo* commandInfo = compData->getCommandInfo("getUserData");
        if (commandInfo == nullptr)
        {
            StringBuilder strBuilder;
            strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): Missing getUserData command data for [" << component << "]";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
        }
        else
        {
            if (commandInfo->requestTdfDesc == nullptr || commandInfo->requestTdfDesc->getTdfId() != CustomComponentGetDataRequest::TDF_ID)
            {
                StringBuilder strBuilder;
                strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): getUserData is not using CustomComponentGetDataRequest for [" << component << "]";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }

            if (commandInfo->responseTdfDesc == nullptr || commandInfo->responseTdfDesc->getTdfId() != CustomComponentGetDataResponse::TDF_ID)
            {
                StringBuilder strBuilder;
                strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): getUserData is not using CustomComponentGetDataRequest for [" << component << "]";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }
        }

        commandInfo = compData->getCommandInfo("deactivateUserData");
        if (commandInfo == nullptr)
        {
            StringBuilder strBuilder;
            strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): Missing deactivateUserData command data for [" << component << "]";
            validationErrors.getErrorMessages().push_back(strBuilder.get());
        }
        else
        {
            if (commandInfo->requestTdfDesc == nullptr || commandInfo->requestTdfDesc->getTdfId() != CustomComponentGetDataRequest::TDF_ID)
            {
                StringBuilder strBuilder;
                strBuilder << "[GdprComplianceSlaveImpl].onValidateConfig(): deactivateUserData is not using CustomComponentGetDataRequest for [" << component << "]";
                validationErrors.getErrorMessages().push_back(strBuilder.get());
            }
        }
    }

    return validationErrors.getErrorMessages().empty();
}

bool GdprComplianceSlaveImpl::onResolve()
{
    DataPullDbTablesByDbIdMap dataPullDbTablesByDbId;
    for (auto& itr : getConfig().getDataPullDbTables())
    {
        for (auto& nameItr : itr->getDbNamesByPlatform())
        {
            if (!gController->isPlatformUsed(nameItr.first))
            {
                INFO_LOG("[GdprComplianceSlaveImpl].onResolve(): Ignoring db '" << nameItr.second.c_str() << "' configured for table '" << itr->getTableName() << "' for platform '" << ClientPlatformTypeToString(nameItr.first) << "' (not hosted)");
                continue;
            }
            uint32_t dbId = gDbScheduler->getDbId(nameItr.second.c_str());
            if (dbId == DbScheduler::INVALID_DB_ID)
            {
                WARN_LOG("[GdprComplianceSlaveImpl].onResolve(): Failed to acquire db id for db '" << nameItr.second.c_str() << "' configured for table '" << itr->getTableName() << "' for platform '" << ClientPlatformTypeToString(nameItr.first) << "'");
                return false;
            }
            itr->copyInto(*dataPullDbTablesByDbId[dbId].pull_back());
        }
    }

    // We need to check additional DB tables existence, it is possible that other components are still populating DB while we starting up
    // Add retry logic here to avoid race condition
    uint8_t retry = 0;
    DataPullDbTablesByDbIdMap::iterator itr = dataPullDbTablesByDbId.begin();
    eastl::string failingTable = "";
    while (itr != dataPullDbTablesByDbId.end() && retry <= getConfig().getWaitAllDBTablesCreatedMaxRetry())
    {
        uint32_t dbId = itr->first;
        DataPullDbTableList& tlist = itr->second;
        if (!validateDataPullDbTables(dbId, tlist, failingTable))
            return false;

        if (tlist.empty())
        {
            ++itr;
        }
        else
        {
            Fiber::sleep(getConfig().getWaitAllDBTablesCreatedInterval().getMicroSeconds(), "GdprComplianceSlaveImpl::onResolve -- wait for Db tables populated correctly");
            retry++;
        }
    }

    bool foundAllTables = (itr == dataPullDbTablesByDbId.end());
    if (foundAllTables)
    {
        INFO_LOG("[GdprComplianceSlaveImpl].onResolve(): Successfully completed additional DB tables validation.");
    }
    else
    {
        ERR_LOG("[GdprComplianceSlaveImpl].onResolve():  DB tables validation failed (see prior WARN logs - the table is missing if no WARN logs were printed).  Missing Table '"<< failingTable <<"'.  Make sure that all of the DB tables are created and using to the correct dbNamesByPlatform entry.");
    }


    return foundAllTables;
}

bool GdprComplianceSlaveImpl::validateDataPullDbTables(uint32_t dbId, DataPullDbTableList& list, eastl::string& failingTable)
{
    DbConnPtr readConn = gDbScheduler->getLagFreeReadConnPtr(dbId);
    if (readConn == nullptr)
    {
        WARN_LOG("[GdprComplianceSlaveImpl].onResolve(): Failed to acquire db connection for db id " << dbId);
        return false;
    }

    for (DataPullDbTableList::iterator itr = list.begin(); itr != list.end() ;)
    {
        const char8_t *tableName = (*itr)->getTableName();
        failingTable = tableName;
        DbResultPtr columnResult;
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(readConn);
        queryBuf->append("SHOW COLUMNS FROM `$s`", (*itr)->getTableName());

        DbError error = readConn->executeQuery(queryBuf, columnResult);
        if (error == DB_ERR_NO_SUCH_TABLE)
        {
            INFO_LOG("[GdprComplianceSlaveImpl].onResolve(): wait for db table '" << tableName << "' to be created "<< (((*itr)->getDbNamesByPlatform().size() == 1) ? "(Unified DB)" : "(Per-Platform DB)") <<".");
            return true;
        }
        if (error != DB_ERR_OK)
        {
            WARN_LOG("[GdprComplianceSlaveImpl].onResolve(): Failed to show columns for table " << tableName << " with db error = " << getDbErrorString(error));
            return false;
        }

        typedef eastl::hash_set<const char8_t*, eastl::hash<const char8_t*>, eastl::str_equal_to<const char8_t*> > ColumnSet;
        ColumnSet columnSet;
        if (columnResult == nullptr || columnResult->empty())
        {
            WARN_LOG("[GdprComplianceSlaveImpl].onResolve(): No columns found for table " << tableName << " in DB id " << dbId);
            return false;
        }

        DbResult::const_iterator it = columnResult->begin();
        DbResult::const_iterator end = columnResult->end();
        for (; it != end; ++it)
        {
            const DbRow* row = *it;
            columnSet.insert(row->getString("Field"));
        }

        const char8_t* col = (!(*itr)->getBlazeIdColumnAliasAsTdfString().empty()) ? (*itr)->getBlazeIdColumnAlias() : (*itr)->getAccountIdColumnAlias();
        if (columnSet.find(col) == columnSet.end())
        {
            WARN_LOG("[GdprComplianceSlaveImpl].onResolve(): No blaze id column found for table " << tableName << " in DB id " << dbId);
            return false;
        }

        ColumnDescriptionByNameMap::const_iterator colItr = (*itr)->getColumnDescriptionByNameMap().begin();
        ColumnDescriptionByNameMap::const_iterator colEnd = (*itr)->getColumnDescriptionByNameMap().end();
        for (; colItr != colEnd; ++colItr)
        {
            const char8_t* column = colItr->first.c_str();
            if (columnSet.find(column) == columnSet.end())
            {
                WARN_LOG("[GdprComplianceSlaveImpl].onResolve(): DB Table " << tableName << " does not contain column " << column);
                return false;
            }
        }
        itr = list.erase(itr);
    }

    return true;
}

void GdprComplianceSlaveImpl::schedulePeriodicDeleteProcess()
{
    TRACE_LOG("[GdprComplianceSlaveImpl].schedulePeriodicDeleteProcess.");

    gSelector->scheduleFiberCall(this, &GdprComplianceSlaveImpl::doUserDelete, "GdprComplianceSlaveImpl::doUserDelete");

#ifdef TARGET_gamemanager
    gSelector->scheduleFiberCall(this, &GdprComplianceSlaveImpl::expireGameManagerData, "GdprComplianceSlaveImpl::expireGameManagerData");
#endif

    mPeriodicDeleteProcessTimerId = gSelector->scheduleTimerCall(TimeValue::getTimeOfDay() + getConfig().getDeleteProcessorPeriod(),
        this, &GdprComplianceSlaveImpl::schedulePeriodicDeleteProcess, "GdprComplianceSlaveImpl::schedulePeriodicDeleteProcess");
}

void GdprComplianceSlaveImpl::doUserDelete()
{
    BlazeRpcError err = ERR_OK;
    // fetch jobs from DB
    typedef eastl::hash_map<AccountId, uint64_t> RecordIdByAccountIdMap;
    RecordIdByAccountIdMap recordIdByAccountIdMap;
    {
        DbConnPtr readConn = gDbScheduler->getLagFreeReadConnPtr(getDbId());
        if (readConn != nullptr)
        {
            QueryPtr query = DB_NEW_QUERY_PTR(readConn);
            if (query != nullptr)
            {
                // Fetch all "in progress" delete jobs that passed the minimum processing delay time
                DbResultPtr dbResult;
                query->append("SELECT `ID`, `accountid` FROM `gdpr_compliance_record` WHERE `status` = '$s' and `jobtype` = '$s' and starttime < DATE_SUB(NOW(), INTERVAL $U SECOND);",
                    GDPR_STATUS_IN_PROGRESS, GDPR_DEL_STR, getConfig().getMinimumProcessingDelay().getSec());
                err = readConn->executeQuery(query, dbResult);
                if (err != ERR_OK)
                {
                    ERR_LOG("[GdprComplianceSlaveImpl].doUserDelete: Error occurred executing query [" << getDbErrorString(err) << "]");
                }
                else if (!dbResult->empty())
                {
                    StringBuilder buf;
                    for (DbResult::const_iterator itr = dbResult->begin(), end = dbResult->end(); itr != end; ++itr)
                    {
                        DbRow *row = *itr;
                        recordIdByAccountIdMap.insert(eastl::make_pair(row->getUInt64("accountid"), row->getUInt64("ID")));
                    }
                }
            }
        }
        else
        {
            err = ERR_DB_SYSTEM;
        }
    }

    if (err != ERR_OK)
    {
        return; // DB error, exit
    }

    // user offline
    typedef eastl::hash_map<AccountId, UserDataResultMap> UserDataResultsByBlazeIdByAccountId;
    UserDataResultsByBlazeIdByAccountId userDeleteMap;
    for (RecordIdByAccountIdMap::const_iterator it = recordIdByAccountIdMap.begin(); it != recordIdByAccountIdMap.end(); it++)
    {
        // Only delete data for offline users with deleted Nucleus accounts
        AccountId accountId = it->first;
        if (!checkIfNucleusDeleteUser(accountId))
            continue;

        UserDataResultsByBlazeIdByAccountId::insert_return_type inserted = userDeleteMap.insert(accountId);
        UserDataResultMap& userMap = inserted.first->second;
        err = getUserDataResults(userMap, nullptr, accountId);
        if (err == Blaze::ERR_OK)
        {
            for (UserDataResultMap::iterator itr = userMap.begin(); itr != userMap.end() ; )
            {
                if (gUserSessionManager->isUserOnline(itr->first))
                {
                    // If ANY persona is currently online, then we can't delete the account information yet.
                    userDeleteMap.erase(accountId);
                    break;
                }
                else
                {
                    ++itr;
                }
            }
        }
        else 
        {
            ERR_LOG("[GdprComplianceSlaveImpl].doUserDelete: Failed to lookup user info for account id " << it->first);
        }
    }
   
    // for each player, delete external association lists, deactivate userinfo
    for (UserDataResultsByBlazeIdByAccountId::const_iterator delItr = userDeleteMap.begin(); delItr != userDeleteMap.end(); delItr++)
    {
        // Clear error for each loop:
        err = ERR_OK;
        AccountId accountId = delItr->first;

        // config tables
        const DataEraseDbTableList& list = getConfig().getDataEraseDbTables();
        for (const auto& eraseIter : list)
        {
            if (err != ERR_OK)
                break;

            // Build delete requests for each blaze id:
            for (auto& blazeIdIter : delItr->second)
            {
                if (eraseIter->getBlazeIdColumnAliasAsTdfString().empty())
                    continue;

                if (err != ERR_OK)
                    break;

                BlazeId blazeId = blazeIdIter.first;
                ClientPlatformType tempPlatform = blazeIdIter.second.platform;

                // For shared DB systems, try to use the default client platform:
                if (eraseIter->getDbNamesByPlatform().size() == 1 && eraseIter->getDbNamesByPlatform().begin()->first == gController->getDefaultClientPlatform())
                    tempPlatform = gController->getDefaultClientPlatform();

                DbNameByPlatformTypeMap::const_iterator nameItr = eraseIter->getDbNamesByPlatform().find(tempPlatform);
                if (nameItr == eraseIter->getDbNamesByPlatform().end())
                {
                    INFO_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: Skipping data pull from table '" << eraseIter->getTableName() << "' for blazeId " << blazeId << " - no db configured for platform '" << ClientPlatformTypeToString(tempPlatform) << "'");
                    continue;
                }

                uint32_t dbId = gDbScheduler->getDbId(nameItr->second.c_str());

                DbConnPtr personaDeleteConn = gDbScheduler->getConnPtr(dbId);
                if (personaDeleteConn != nullptr)
                {
                    QueryPtr queryBuf = DB_NEW_QUERY_PTR(personaDeleteConn);

                    queryBuf->append((eraseIter->getDeleteRow()) ? "DELETE FROM " : "UPDATE ");
                    queryBuf->append("`$s` ", eraseIter->getTableName());

                    if (!eraseIter->getDeleteRow())
                    {
                        queryBuf->append("SET ");

                        for (auto& nullColumnIter : eraseIter->getColumnsToSetToNULL())
                            queryBuf->append("`$s`=NULL, ", nullColumnIter.c_str());
                        for (auto& zeroColumnIter : eraseIter->getColumnsToSetToZero())
                            queryBuf->append("`$s`=0, ", zeroColumnIter.c_str());
                        for (auto& emptyColumnIter : eraseIter->getColumnsToSetToEmptyString())
                            queryBuf->append("`$s`='', ", emptyColumnIter.c_str());

                        queryBuf->trim(2);
                        queryBuf->append(" ");
                    }
                    queryBuf->append("WHERE `$s`=$D", eraseIter->getBlazeIdColumnAlias(), blazeId);
                    err = personaDeleteConn->executeQuery(queryBuf);

                    if (err != ERR_OK)
                    {
                        ERR_LOG("[GdprComplianceSlaveImpl].doUserDelete: Error occurred when updating GDPR record with request '"<< queryBuf->c_str() << "' Error[" << getDbErrorString(err) << "]");
                    }
                }
            }
        
            // Attempt Account Id based removal:
            if (eraseIter->getAccountIdColumnAliasAsTdfString().empty())
                continue;

            if (err != ERR_OK)
                break;

            ClientPlatformType tempPlatform = common;

            // For shared DB systems, try to use the default client platform:
            if (eraseIter->getDbNamesByPlatform().size() == 1 && eraseIter->getDbNamesByPlatform().begin()->first == gController->getDefaultClientPlatform())
                tempPlatform = gController->getDefaultClientPlatform();

            DbNameByPlatformTypeMap::const_iterator nameItr = eraseIter->getDbNamesByPlatform().find(tempPlatform);
            if (nameItr == eraseIter->getDbNamesByPlatform().end())
            {
                INFO_LOG("[GdprComplianceSlaveImpl].pullUserDataFromDBs: Skipping data pull from table '" << eraseIter->getTableName() << "' for accountId " << accountId << " - no db configured for platform '" << ClientPlatformTypeToString(tempPlatform) << "'");
                continue;
            }

            uint32_t dbId = gDbScheduler->getDbId(nameItr->second.c_str());

            DbConnPtr accountDeleteConn = gDbScheduler->getConnPtr(dbId);
            if (accountDeleteConn != nullptr)
            {
                QueryPtr queryBuf = DB_NEW_QUERY_PTR(accountDeleteConn);

                queryBuf->append((eraseIter->getDeleteRow()) ? "DELETE FROM " : "UPDATE ");
                queryBuf->append("`$s` ", eraseIter->getTableName());

                if (!eraseIter->getDeleteRow())
                {
                    queryBuf->append("SET ");

                    for (auto& nullColumnIter : eraseIter->getColumnsToSetToNULL())
                        queryBuf->append("`$s`=NULL, ", nullColumnIter.c_str());
                    for (auto& zeroColumnIter : eraseIter->getColumnsToSetToZero())
                        queryBuf->append("`$s`=0, ", zeroColumnIter.c_str());
                    for (auto& emptyColumnIter : eraseIter->getColumnsToSetToEmptyString())
                        queryBuf->append("`$s`='', ", emptyColumnIter.c_str());

                    queryBuf->trim(2);
                    queryBuf->append(" ");
                }
                queryBuf->append("WHERE `$s`=$D", eraseIter->getAccountIdColumnAlias(), accountId);
                err = accountDeleteConn->executeQuery(queryBuf);

                if (err != ERR_OK)
                {
                    ERR_LOG("[GdprComplianceSlaveImpl].doUserDelete: Error occurred when updating GDPR record with request '" << queryBuf->c_str() << "' Error[" << getDbErrorString(err) << "]");
                }
            }
        }

        // Attempt to delete with RPC:
        if (err == ERR_OK)
        {
            err = customComponentDeactivateUserData(delItr->second, accountId);
        }

        if (err == ERR_OK)
        {
        err = deactivateAccountInfoTable(delItr->first);
        }

        if (err == ERR_OK)
        {
            for (UserDataResultMap::const_iterator userItr = delItr->second.begin(); userItr != delItr->second.end(); ++userItr)
            {
                BlazeRpcError deleteByBlazeIdErr = deactivateUserInfoTable(userItr->first, userItr->second.platform);
#ifdef TARGET_associationlists
                if (deleteByBlazeIdErr == ERR_OK)
                    deleteByBlazeIdErr = deleteUserAssociationList(userItr->first, userItr->second.platform);
#endif
                if (err == ERR_OK)
                    err = deleteByBlazeIdErr;
            }
        }

        RecordIdByAccountIdMap::const_iterator recordItr = recordIdByAccountIdMap.find(delItr->first);
        BlazeRpcError updateErr = updateGDPRRequestStatus(recordItr->second, err);
        if (updateErr != ERR_OK)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].doUserDelete: Error occurred when updating GDPR record [" << getDbErrorString(updateErr) << "]");
        }
    }    
}

BlazeRpcError GdprComplianceSlaveImpl::updateGDPRRequestStatus(uint64_t recordId, BlazeRpcError err)
{
    DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
        queryBuf->append("UPDATE `gdpr_compliance_record` set `status` = '$s', `msg` = '$s', `lastupdated` = '$D' WHERE `ID` = $D", (err == ERR_OK) ? GDPR_STATUS_COMPLETED : GDPR_STATUS_FAILED,
            ErrorHelp::getErrorName(err), TimeValue::getTimeOfDay().getMicroSeconds(), recordId);
        return conn->executeQuery(queryBuf);
    }
    else
    {
        return ERR_SYSTEM;
    }
}

#ifdef TARGET_associationlists
BlazeRpcError GdprComplianceSlaveImpl::deleteUserAssociationList(BlazeId id, ClientPlatformType platform)
{
    BlazeRpcError err = ERR_OK;
    Component* component = gController->getComponent(Blaze::Association::AssociationListsSlave::COMPONENT_ID, false);
    if (component == nullptr)
    {
        INFO_LOG("[GdprComplianceSlaveImpl].deleteUserAssociationList: No Association component found.");
    }
    else
    {
        // generate Association list Queries from config    
        Blaze::Association::AssociationlistsConfig associationListCfg;
        BlazeRpcError rc = gController->getConfigTdf("associationlists", false, associationListCfg);
        if (rc != ERR_OK)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].deleteUserAssociationList: Failure fetching config for associationlists, error=" << ErrorHelp::getErrorName(rc));
            return rc;
        }

        // AssocLists only use a single platform, the current one:
        if (associationListCfg.getDbNamesByPlatform().find(gController->getDefaultClientPlatform()) == associationListCfg.getDbNamesByPlatform().end())
        {
            ERR_LOG("[GdprComplianceSlaveImpl].deleteUserAssociationList: No AssocList DB config found for platform " << ClientPlatformTypeToString(gController->getDefaultClientPlatform()) << ".");
            return ERR_SYSTEM;
        }

        uint32_t dbId = gDbScheduler->getDbId(associationListCfg.getDbNamesByPlatform().find(gController->getDefaultClientPlatform())->second.c_str());

        eastl::list<eastl::string> tableNames;
        const Blaze::Association::ListDataVector& assocLists = associationListCfg.getLists();
        const char8_t* tableSuffix = "shared";
        if (!gController->isSharedCluster())
            tableSuffix = gController->usesExternalString(gController->getDefaultClientPlatform()) ? "string" : "id";
        for (Blaze::Association::ListDataVector::const_iterator it = assocLists.begin(); it != assocLists.end(); ++it)
        {
            const Blaze::Association::ListData* listData = *it;
            // mutual or paired lists does not have external id tables
            if (listData->getName()[0] != '\0' && !listData->getMutualAction() && listData->getPairId() == 0)
            {
                char8_t valueBuf[128];
                blaze_snzprintf(valueBuf, sizeof(valueBuf), "user_association_list_%s_ext%s", listData->getName(), tableSuffix);
                tableNames.push_back(valueBuf);
            }
        }

        if (!tableNames.empty())
        {
            DbConnPtr conn = gDbScheduler->getConnPtr(dbId);
            if (conn != nullptr)
            {
                for (eastl::list<eastl::string>::const_iterator itr = tableNames.begin(); itr != tableNames.end(); itr++)
                {
                    QueryPtr query = DB_NEW_QUERY_PTR(conn);
                    if (query != nullptr)
                    {
                        const char8_t *tableName = itr->c_str();
                        query->append("DELETE FROM `$s` where `blazeid` = $D;", tableName, id);
                        err = conn->executeQuery(query);
                        if (err != ERR_OK)
                        {
                            ERR_LOG("[GdprComplianceSlaveImpl].deleteUserAssociationList: Error occurred executing count query [" << getDbErrorString(err) << "]");
                            return ERR_SYSTEM;
                        }
                    }
                    else
                    {
                        ERR_LOG("[GdprComplianceSlaveImpl].deleteUserAssociationList: Failed to create query");
                        return ERR_SYSTEM;
                    }
                }
            }
            else
            {
                ERR_LOG("[GdprComplianceSlaveImpl].deleteUserAssociationList: Failed to get DB connection");
                return ERR_DB_SYSTEM;
            }
        }        
    }

    return err;
}
#endif

BlazeRpcError GdprComplianceSlaveImpl::deactivateUserInfoTable(BlazeId id, ClientPlatformType platform)
{
    BlazeRpcError err = ERR_OK;
    DbConnPtr conn = gDbScheduler->getConnPtr(gUserSessionManager->getDbId(platform));
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
        queryBuf->append("UPDATE `userinfo` set `persona`=NULL, `canonicalpersona`=NULL, `externalid`=NULL, `externalstring`=NULL, `status`=0 where `blazeid`=$D", id);
        err = conn->executeQuery(queryBuf);

        if (err == ERR_OK)
        {
            INFO_LOG("[GdprComplianceSlaveImpl].deactivateUserInfoTable: Player (" << id << ") has been deactivated.");
        }
        else
        {
            ERR_LOG("[GdprComplianceSlaveImpl].deactivateUserInfoTable: Error " << ErrorHelp::getErrorName(err) << " deactivating userinfo for user (" << id << "), platform '" << ClientPlatformTypeToString(platform) << "'");
        }
    }
    else
    {
        ERR_LOG("[GdprComplianceSlaveImpl].deactivateUserInfoTable: Failed to get database connection for deactivating user (" << id << "), platform '" << ClientPlatformTypeToString(platform) << "'");
        err = ERR_SYSTEM;
    }

    // If we're deactivating the userinfo table, we must have successfully deactivated the accountinfo table already.
    // So even if the userinfo deactivation failed, we need to notify all usersession slaves to clear the userinfo cache and indices.
    UserInfoUpdated userInfoUpdated;
    userInfoUpdated.setBlazeId(id);
    gUserSessionManager->sendNotifyUserInfoUpdatedToAllSlaves(&userInfoUpdated);

    return err;
}

BlazeRpcError GdprComplianceSlaveImpl::deactivateAccountInfoTable(AccountId id)
{
    BlazeRpcError err = ERR_OK;
    DbConnPtr conn = gDbScheduler->getConnPtr(gUserSessionManager->getDbId());
    if (conn != nullptr)
    {
        QueryPtr queryBuf = DB_NEW_QUERY_PTR(conn);
        queryBuf->append("UPDATE `accountinfo` set `originpersonaname`=NULL, `canonicalpersona`=NULL, `primaryexternalidxbl`=NULL, `primaryexternalidpsn`=NULL, `primaryexternalidswitch`=NULL, `primaryexternalidsteam`=NULL, `primaryexternalidstadia`=NULL where `accountid`=$U", id);
        err = conn->executeQuery(queryBuf);

        if (err == ERR_OK)
        {
            INFO_LOG("[GdprComplianceSlaveImpl].deactivateAccountInfoTable: Account (" << id << ") has been deactivated.");
        }
        else
        {
            ERR_LOG("[GdprComplianceSlaveImpl].deactivateAccountInfoTable: Error " << ErrorHelp::getErrorName(err) << " deactivating account (" << id << ")");
        }
    }
    else
    {
        ERR_LOG("[GdprComplianceSlaveImpl].deactivateUserInfoTable: Failed to get database connection for deactivating account (" << id << ")");
        err = ERR_SYSTEM;
    }

    return err;
}

bool GdprComplianceSlaveImpl::checkIfNucleusDeleteUser(AccountId id)
{
    //Since we're a trusted server connection, we set the super user privilege to fetch auth token with all default scopes
    UserSession::SuperUserPermissionAutoPtr ptr(true);

    OAuth::AccessTokenUtil tokenUtil;
    BlazeRpcError tokErr = tokenUtil.retrieveServerAccessToken(OAuth::TOKEN_TYPE_BEARER);
    if (tokErr != ERR_OK)
    {
        WARN_LOG("[GdprComplianceSlaveImpl].getAccountFromNucleus: failed to retrieve server access token with error: " << ErrorHelp::getErrorName(tokErr));
        return false;
    }

    NucleusIdentity::GetPidRequest req;
    NucleusIdentity::GetAccountResponse resp;
    NucleusIdentity::IdentityError error;
    req.setPid(id);
    req.getAuthCredentials().setAccessToken(tokenUtil.getAccessToken());
    req.getAuthCredentials().setClientId(tokenUtil.getClientId());

    NucleusIdentity::NucleusIdentitySlave* ident = (NucleusIdentity::NucleusIdentitySlave*)(gOutboundHttpService->getService(NucleusIdentity::NucleusIdentitySlave::COMPONENT_INFO.name));

    BlazeRpcError rc = ident->getPid(req, resp, error);
    if (rc != ERR_OK)
    {
        if (blaze_stricmp(error.getError().getCode(), "NO_SUCH_PID") == 0)
        {
            INFO_LOG("[GdprComplianceSlaveImpl].checkIfNucleusDeleteUser: Player with accountId(" << id << ") does not exist in Nucleus.");
            return true;
        }
    }
    return false;
}

BlazeRpcError GdprComplianceSlaveImpl::customComponentGetUserData(const GdprComplianceSlaveImpl::UserDataResultMap &userMap, AccountId accountId, GetUserDataResponse &response)
{
    for (auto component : getConfig().getCustomComponentsWithGDPR())
    {
        const ComponentData* compData = ComponentData::getComponentDataByName(component.c_str());
        if (compData == nullptr)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].processGetUserData(): Unable to find component (" << component << ") for GDPR getData call.");
            return ERR_SYSTEM;
        }

        const CommandInfo* commandInfo = compData->getCommandInfo("getUserData");
        if (commandInfo == nullptr)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].processGetUserData(): Unable to find command getUserData for component (" << component << ") for GDPR getData call.");
            return ERR_SYSTEM;
        }

        Component* componentProxy = gController->getComponent(compData->id, false, false);
        if (componentProxy == nullptr)
        {
            // Check if it is a proxy http component
            componentProxy = (Component*)Blaze::gOutboundHttpService->getService(compData->name);
            if (componentProxy == nullptr)
            {
                ERR_LOG("[GdprComplianceSlaveImpl].processGetUserData(): Component (" << component << ") not found.");
                return ERR_SYSTEM;
            }
        }

        CustomComponentGetDataRequest customRequest;
        CustomComponentGetDataResponse customResponse;
        EA::TDF::TdfPtr errResponse = commandInfo->createErrorResponse("GetUserData Err Response");  // This shouldn't have anything but is included here for robustness.

        // Build the request: 
        customRequest.setAccountId(accountId);
        for (auto& itr : userMap)
        {
            customRequest.getPlatformMap()[itr.first] = itr.second.platform;
        }

        // Call the RPC:
        RpcCallOptions callOpts;
        callOpts.timeoutOverride = getConfig().getCustomComponentWaitTime();
        BlazeRpcError customErr = componentProxy->sendRequest(*commandInfo, &customRequest, &customResponse, errResponse, callOpts);
        if (customErr != ERR_OK)
        {
            ERR_LOG("[ExternalDataManager].makeRpcCall(): Failed to sendRequest (" << component << ") : getUserData.  Error: " << customErr);
            return customErr;
        }

        // Merge the data in, so that the TitleData is updated:  (This code assumes that the UserDataList already has entries for each platform)
        // For each platform in the final response...
        for (auto& userDataListIter : response.getUserDataList())
        {
            // check if there's a custom data table for that platform...
            auto customRespIter = customResponse.getTablesByPlatform().find(userDataListIter->getPlatform());
            if (customRespIter != customResponse.getTablesByPlatform().end())
            {
                // if so, for each custom data table in the response:
                for (auto& customDataTable : *(customRespIter->second))
                {
                    // add a new entry into the final response...
                    auto tableDataIter = userDataListIter->getTableDataList().pull_back();

                    // and copy over the custom data tables:
                    customDataTable->copyInto(*tableDataIter);
                }
            }
        }
    }

    return ERR_OK;
}
BlazeRpcError GdprComplianceSlaveImpl::customComponentDeactivateUserData(const UserDataResultMap &userMap, AccountId accountId)
{
    for (auto component : getConfig().getCustomComponentsWithGDPR())
    {
        const ComponentData* compData = ComponentData::getComponentDataByName(component.c_str());
        if (compData == nullptr)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].processGetUserData(): Unable to find component (" << component << ") for GDPR getData call.");
            return ERR_SYSTEM;
        }

        const CommandInfo* commandInfo = compData->getCommandInfo("deactivateUserData");
        if (commandInfo == nullptr)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].processGetUserData(): Unable to find command getUserData for component (" << component << ") for GDPR getData call.");
            return ERR_SYSTEM;
        }

        Component* componentProxy = gController->getComponent(compData->id, false, false);
        if (componentProxy == nullptr)
        {
            // Check if it is a proxy http component
            componentProxy = (Component*)Blaze::gOutboundHttpService->getService(compData->name);
            if (componentProxy == nullptr)
            {
                ERR_LOG("[GdprComplianceSlaveImpl].processGetUserData(): Component (" << component << ") not found.");
                return ERR_SYSTEM;
            }
        }

        CustomComponentGetDataRequest customRequest;
        EA::TDF::TdfPtr customResponse = commandInfo->createResponse("GetUserData Response");   // This shouldn't have anything but is included here for robustness.
        EA::TDF::TdfPtr errResponse = commandInfo->createErrorResponse("GetUserData Err Response");  // This shouldn't have anything but is included here for robustness.

        // Build the request: 
        customRequest.setAccountId(accountId);
        for (auto& itr : userMap)
        {
            customRequest.getPlatformMap()[itr.first] = itr.second.platform;
        }

        // Call the RPC:
        RpcCallOptions callOpts;
        callOpts.timeoutOverride = getConfig().getCustomComponentWaitTime();
        BlazeRpcError customErr = componentProxy->sendRequest(*commandInfo, &customRequest, customResponse, errResponse, callOpts);
        if (customErr != ERR_OK)
        {
            ERR_LOG("[ExternalDataManager].makeRpcCall(): Failed to sendRequest (" << component << ") : getUserData.  Error: " << customErr);
            return customErr;
        }
    }

    return ERR_OK;
}

CheckRecordProgressError::Error GdprComplianceSlaveImpl::processCheckRecordProgress(const CheckRecordProgressRequest &request, CheckRecordProgressResponse &response, const Message* message)
{
    TRACE_LOG("[CheckRecordProgressCommand].execute() - " << request);

    // validate request sessionKey's session id
    UserSessionId userSessionId = gUserSessionManager->getUserSessionIdFromSessionKey(request.getSessionKey());
    if (userSessionId == INVALID_USER_SESSION_ID)
    {
        return CheckRecordProgressError::commandErrorFromBlazeError(GDPRCOMPLIANCE_ERR_AUTHENTICATION_REQUIRED);
    }

    if (!gUserSessionManager->isSessionAuthorized(userSessionId, Blaze::Authorization::PERMISSION_DELETE_USER_DATA)
        || !gUserSessionManager->isSessionAuthorized(userSessionId, Blaze::Authorization::PERMISSION_GET_USER_DATA))
        return CheckRecordProgressError::commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);

    // If request specify account id, return record status
    BlazeRpcError err = ERR_OK;
    DbConnPtr readConn = gDbScheduler->getLagFreeReadConnPtr(getDbId());
    if (readConn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(readConn);
        if (query != nullptr)
        {
            DbResultPtr dbResult;
            query->append("SELECT `requester`, `status`, `starttime`, `jobtype`, `msg` FROM `gdpr_compliance_record` WHERE `accountid` = $D", request.getAccountId());
            err = readConn->executeQuery(query, dbResult);
            if (err != ERR_OK)
            {
                ERR_LOG("[GdprComplianceSlaveImpl].processCheckRecordProgress: Error occurred executing check record progress query [" << getDbErrorString(err) << "]");
                return CheckRecordProgressError::commandErrorFromBlazeError(err);
            }

            if (dbResult != nullptr && !dbResult->empty())
            {
                DbResult::const_iterator rowIter = dbResult->begin();
                for (; rowIter != dbResult->end(); ++rowIter)
                {
                    const DbRow *row = *rowIter;
                    StringBuilder msg;
                    ProgressStatus* status = response.getProgressStatusList().pull_back();
                    status->setRequesterInfo(row->getString("requester"));
                    status->setRequestCreatedTime(row->getString("startTime"));
                    status->setJobStatus(row->getString("status"));
                    status->setJobType(row->getString("jobtype"));
                    status->setMessage(row->getString("msg"));
                }
            }
        }
    }

    return CheckRecordProgressError::commandErrorFromBlazeError(err);
}

DeactivateUserDataError::Error GdprComplianceSlaveImpl::processDeactivateUserData(const DeactivateUserDataRequest &request, DeactivateUserDataResponse &response, const Message* message)
{
    TRACE_LOG("[DeactivateUserDataCommand].execute() - " << request);

    // validate request sessionKey's session id
    UserSessionId userSessionId = gUserSessionManager->getUserSessionIdFromSessionKey(request.getSessionKey());
    if (userSessionId == INVALID_USER_SESSION_ID)
    {
        return DeactivateUserDataError::commandErrorFromBlazeError(GDPRCOMPLIANCE_ERR_AUTHENTICATION_REQUIRED);
    }

    if (!gUserSessionManager->isSessionAuthorized(userSessionId, Blaze::Authorization::PERMISSION_DELETE_USER_DATA))
        return DeactivateUserDataError::commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);

    // Handle duplicate request by checking if user is already deactivated
    UserDataResultMap userMap;
    BlazeRpcError err = getUserDataResults(userMap, nullptr, request.getAccountId());
    if (err == GDPRCOMPLIANCE_ERR_REQUESTED_USER_NOT_FOUND || (err == ERR_OK && userMap.empty()))
    {
        response.setMessage("Player does not exist.");
        return DeactivateUserDataError::ERR_OK;
    }

    // insert new record if no in progress delete job for this player
    if (isUserDeleteAlreadyInProgress(request.getAccountId()))
    {
        response.setMessage("Player deactivate request is already in progress.");
        return DeactivateUserDataError::ERR_OK;
    }
    
    uint64_t recordId = insertGDPRRecord(request.getAccountId(), GDPR_STATUS_IN_PROGRESS, GDPR_DEL_STR, request.getRequesterInfo());
    if (recordId > 0)
    {
        response.setMessage("Player deactivate request is scheduled.");
    }
    else
    {
        response.setMessage("Error occur during scheduling player deactivate request.");
        return DeactivateUserDataError::commandErrorFromBlazeError(ERR_SYSTEM);
    }
    return DeactivateUserDataError::ERR_OK;
}

GetUserDataError::Error GdprComplianceSlaveImpl::processGetUserData(const GetUserDataRequest &request, GetUserDataResponse &response, const Message* message)
{
    TRACE_LOG("[GetUserDataCommand].execute() - " << request);

    // validate request sessionKey's session id
    UserSessionId userSessionId = gUserSessionManager->getUserSessionIdFromSessionKey(request.getSessionKey());
    if (userSessionId == INVALID_USER_SESSION_ID)
    {
        return GetUserDataError::commandErrorFromBlazeError(GDPRCOMPLIANCE_ERR_AUTHENTICATION_REQUIRED);
    }

    // check permission
    if (!gUserSessionManager->isSessionAuthorized(userSessionId, Blaze::Authorization::PERMISSION_GET_USER_DATA))
        return GetUserDataError::commandErrorFromBlazeError(ERR_AUTHORIZATION_REQUIRED);

    uint64_t recordId = insertGDPRRecord(request.getAccountId(), GDPR_STATUS_IN_PROGRESS, GDPR_PULL_STR, request.getRequesterInfo());
    if (recordId == 0)
    {
        return GetUserDataError::commandErrorFromBlazeError(ERR_SYSTEM);
    }

    UserDataResultMap userMap;
    BlazeRpcError err = getUserDataResults(userMap, nullptr, request.getAccountId());
    if (!userMap.empty())
    {
        for (UserDataResultMap::const_iterator itr = userMap.begin(); itr != userMap.end(); itr++)
        {
            BlazeId id = itr->first;
            UserData* userdata = response.getUserDataList().pull_back();
            userdata->setPersona(itr->second.persona.c_str());
            userdata->setPlatform(itr->second.platform);
            BlazeRpcError pullErr = pullUserDataFromDBsByBlazeId(id, userdata);

            if (pullErr != ERR_OK)
            {
                ERR_LOG("[GetUserDataCommand].execute() pull User(" << id << ") data failed with error '"
                    << ErrorHelp::getErrorName(pullErr) << "'.");
                if (err == ERR_OK)
                    err = pullErr;
                break;
            }
        }

        // Only do account lookup when no errors are occurring:
        if (err == ERR_OK)
        {
            UserData* userdata = response.getUserDataList().pull_back();
            userdata->setPersona("(Account)");
            userdata->setPlatform(common);
            BlazeRpcError pullErr = pullUserDataFromDBsByAccountId(request.getAccountId(), userdata);

            if (pullErr != ERR_OK)
            {
                ERR_LOG("[GetUserDataCommand].execute() pull User(" << request.getAccountId() << ") data by account id failed with error '"
                    << ErrorHelp::getErrorName(pullErr) << "'.");
                if (err == ERR_OK)
                    err = pullErr;
            }
        }
    
        // Get data from the custom components: (Direct RPC call)
        if (err == ERR_OK)
        {
            err = customComponentGetUserData(userMap, request.getAccountId(), response);
        }

        // Don't both returning the Account entry if there's nothing in it:
        for (auto listEntry = response.getUserDataList().begin(); listEntry != response.getUserDataList().end(); ++listEntry)
        {
            if ((*listEntry)->getPlatform() == common && (*listEntry)->getTableDataList().empty())
            {
                response.getUserDataList().erase(listEntry);
                break;
            }
        }
    }
    else if (err == ERR_OK)
    {
        err = GDPRCOMPLIANCE_ERR_REQUESTED_USER_NOT_FOUND;
    }

    // Update Gdpr record status in DB
    BlazeRpcError updateRecordErr = updateGDPRRequestStatus(recordId, err);
    if (updateRecordErr != ERR_OK)
    {
        ERR_LOG("[GdprComplianceSlaveImpl].processGetUserData: Error occurred when updating GDPR record [" << getDbErrorString(updateRecordErr) << "]");
        if (err == ERR_OK)
        {
            err = updateRecordErr;
        }
    }

    // return the first error we seen
    return GetUserDataError::commandErrorFromBlazeError(err);
}

BlazeRpcError GdprComplianceSlaveImpl::getUserDataResults(UserDataResultMap& resultMap, const BlazeIdVector* blazeIds, AccountId accountId /*= INVALID_ACCOUNT_ID*/)
{
    bool searchByBlazeIds = (blazeIds != nullptr && !blazeIds->empty());
    if (!searchByBlazeIds && accountId == INVALID_ACCOUNT_ID)
    {
        ERR_LOG("[GdprComplianceSlaveImpl].getUserDataResults: Failed to look up userinfo: no valid blazeids or accountid provided.");
        return GDPRCOMPLIANCE_ERR_REQUESTED_USER_NOT_FOUND;
    }

    // Build the query search string. Avoid these escaped characters: \x00 (ASCII 0, NUL), \n, \r, \, ', ", and \x1a (Control-Z)
    eastl::string searchStr;
    if (searchByBlazeIds)
    {
        BlazeIdVector::const_iterator itr = blazeIds->begin();
        searchStr.append_sprintf("`blazeid` IN (%" PRIi64, *itr);
        ++itr;
        for ( ; itr != blazeIds->end(); ++itr)
            searchStr.append_sprintf(",%" PRIi64, *itr);
        searchStr.append_sprintf(")");
    }
    else
    {
        searchStr.append_sprintf("`accountid` = %" PRIu64, accountId);
    }

    BlazeRpcError err = ERR_OK;
    for (ClientPlatformSet::const_iterator platItr = gController->getHostedPlatforms().begin(); platItr != gController->getHostedPlatforms().end(); ++platItr)
    {
        DbConnPtr readConn = gDbScheduler->getReadConnPtr(gUserSessionManager->getDbId(*platItr));
        if (readConn == nullptr)
        {
            ERR_LOG("[GdprComplianceSlaveImpl].getUserDataResults: Failed to obtain read-only connection to userinfo db for platform '" << ClientPlatformTypeToString(*platItr) << "'");
            err = ERR_SYSTEM;
            continue;
        }

        QueryPtr queryBuf = DB_NEW_QUERY_PTR(readConn);

        // Note that searchStr is populated above and does not contain any characters affected by the escapeString function.
        queryBuf->append("SELECT `blazeid`, `persona` FROM `userinfo` where `status` = 1 and $s;", searchStr.c_str());

        DbResultPtr result;
        BlazeRpcError queryErr = readConn->executeQuery(queryBuf, result);
        if (queryErr != ERR_OK)
        {
            ERR_LOG("[GetUserDataCommand].execute(): Failed to look up userinfo for " << searchStr.c_str() << " for platform '" << ClientPlatformTypeToString(*platItr) <<
                "'. Error: " << queryErr << " '" << ErrorHelp::getErrorName(queryErr) << "'.");
            if (err == ERR_OK)
                err = queryErr;
            continue;
        }

        for (DbResult::const_iterator it = result->begin(), end = result->end(); it != end; ++it)
        {
            UserDataResult& userResult = resultMap[(*it)->getInt64("blazeid")];
            userResult.persona = (*it)->getString("persona");
            userResult.platform = *platItr;
        }

        if (searchByBlazeIds && resultMap.size() == blazeIds->size())
            return ERR_OK;
    }
    return err;
}


BlazeRpcError GdprComplianceSlaveImpl::createGDPRTable()
{
    BlazeRpcError result = ERR_OK;
    DbConnPtr conn = gDbScheduler->getConnPtr(getDbId());
    if (conn != nullptr)
    {
        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        if (query != nullptr)
        {
            query->append(
                "CREATE TABLE IF NOT EXISTS `gdpr_compliance_record` ("
                "`ID` int NOT NULL AUTO_INCREMENT,"
                "`accountid` bigint(20) NOT NULL,"
                "`status` varchar(32) NOT NULL,"
                "`jobtype` varchar(32) NOT NULL,"
                "`requester` varchar(256) NOT NULL,"
                "`starttime` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,"
                "`lastupdated` BIGINT(20) unsigned NOT NULL DEFAULT '0',"
                "`msg` varchar(256) DEFAULT NULL,"
                "PRIMARY KEY(`ID`),"
                "KEY `startTime_idx` (`starttime`),"
                "KEY `accountid_idx` (`accountid`) USING BTREE,"
                "KEY `status_idx` (`status`) USING BTREE,"
                "KEY `jobtype_idx` (`jobtype`) USING BTREE"
                ") ENGINE = InnoDB DEFAULT CHARSET = utf8;"
            );
            result = conn->executeQuery(query);
            if (result != ERR_OK)
            {
                ERR_LOG("[GdprComplianceSlaveImpl].createGDPRTable: Error occurred executing query [" << getDbErrorString(result) << "]");
                result = ERR_SYSTEM;
            }
        }
        else
        {
            ERR_LOG("[GdprComplianceSlaveImpl].createGDPRTable: Failed to create new query");
            result = ERR_SYSTEM;
        }
    }
    else
    {
        ERR_LOG("[GdprComplianceSlaveImpl].createGDPRTable: Failed to obtain connection");
        result = ERR_SYSTEM;
    }
    return result;
}

} // GdprCompliance
} // Blaze
