/*************************************************************************************************/
/*!
    \file   dbconnpool.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DbScheduler

    This class is responsible for scheduling Database worker threads.  A command can call into the 
    DBScheduler by calling one of the "scheduleCall" functions.  There are two varieties of each
    schedule call function - one takes a function with an extra starting parameter of DbConn*.  This
    variety will automatically fetch a DbConn from the appropriate connection pool before actually
    executing the method call.  The other variety of schedule call assumes the command already owns
    a DbConn and will schedule the call on the next available thread.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/dbconn.h"
#include "framework/database/query.h"
#include "framework/database/mysql/mysqlasyncconn.h"
#include "framework/system/fiber.h"
#include "framework/system/blazethread.h"
#include "framework/system/threadlocal.h"
#include "framework/system/timerqueue.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/controller/processcontroller.h"
#include "EASTL/list.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

DbScheduler::DbMetrics::DbMetrics()
    : mTimer(*Metrics::gFrameworkCollection, "database.queries.timer", Metrics::Tag::db_query_name, Metrics::Tag::fiber_context)
    , mSuccessCount(*Metrics::gFrameworkCollection, "database.queries.successCount", Metrics::Tag::db_query_name, Metrics::Tag::fiber_context)
    , mSlowQueryCount(*Metrics::gFrameworkCollection, "database.queries.slowQueryCount", Metrics::Tag::db_query_name, Metrics::Tag::fiber_context)
    , mFailCount(*Metrics::gFrameworkCollection, "database.queries.failCount", Metrics::Tag::db_query_name, Metrics::Tag::fiber_context, Metrics::Tag::rpc_error)
    , mTotalWorkerThreads(*Metrics::gFrameworkCollection, "database.workerThreads", []() { return gDbScheduler->getNumWorkers(); })
    , mActiveWorkerThreads(*Metrics::gFrameworkCollection, "database.activeWorkerThreads", []() { return gDbScheduler->getRunningJobCount(); })
    , mPeakWorkerThreads(*Metrics::gFrameworkCollection, "database.peakWorkerThreads", []() { return gDbScheduler->getPeakRunningJobCount(); })
{
}

void DbScheduler::DbMetrics::buildDbQueryAccountingMap(DbQueryAccountingInfos& map)
{
    mTimer.iterate([this, &map](const Metrics::TagPairList& tags, const Metrics::Timer& metric) {
            const char8_t* queryName = tags[0].second.c_str();
            DbQueryAccountingInfos::iterator itr = map.find(queryName);
            DbQueryAccountingInfo* accInfo;
            if (itr != map.end())
            {
                accInfo = itr->second;
            }
            else
            {
                accInfo = map.allocate_element();
                map[queryName] = accInfo;
            }
            accInfo->setTotalCount(accInfo->getTotalCount() + metric.getTotalCount());
            accInfo->setTotalTime(accInfo->getTotalTime() + metric.getTotalTime());
            accInfo->setSuccessCount(accInfo->getSuccessCount() + mSuccessCount.getTotal({ { Metrics::Tag::db_query_name, queryName } }));
            accInfo->setSlowQueryCount(accInfo->getSlowQueryCount() + mSlowQueryCount.getTotal({ { Metrics::Tag::db_query_name, queryName } }));
            accInfo->setFailCount(accInfo->getFailCount() + mFailCount.getTotal({ { Metrics::Tag::db_query_name, queryName } }));

            accInfo->getFiberCallCounts()[tags[1].second.c_str()] += metric.getTotalCount();

    });
    mFailCount.iterate([&map](const Metrics::TagPairList& tags, const Metrics::Counter& value) {
            const char8_t* queryName = tags[0].second.c_str();
            auto itr = map.find(queryName);
            if (itr != map.end())
            {
                DbQueryAccountingInfo* accInfo = itr->second;
                accInfo->getErrorCounts()[tags[2].second.c_str()] += value.getTotal();
            }
    });
}

/*! ***************************************************************************/
/*!    \brief  Initializes the internal DB connections from the db connection config map.

    \param config The config map.
    \return BlazeRpcError code (DB_ERR_OK on success)
*******************************************************************************/
BlazeRpcError DbScheduler::initFromConfig(const DatabaseConfiguration& config)
{
    {
        EA::Thread::AutoRWMutex autoRWMutex(mMutex,EA::Thread::RWMutex::kLockTypeWrite);

        if (!mInitialized)
        {
            mVerbatimQueryStringChecksEnabled = config.getVerbatimQueryStringChecksEnabled();

            config.getDbScheduler().copyInto(mDbSchedulerConfig);
            config.getDbmig().copyInto(mDbMigConfig);

            //If the python path for dbmig hasn't been specifically overridden, 
            //use a platform dependent default path
            if (*mDbMigConfig.getPythonPath() == '\0')
            {
#ifdef EA_PLATFORM_WINDOWS
                mDbMigConfig.setPythonPath("python");
#else
                mDbMigConfig.setPythonPath("/opt/local/bin/python");
#endif
            }

            for (DatabaseConfiguration::DbConfigMap::const_iterator itr = config.getDatabaseConnections().begin(),
                end = config.getDatabaseConnections().end(); itr != end; ++itr)
            {
                BlazeRpcError dbErr = addDbConfig(itr->first, *itr->second);
                if (dbErr != DB_ERR_OK)
                {
                    return dbErr;
                }
            }

            for (DatabaseConfiguration::DbAliasMap::const_iterator itr = config.getDatabaseAliases().begin(),
                end = config.getDatabaseAliases().end(); itr != end; ++itr)
            {
                const char8_t* alias = itr->first;
                const char8_t* name = itr->second;
                if (getDbIdInternal(name) == INVALID_DB_ID)
                {
                    BLAZE_FATAL_LOG(Log::DB, "[DbScheduler].initFromConfig: Database alias '" << alias << "' refers to a non-existent database configuration: '" << name << "'");
                    return DB_ERR_SYSTEM;
                }
            }
            config.getDatabaseAliases().copyInto(mConnPoolAliasMap);

            mInitialized = true;

            // Trigger all the worker threads to re-read the config
            ThreadPoolJobHandler::reconfigure();

            // Initialize the master DB conn pools
            for (ConnPoolMap::iterator itr = mConnPoolMap.begin(), end = mConnPoolMap.end(); itr != end; ++itr)
            {
                BlazeRpcError error = itr->second->initialize();
                if (error != DB_ERR_OK)
                {
                    return error;
                }
            }
        }

    }

    {
        EA::Thread::AutoRWMutex autoRWMutex(mMutex,EA::Thread::RWMutex::kLockTypeRead);
        if (!mStarted)
        {
            for (ConnPoolMap::iterator itr = mConnPoolMap.begin(), end = mConnPoolMap.end(); itr != end; ++itr)
            {
                BlazeRpcError error = itr->second->start();
                if (error != DB_ERR_OK)
                {
                    return error;
                }
            }
            mStarted = true;
        }
    }

    return DB_ERR_OK;
}

/*! ***************************************************************************/
/*!    \brief  Check whether the MySQL DB version is greater than 5.5.

    \return BlazeRpcError code (DB_ERR_OK on success)
*******************************************************************************/
BlazeRpcError DbScheduler::checkDBVersion()
{
    // for services that do not require DB
    if (mConnPoolMap.empty())
        return DB_ERR_OK;

    ConnPoolMap::const_iterator itr = mConnPoolMap.begin();
    ConnPoolMap::const_iterator end = mConnPoolMap.end();

    for (; itr != end; ++itr)
    {
        DbConnPool* pool = itr->second;
        if (pool->getDbConfig().getSummaryMaxConnCount() == 0)
        {
            continue;
        }

        DbConnPtr conn = getReadConnPtr(pool->getId());
        if (conn == nullptr)
        {
            BLAZE_FATAL_LOG(Log::DB, "[DbScheduler].checkDBVersion: DB connection pointer is nullptr.");
            return DB_ERR_SYSTEM;    
        }

        QueryPtr query = DB_NEW_QUERY_PTR(conn);
        query->append("SELECT version();");

        DbResultPtr dbResult;
        BlazeRpcError error = conn->executeQuery(query, dbResult);

        if (error == DB_ERR_OK)
        {
            if (dbResult != nullptr)
            {
                DbResult::const_iterator versionItr = dbResult->begin();
                const DbRow* row = *versionItr;
                const char8_t* version = row->getString("version()");
                if (blaze_strcmp(version, "5.5") < 0)
                {
                    BLAZE_FATAL_LOG(Log::DB, "[DbScheduler].checkDBVersion: Upgrade current MySQL verison '" << version << "' to at least 5.5");
                    return DB_ERR_SYSTEM;
                }
            }
        }
        else
        {
            BLAZE_FATAL_LOG(Log::DB, "[DbScheduler].checkDBVersion: Failed to execute query, got error " << (ErrorHelp::getErrorName(error)) );
            return error;
        }
    }
    return DB_ERR_OK;
}

BlazeRpcError DbScheduler::initSchemaVersions()
{
    typedef eastl::list<uint32_t> PoolIdList;
    PoolIdList schemaInfoPools;
    BlazeRpcError err = checkDBVersion();
    if (err != DB_ERR_OK)
    {
        return err;
    }

    {
        EA::Thread::AutoRWMutex autoRWmMutex(mMutex, EA::Thread::RWMutex::kLockTypeRead);

        for (ConnPoolMap::const_iterator itr = mConnPoolMap.begin(), end = mConnPoolMap.end(); itr != end; ++itr)
        {
            DbConnPool* pool = itr->second;
            if (!pool->isSchemaInfoInitialized() && (pool->getDbConfig().getSummaryMaxConnCount() != 0))
            {
                pool->setSchemaInfoInitialized();
                schemaInfoPools.push_back(pool->getId());
            }
        }
    }

    while (!schemaInfoPools.empty())
    {
        uint32_t poolId = schemaInfoPools.front();
        schemaInfoPools.pop_front();


        err = selectComponentSchemaVersions(poolId);
        
        // if schema info is not found then database is empty
        // which is valid situation
        if (err == DB_ERR_NO_SUCH_TABLE)
            err = DB_ERR_OK;

        if (err != DB_ERR_OK)
            return err;
    } 

    return err;
}

BlazeRpcError DbScheduler::selectComponentSchemaVersions(uint32_t poolId)
{
    DbConnPtr conn = getReadConnPtr(poolId);
    if (conn == nullptr)
        return DB_ERR_SYSTEM;

    const char8_t* dbName = getConfig(poolId)->getDatabase();
    QueryPtr query = DB_NEW_QUERY_PTR(conn);
    query->append("SELECT TABLE_NAME FROM INFORMATION_SCHEMA.TABLES WHERE TABLE_SCHEMA = '$s' AND TABLE_NAME = 'blaze_schema_info';", dbName);

    DbResultPtr dbResult;
    BlazeRpcError error = conn->executeQuery(query, dbResult);
    if (error != DB_ERR_OK)
    {
        return error;
    }
    if (dbResult == nullptr || dbResult->empty())
    {
        // no blaze_schema_info table is okay; if it is needed by any components using this conn pool, it will be created by dbmig
        return DB_ERR_OK;
    }

    DB_QUERY_RESET(query);
    query->append("SELECT version, component FROM blaze_schema_info;");
    error = conn->executeQuery(query, dbResult);

    if (error == DB_ERR_OK)
    {
        if (dbResult != nullptr)
        {
            EA::Thread::AutoRWMutex autoRWmMutex(mMutex, EA::Thread::RWMutex::kLockTypeRead);
            for (DbResult::const_iterator componentItr = dbResult->begin(), componentEnd = dbResult->end() ; componentItr != componentEnd; ++componentItr)
            {
                const char8_t* componentName = (*componentItr)->getString("component");
                uint32_t componentVersion = (*componentItr)->getUInt("version");

                ConnPoolMap::const_iterator itr = mConnPoolMap.find(poolId);
                if (itr != mConnPoolMap.end())
                {
                    itr->second->addComponentSchemaRecord(componentName, componentVersion);
                }
            }
        }
    }
    return error;
}

void DbScheduler::reconfigure(const DatabaseConfiguration& config)
{
    mVerbatimQueryStringChecksEnabled = config.getVerbatimQueryStringChecksEnabled();

    mTempDbSchedulerConfig.copyInto(mDbSchedulerConfig);

    // Reconfigure any existing DbConnPool
    for (DatabaseConfiguration::DbConfigMap::const_iterator it = config.getDatabaseConnections().begin(), 
        itend = config.getDatabaseConnections().end();
        it != itend;
        ++it)
    {
        uint32_t dbId = getDbId(it->first);
        if (dbId == INVALID_DB_ID)
            continue;

        DbConnPool *dbConnPool = getConnPool(dbId);
        if (dbConnPool == nullptr) // this should never happen, but just incase
            continue;

        dbConnPool->reconfigure(*it->second);
    }

    ThreadPoolJobHandler::reconfigure();
}

void DbScheduler::prepareForReconfigure(const DatabaseConfiguration& config)
{
    config.getDbScheduler().copyInto(mTempDbSchedulerConfig);

    // Reconfigure any existing DbConnPool
    for (DatabaseConfiguration::DbConfigMap::const_iterator it = config.getDatabaseConnections().begin(), 
        itend = config.getDatabaseConnections().end();
        it != itend;
        ++it)
    {
        uint32_t dbId = getDbId(it->first);
        if (dbId == INVALID_DB_ID)
            continue;

        DbConnPool *dbConnPool = getConnPool(dbId);
        if (dbConnPool == nullptr) // this should never happen, but just incase
            continue;

        dbConnPool->prepareForReconfigure(*it->second);
    }
}

void DbScheduler::validateConfig(DatabaseConfiguration& config, const DatabaseConfiguration* refConfig, ConfigureValidationErrors& validationErrors)
{
    StringBuilder sb;
    auto& schedulerConfig = config.getDbScheduler();
    if ((schedulerConfig.getDbQueryVarianceThreshold() < 0) || (schedulerConfig.getDbQueryVarianceThreshold() > schedulerConfig.getDbSquelchingMaxQueryVariance()))
    {
        sb.reset() << "[DbScheduler].validateConfig: dbQueryVarianceThreshold must be between 0.0 and dbSquelchingMaxQueryVariance. Configured value=" << schedulerConfig.getDbQueryVarianceThreshold();
        validationErrors.getErrorMessages().push_back(sb.c_str());
    }

    if (schedulerConfig.getDbQueryVarianceHistorySize() < 8)
    {
        sb.reset() << "[DbScheduler].validateConfig: dbQueryVarianceHistorySize must have a minimum value of 8. Configured value=" << schedulerConfig.getDbQueryVarianceHistorySize();
        validationErrors.getErrorMessages().push_back(sb.c_str());
    }

    if ((schedulerConfig.getDbSquelchingMinConnectionCountPercent() < 0) || (schedulerConfig.getDbSquelchingMinConnectionCountPercent() > 1))
    {
        sb.reset() << "[DbScheduler].validateConfig: dbSquelchingMinConnectionCountPercent must be between 0.0 and 1.0. Configured value=" << schedulerConfig.getDbSquelchingMinConnectionCountPercent();
        validationErrors.getErrorMessages().push_back(sb.c_str());
    }

    if (schedulerConfig.getDbSquelchingMaxQueryVariance() <= 1)
    {
        sb.reset() << "[DbScheduler].validateConfig: dbSquelchingMaxQueryVariance must be greater than 1.0. Configured value=", schedulerConfig.getDbSquelchingMaxQueryVariance();
        validationErrors.getErrorMessages().push_back(sb.c_str());
    }

    if ((schedulerConfig.getDbConnectionTrimThreshold() < 0) || (schedulerConfig.getDbConnectionTrimThreshold() > 1))
    {
        sb.reset() << "[DbScheduler].validateConfig: dbConnectionTrimThreshold must be between 0.0 and 1.0. Configured value=", schedulerConfig.getDbConnectionTrimThreshold();
        validationErrors.getErrorMessages().push_back(sb.c_str());
    }

    // Iterate all the db instance configurations and merge all their overrides.
    for (auto& dbConnItr : config.getDatabaseConnections())
    {
        auto& dbConfig = *dbConnItr.second;
        const auto* hostname = dbConnItr.second->getHostname();
        if (hostname[0] == '\0')
        {
            sb.reset() << "[DbScheduler].validateConfig: Invalid configuration, database(" << dbConnItr.first << ") hostname(" << hostname << ") cannot be empty.";
            validationErrors.getErrorMessages().push_back(sb.c_str());
        }

        // First merge overrides for master
        auto overrideItr = dbConfig.getInstanceOverrides().find(gController->getBaseName());
        if (overrideItr != dbConfig.getInstanceOverrides().end())
        {
            //now we only support overriding the max conn count
            dbConfig.setMaxConnCount(overrideItr->second->getMaxConnCount());
        }

        // Now merge overrides for slaves. Note that copyInto() repopulates collections; this
        // means that if the master has a list of slaves in its instance override, then we consider the 
        // overridden list when merging overrides for slaves.
        for (auto& dbSlaveConfigItr : dbConfig.getSlaves())
        {
            auto& dbSlaveConfig = *dbSlaveConfigItr;
            auto slaveOverrideItr = dbSlaveConfig.getInstanceOverrides().find(gController->getBaseName());
            if (slaveOverrideItr != dbSlaveConfig.getInstanceOverrides().end())
            {
                //now we only support overriding the max conn count
                dbSlaveConfig.setMaxConnCount(slaveOverrideItr->second->getMaxConnCount());
            }
        }
    }

    if (refConfig != nullptr)
    {
        // onReconfigure validation
        if (config.getDatabaseConnections().size() != refConfig->getDatabaseConnections().size())
        {
            sb.reset() << "[DbScheduler].validateConfig: Unsupported operation - attempt to add or remove a db master - new database config has "<< config.getDatabaseConnections().size() <<" configured master(s) (vs "<< refConfig->getDatabaseConnections().size() <<" in old config).";
            validationErrors.getErrorMessages().push_back(sb.c_str());
            return;
        }

        auto refDbConnMapItr = refConfig->getDatabaseConnections().begin();
        for (auto dbConnMapItr = config.getDatabaseConnections().begin(), dbConnMapEnd = config.getDatabaseConnections().end(); dbConnMapItr != dbConnMapEnd; ++dbConnMapItr, ++refDbConnMapItr)
        {
            // NOTE: Validating matching hostname/port is currently needed for master dbs to ensure no changes because DB slaves are reconfigurable but masters are not, since they both use a shared DbConnConfig TDF for legacy reasons those two fields must be marked as reconfigurable=yes. (The description of these fields mentions this detail)
            // FUTURE: The right thing to do eventually is to enable reconfiguring host/port for the master DB as well; however, this will require *extensive* testing.
            if (blaze_stricmp(refDbConnMapItr->first.c_str(), dbConnMapItr->first.c_str()) != 0)
            {
                sb.reset() << "[DbScheduler].validateConfig: Unsupported operation - attempt to add or remove a db master database(" << dbConnMapItr->first.c_str() << ") (vs " << refDbConnMapItr->first.c_str() << " in old config).";
                validationErrors.getErrorMessages().push_back(sb.c_str());
                return;
            }
            if (blaze_stricmp(refDbConnMapItr->second->getHostname(), dbConnMapItr->second->getHostname()) != 0)
            {
                sb.reset() << "[DbScheduler].validateConfig: Unsupported operation - attempt to change hostname for db master database(" << dbConnMapItr->first.c_str() << ") to hostname(" << dbConnMapItr->second->getHostname() << ") (vs " << refDbConnMapItr->second->getHostname() << " in old config).";
                validationErrors.getErrorMessages().push_back(sb.c_str());
                return;
            }
            if (refDbConnMapItr->second->getPort() != dbConnMapItr->second->getPort())
            {
                sb.reset() << "[DbScheduler].validateConfig: Unsupported operation - attempt to change port for db master database(" << dbConnMapItr->first.c_str() << ") to port(" << dbConnMapItr->second->getPort() << ") (vs " << refDbConnMapItr->second->getPort() << " in old config).";
                validationErrors.getErrorMessages().push_back(sb.c_str());
                return;
            }

            // Copy the ref master DB config
            DbConnConfig tempDbConnConfig;
            refDbConnMapItr->second->copyInto(tempDbConnConfig);

            // Now copy the new config's slaves.
            // We will later validate the slaves by first creating a DbConfig for the master and then iterating through the 
            // master's slaves (see below). We need to preserve change tracking info for the slaves because when we create 
            // the master DbConfig, this info is used to determine which properties a slave DbConfig needs to inherit from the master.
            EA::TDF::MemberVisitOptions opts;
            opts.onlyIfSet = true;
            dbConnMapItr->second->getSlaves().copyInto(tempDbConnConfig.getSlaves(), opts);

            // We only need to validate the slaves because the master config is not reconfigurable
            // (except for maxConnCount), but we may need to inherit values from the master, 
            // so create a DbConfig for it first and then iterate through its slaves
            const DbConfig masterConfig(refDbConnMapItr->first, tempDbConnConfig);
            for (auto& dbConnItr : masterConfig.getSlaves())
            {
                // Note: already validated overall there is non-zero max conn count, across all instances, in mergeInstanceSpecificConfigOverrides
                if (dbConnItr->getMaxConnCount() == 0)
                {
                    BLAZE_WARN_LOG(Log::DB, "[DbScheduler].validateConfig: databaseConnection(" << dbConnMapItr->first << ") has a slave(" << (dbConnItr->getName() != nullptr ? dbConnItr->getName() : "nullptr") << ") with maxConnCount of zero. Calls to get db connections on this type(" << gController->getBaseName() << ") will fail.");
                    continue;
                }

                DbConnPtr conn = getReadConnPtr(getDbId(dbConnMapItr->first), Log::DB);
                if (conn == nullptr)
                {
                    sb.reset() << "[DbScheduler].validateConfig: Failed to obtain a database(" << dbConnMapItr->first << ") slave connection to " << (dbConnItr)->getHost() << ":" << (dbConnItr)->getPort() << "";
                    validationErrors.getErrorMessages().push_back(sb.c_str());
                    return;
                }

                BlazeRpcError err = conn->getDbConnInterface().testConnect(*dbConnItr);
                if (err != ERR_OK)
                {
                    sb.reset() << "[DbScheduler].validateConfig: Failed to establish a database(" << dbConnMapItr->first << ") slave connection to " << (dbConnItr)->getHost() << ":" << (dbConnItr)->getPort() << "";
                    validationErrors.getErrorMessages().push_back(sb.c_str());
                    return;
                }
            }
        }
    }
}

/*! ***************************************************************************/
/*!    \brief Gets the id of a database by name.

    Will look at the database name and any aliases it goes by when trying
    to find a match.

    \param dbName  The name of the database to look up.
    \return  The db id or INVALID_DB_ID if not found.
*******************************************************************************/
uint32_t DbScheduler::getDbId(const char8_t *dbName)
{
    EA::Thread::AutoRWMutex autoRWmMutex(mMutex, EA::Thread::RWMutex::kLockTypeRead);

    uint32_t result = getDbIdInternal(dbName);
    return result;
}

uint32_t DbScheduler::getDbIdInternal(const char8_t *dbName)
{
    uint32_t result = INVALID_DB_ID;
    
    if (dbName != nullptr)
    {
        //Loop over all the DBs and find the one that matches.  Be sure to check
        DatabaseConfiguration::DbAliasMap::iterator aliasItr = mConnPoolAliasMap.find(dbName);
        if (aliasItr != mConnPoolAliasMap.end())
            dbName = aliasItr->second.c_str();

        //Loop over all the DBs and find the one that matches.
        ConnPoolMap::const_iterator itr = mConnPoolMap.begin();
        ConnPoolMap::const_iterator end = mConnPoolMap.end();
        for (; itr != end; ++itr)
        {
            if (blaze_stricmp(dbName, itr->second->getName()) == 0)
            {
                result = itr->first;
                break;
            }
        }
    }

    return result;
}

/*! ***************************************************************************/
/*!    \brief Gets the configuration for a specified DB id.

    \param dbId The id of the configuration to fetch.
    \return  The configuration, or nullptr if not found.
*******************************************************************************/
const DbConfig *DbScheduler::getConfig(uint32_t dbId)
{
    const DbConfig *result = nullptr;

    EA::Thread::AutoRWMutex autoRWmMutex(mMutex, EA::Thread::RWMutex::kLockTypeRead);
    ConnPoolMap::const_iterator itr = mConnPoolMap.find(dbId);
    if (itr != mConnPoolMap.end())
    {
        result = &itr->second->getDbConfig();
    }
    return result;
}

/*! ***************************************************************************/
/*!    \brief This function gets a connection and blocks the current fiber 
           while waiting for it.

    \return 
*******************************************************************************/
DbConn *DbScheduler::getConn(uint32_t dbId, DbConn::Mode mode, int32_t logCategory, bool forceReadWithoutRepLag)
{
    if (!mAllowRequests)
        return nullptr;

    DbConn *result = nullptr;
    DbConnPool *pool = getConnPool(dbId);
    
    if (pool != nullptr)
    {
        // Since MySQL master/slave does not have synchronous write replication (i.e. like Galera),
        // a read after a write may need to be executed on the master DB host
        // instead of a slave DB host to avoid replication lag.
        if (mode == DbConn::READ_ONLY && forceReadWithoutRepLag && !pool->isWriteReplicationSynchronous())
        {
            BLAZE_TRACE_LOG(Log::DB, "[DbScheduler].getConn: forcing mode from RO to RW to avoid rep lag for db id " << dbId);
            mode = DbConn::READ_WRITE;
        }

        BlazeRpcError err = pool->getConn(mode, logCategory, result);
        if (err != ERR_OK)
            return nullptr;
    }
    return result;
}


DbConnPtr DbScheduler::getConnPtr(uint32_t dbId, int32_t logCategory)
{
    DbConnPtr ptr;
    ptr.attach(getConn(dbId, DbConn::READ_WRITE, logCategory, false));
    return ptr;
}


DbConnPtr DbScheduler::getReadConnPtr(uint32_t dbId, int32_t logCategory)
{
    DbConnPtr ptr;
    ptr.attach(getConn(dbId, DbConn::READ_ONLY, logCategory, false));
    return ptr;
}


DbConnPtr DbScheduler::getLagFreeReadConnPtr(uint32_t dbId, int32_t logCategory)
{
    DbConnPtr ptr;
    ptr.attach(getConn(dbId, DbConn::READ_ONLY, logCategory, true));
    return ptr;
}


void DbScheduler::getStatusInfo(DatabaseStatus& status) const
{
    status.setTotalWorkerThreads((uint32_t)mDbMetrics.mTotalWorkerThreads.get());
    status.setActiveWorkerThreads((uint32_t)mDbMetrics.mActiveWorkerThreads.get());
    status.setPeakWorkerThreads((uint32_t)mDbMetrics.mPeakWorkerThreads.get());

    EA::Thread::AutoRWMutex autoRWmMutex(mMutex, EA::Thread::RWMutex::kLockTypeRead);

    DatabaseStatus::ConnectionPoolsMap& poolMap = status.getConnectionPools();
    for (ConnPoolMap::const_iterator itr = mConnPoolMap.begin(), end = mConnPoolMap.end(); itr != end; ++itr)
    {
        DbConnPool* pool = itr->second;
        DbConnPoolStatus* poolStatus = BLAZE_NEW DbConnPoolStatus;
        pool->getStatusInfo(*poolStatus);
        poolMap[itr->first] = poolStatus;
    }
}

uint32_t DbScheduler::getComponentVersion(uint32_t db, const char8_t* component)
{
    uint32_t result = 0;

    EA::Thread::AutoRWMutex autoRWmMutex(mMutex, EA::Thread::RWMutex::kLockTypeRead);

    ConnPoolMap::iterator itr = mConnPoolMap.find(db);
    if (itr != mConnPoolMap.end())
    {
        result =  itr->second->getComponentSchemaVersion(component);
    }

    return result;
}

void DbScheduler::updateComponentVersion(uint32_t db, const char8_t* component, uint32_t newVersion)
{
    EA::Thread::AutoRWMutex autoRWmMutex(mMutex, EA::Thread::RWMutex::kLockTypeRead);

    ConnPoolMap::iterator itr = mConnPoolMap.find(db);
    if (itr != mConnPoolMap.end())
    {
        itr->second->updateComponentSchemaVersion(component, newVersion);
    }
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/


/*! ***************************************************************************/
/*!    \brief Adds a database configuration to the scheduler.

\param config  The configuration to add.
\return BlazeRpcError code (DB_ERR_OK on success, DB_ERR_INIT_FAILED on failure)
*******************************************************************************/
BlazeRpcError DbScheduler::addDbConfig(const char8_t* name, const DbConnConfig& config)
{
    DbConfig testConfig(name, config);

    //Add a new conn pool
    DbConnPool* pool = BLAZE_NEW DbConnPool(name, config, mNextIdentifier++);
    mConnPoolMap[pool->getId()] = pool;

    return DB_ERR_OK;
}

DbScheduler::DbScheduler() : ThreadPoolJobHandler(0, "db", BlazeThread::DB_WORKER), 
    mInitialized(false),
    mStarted(false),
    mSchemaVersionInitialized(false),
    mNextIdentifier(0),
    mVerbatimQueryStringChecksEnabled(false),
    mAllowRequests(true)
{
    const char8_t* path = strchr(gProcessController->getVersion().getBuildLocation(), ':');
    if (path != nullptr)
        mBuildPath = (path + 1);
}

DbScheduler::~DbScheduler()
{
    //We need to stop before cleaning up:
    stop();
}

void DbScheduler::start()
{
    ThreadPoolJobHandler::start();
}

void DbScheduler::stop()
{
    //We need to stop before cleaning up:
    ThreadPoolJobHandler::stop();

    //Kill our pools
    ConnPoolMap connPoolMap = eastl::move(mConnPoolMap);
    ConnPoolMap::iterator itr = connPoolMap.begin();
    ConnPoolMap::iterator end = connPoolMap.end();
    for (; itr != end; ++itr)
    {
        delete itr->second;
    }
}

/*! ***************************************************************************/
/*!    \brief Gets a DB Conn pool in a thread safe manger.

    \param id Id of the pool to get.
    \return  The pool or nullptr if not found.
*******************************************************************************/
DbConnPool *DbScheduler::getConnPool(uint32_t id)
{
    if (id == INVALID_DB_ID)
    {
        BLAZE_ERR_LOG(Log::DB, "[DbScheduler].getConnPool: invalid db id passed for obtaining connection pool. ");
        return nullptr;
    }
    
    DbConnPool *result = nullptr;

    EA::Thread::AutoRWMutex autoRWmMutex(mMutex, EA::Thread::RWMutex::kLockTypeRead);

    ConnPoolMap::iterator itr = mConnPoolMap.find(id);
    if (itr != mConnPoolMap.end())
    {
        result = itr->second;
    }
    else
    {
        BLAZE_ERR_LOG(Log::DB, "[DbScheduler].getConnPool: connection pool not found for db id " << id);
    }

    return result;
}

/*! ***************************************************************************/
/*!    \brief Internally sets up and queues a job.
 
    If a connection is required for this job, that is acquired first before
    the job is actually queued.

    \param id The id of the db to queue the job for.
    \param job The job to queue.
*******************************************************************************/
BlazeRpcError DbScheduler::queueJob(uint32_t id, DbScheduleJob &job, bool block, bool* jobWasQueued)
{
    if (jobWasQueued != nullptr)
        *jobWasQueued = false;

    VERIFY_WORKER_FIBER();

    if (EA_UNLIKELY(block && !Fiber::isCurrentlyBlockable()))
    {
        delete &job;
        return ERR_SYSTEM;
    }

    DbConnPool *pool = getConnPool(id);
    if (pool != nullptr)
    {
        //Set the job's DB pool and queue it up to run
        job.setDbConnPool(pool);

        if (job.isConnectionNeeded() && !job.hasDbConn())
        {
            DbConn* conn = nullptr;
            BlazeRpcError err = pool->getConn(job.getDbConnMode(), job.getLogCategory(), conn);
            if (err != ERR_OK)
            {
                delete &job;
                return err;
            }
            job.setDbConn(conn);
        }

        if (pool->getUseAsyncDbConns())
        {
            job.execute();
            delete &job;
        }
        else
        {
            Fiber::EventHandle ev;
            if (block)
            {
                ev = Fiber::getNextEventHandle();
                job.setEventHandle(ev);
                job.setLogContext(*gLogContext);
                job.setTimeout(Fiber::getCurrentTimeout().getMicroSeconds());
            }

            ThreadPoolJobHandler::queueJob(job);

            if (jobWasQueued != nullptr)
                *jobWasQueued = true;

            //Its not a problem to do this after the queue - the DB thread has to come through the
            //caller's selector thread to schedule the fiber, which means it can only happen after we
            //exit out of here.
            if (block)
            {
                BlazeRpcError err = Fiber::wait(ev, "DbScheduler::queueJob");
                if (err != ERR_OK)
                    return err;
            }
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::DB, "[DbScheduler].queueJob: Connection pool does not exist for id=" << id);
        delete &job;
        return ERR_SYSTEM;
    }
    return ERR_OK;
}


/*! ***************************************************************************/
/*!    \brief Overridden to set the gIsDbThread global variable. Only DB threads 
           should have this set.
           Calls the DbAdmin::registerDbThread() function for all ConnPools.
*******************************************************************************/
void DbScheduler::threadStart()
{
    {
        EA::Thread::AutoRWMutex autoRWmMutex(mMutex, EA::Thread::RWMutex::kLockTypeRead);

        for (ConnPoolMap::iterator connPoolItr = mConnPoolMap.begin(), connPoolEnd = mConnPoolMap.end(); connPoolItr != connPoolEnd; ++connPoolItr)
        {
            if (!connPoolItr->second->getUseAsyncDbConns())
                connPoolItr->second->registerDbThread();
        }
    }

    gIsDbThread = true;
    gDbThreadTimeout = 0;
    gDbScheduler = this;
}

/*! ***************************************************************************/
/*! \brief Overriden to call the DbAdmin::unregisterDbThread() function for all ConnPools.
*******************************************************************************/
void DbScheduler::threadEnd()
{
    EA::Thread::AutoRWMutex autoRWmMutex(mMutex, EA::Thread::RWMutex::kLockTypeRead);

    for (ConnPoolMap::iterator connPoolItr = mConnPoolMap.begin(), connPoolEnd = mConnPoolMap.end(); connPoolItr != connPoolEnd; ++connPoolItr)
    {
        if (!connPoolItr->second->getUseAsyncDbConns())
            connPoolItr->second->unregisterDbThread();
    }
}

void DbScheduler::threadReconfigure(int32_t workerId)
{
}

/*! ***************************************************************************/
/*!    \brief Sets up the DbConection before running a job.

    This function is overridden to lookup the DbConn before running a job. This 
    is run on the blocking thread so its fine to get the connection from the conn
    before running.

    \param job  The job to run.
*******************************************************************************/
void DbScheduler::executeJob(Job& job)
{
    //This is safe since we've guarded any access to the internal job queue.
    DbScheduleJob& dbjob = (DbScheduleJob&) job;
    if (dbjob.getEventHandle().isValid())
    {
        gDbThreadTimeout = dbjob.getTimeout();
        *gLogContext = *(dbjob.getLogContext());
    }

    //Now execute the job.
    dbjob.execute();

    //Did the job have fiber data? If so schedule the fiber
    if (dbjob.getEventHandle().isValid())
    {
        Fiber::signal(dbjob.getEventHandle(), ERR_OK);
    }

    gDbThreadTimeout = 0;
    gLogContext->clear();

    delete &dbjob;
}

/*! ***************************************************************************/
/*!    \brief Register a prepared statement.
          
    This function will register a prepared statement with the connection pool
    that is associated with the given DB id.

    \param dbId  DB Id of pool to register statement against
    \param name  Name of the prepared statement
    \param query The query to register.

    \return The unique identifier for the registered statement
*******************************************************************************/
PreparedStatementId DbScheduler::registerPreparedStatement(
        uint32_t dbId, const char8_t* name, const char8_t* query)
{
    DbConnPool* pool = getConnPool(dbId);
    if (pool == nullptr)
        return INVALID_PREPARED_STATEMENT_ID;
    return pool->registerPreparedStatement(name, query);
}

void DbScheduler::tickQuery(const char8_t* queryName, const TimeValue& time)
{
    mDbMetrics.mTimer.record(time, queryName, Fiber::getCurrentContext());
}

void DbScheduler::tickQuerySuccess(const char8_t* queryName)
{
    mDbMetrics.mSuccessCount.increment(1, queryName, Fiber::getCurrentContext());
}

void DbScheduler::tickSlowQuery(const char8_t* queryName)
{
    mDbMetrics.mSlowQueryCount.increment(1, queryName, Fiber::getCurrentContext());
}

void DbScheduler::tickQueryFailure(const char8_t* queryName, BlazeRpcError err)
{
    mDbMetrics.mFailCount.increment(1, queryName, Fiber::getCurrentContext(), err);
}

void DbScheduler::buildDbQueryAccountingMap(DbQueryAccountingInfos& map)
{
    mDbMetrics.buildDbQueryAccountingMap(map);
}

bool DbScheduler::parseDbNameByPlatformTypeMap(const char8_t* componentName, const DbNameByPlatformTypeMap& dbNameByPlatformTypeMap, DbIdByPlatformTypeMap& dbIdByPlatformTypeMap)
{
    if (dbNameByPlatformTypeMap.empty())
    {
        BLAZE_ERR_LOG(Log::DB, "[DbScheduler::parseDbNameByPlatformTypeMap] Empty dbNameByPlatformTypeMap for component '" << componentName << "' provided. No Db Ids can be parsed.");
        return false;
    }

    for (DbNameByPlatformTypeMap::const_iterator itr = dbNameByPlatformTypeMap.begin(); itr != dbNameByPlatformTypeMap.end(); ++itr)
    {
        if (!gController->isPlatformUsed(itr->first))
        {
            BLAZE_INFO_LOG(Log::DB, "[DbScheduler::parseDbNameByPlatformTypeMap] Ignoring db config with name '" << itr->second.c_str() << "' configured by component '" << componentName << "' for platform '" << ClientPlatformTypeToString(itr->first) << "' (not hosted)");
            continue;
        }

        // Fail if one of the db names was invalid
        uint32_t dbId = gDbScheduler->getDbId(itr->second.c_str());
        if (dbId == INVALID_DB_ID)
        {
            //No config.  No choice but to bail.
            BLAZE_ERR_LOG(Log::DB, "[DbScheduler::parseDbNameByPlatformTypeMap] no valid configuration found for db config name '" << itr->second.c_str() << "' configured by component '" << componentName << "' for platform '" << ClientPlatformTypeToString(itr->first) << "'");
            return false;
        }

        dbIdByPlatformTypeMap[itr->first] = dbId;
    }

    return true;
}


} // namespace Blaze

