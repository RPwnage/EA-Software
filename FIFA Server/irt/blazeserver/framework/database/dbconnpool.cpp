/*************************************************************************************************/
/*!
    \file   dbconnpool.cpp


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class DbConnPool

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbconninterface.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/dbadmin.h"
#include "framework/database/dbscheduler.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"
#include "EASTL/algorithm.h"
#include "framework/util/random.h"
#include "framework/database/mysql/mysqladmin.h"
#include "framework/database/mysql/mysqlasyncconn.h"

namespace Blaze
{

/*************************************************************************************************/
/*!
\brief getConn

Takes the first element off the queue and returns it.

\param block If true the thread will block while waiting for the connection if none is available. 

\return - First element of the queue.

*/
/*************************************************************************************************/

BlazeRpcError DbConnPool::getConn(DbConn::Mode mode, int32_t logCategory, DbConn*& conn)
{
    VERIFY_WORKER_FIBER();

    conn = nullptr;
    if (getMaxConnCountForMode(mode) == 0)
    {
        BLAZE_ERR_LOG(logCategory, "[DbConnPool].getConn: not allowed, due to DB maxConnCount 0, for (" << mDbConfig.getName() << "), mode(" << DbConn::modeToString(mode) << ")");
        return ERR_DB_NO_CONNECTION_AVAILABLE;
    }

    if (EA_UNLIKELY(!gDbScheduler->getAllowRequests()))
    {
        BLAZE_INFO_LOG(logCategory, "[DbConnPool].getConn: canceled due to DB scheduler denying requests"); 
        return ERR_CANCELED;
    }

    lockMutex();

    bool squelching = false;
    conn = activateClientConnection(mode, squelching);
    Fiber::EventHandle eventHandle;
    if (conn != nullptr)
    {
        EA_ASSERT(conn->getRefCount() == 0);
        unlockMutex();
    }
    else if (squelching)
    {
        unlockMutex();
        // The pool a connection is being requested from is currently squelching because the DB
        // is down so fast-fail the request
        BLAZE_ERR_LOG(logCategory, "Unable to acquire connection due to squelching; fiber=" << Fiber::getCurrentContext());
        return ERR_SYSTEM;
    }
    else
    {
        // No connections are available so queue the request
        GetConnQueue* waiterQueue = nullptr;
        switch (mode)
        {
            case DbConn::READ_WRITE:
                waiterQueue = &mMasterConnectionWaiters;
                break;
            case DbConn::READ_ONLY:
            default: //There's no other choice

                // (As done in activateClientConnection) if no slave connections configured, use the master pool instead.
                waiterQueue = (!mSlaves.empty() ? &mSlaveConnectionWaiters : &mMasterConnectionWaiters);
                break;          
        }
        eventHandle = Fiber::getNextEventHandle();
        waiterQueue->push(ConnectionWaiter(&conn, eventHandle));

        BLAZE_TRACE_LOG(Log::DB, "[DbConnPool].getConn: no connections available; queuing request for fiber " << Fiber::getCurrentContext() << "; event(" << eventHandle.eventId <<
            "); maxConnCount(" << mDbConfig.getMaxConnCount() << "); summaryMaxConnCount(" << mDbConfig.getSummaryMaxConnCount() << "); slave count(" << mSlaves.size() <<
            "); masterConnectionWaiters count(" << mMasterConnectionWaiters.size() << "); slaveConnectionWaiters count(" << mSlaveConnectionWaiters.size() << ")");

        unlockMutex();

        // Sleep this fiber until a connection is available (or it times out).  The result
        // pointer should be set once the fiber awakens (or set to nullptr on timeout).
        BlazeRpcError error = Fiber::wait(eventHandle, Metrics::Tag::DB_GET_CONN_CONTEXT);
        if (error != Blaze::ERR_OK)
        {
            BLAZE_ERR_LOG(Log::DB, "[DbConnPool].getConn: timed out fiber " << Fiber::getCurrentContext() << " "
                "waiting for a connection; event(" << eventHandle.eventId << ")");
            BLAZE_ERR_LOG(logCategory, "Unable to acquire connection: " << (ErrorHelp::getErrorName(error)) << "; fiber=" << Fiber::getCurrentContext());

            lockMutex();
            GetConnQueue::container_type::iterator itr = waiterQueue->get_container().begin();
            GetConnQueue::container_type::iterator end = waiterQueue->get_container().end();
            for(; itr != end; ++itr)
            {
                if (itr->getEventHandle() == eventHandle)
                {
                    //Remove us
                    waiterQueue->get_container().erase(itr);
                    break;
                }
            }
            unlockMutex();
            
            if (conn != nullptr)
            {
                // Release the connection back to the pool
                // since we cannot use it due to error.
                release(*conn);
            }
            
            return ERR_SYSTEM;
        }
    }


    if (conn != nullptr)
    {
        BLAZE_TRACE_LOG(Log::DB, "[DbConnPool].getConn: acquired connection " << conn->getName() << "; current conn "
                "size:" << conn->getDbInstancePool().getActivePoolCount() << " and total conn size:" << conn->getDbInstancePool().getInactivePoolCount() << ".");
        BLAZE_TRACE_DB_LOG(logCategory, "Acquire connection: conn=" << conn->getName() << "; fiber=" << Fiber::getCurrentContext());
        conn->setLogCategory(logCategory);
        EA_ASSERT(conn->getRefCount() == 0);
        conn->assign();
        return ERR_OK;
    }
    return ERR_SYSTEM;
}


/*************************************************************************************************/
/*!
    \brief release

    Puts an DbConn into the queue (at the back).

    \param[in]  DbConn - DbConn to be pushed.

    \return - returns the DbConn that got pushed.

*/
/*************************************************************************************************/
void DbConnPool::release(DbConnBase& connBase)
{
    //  up-cast safe.
    DbConn& conn = static_cast<DbConn&>(connBase);

    bool replace = false;

    BLAZE_TRACE_LOG(Log::DB, "[DbConnPool].release: releasing connection("<<&conn<<":" << conn.getName() << ") refcount:"<< conn.getRefCount() << " to pool.");
    if (EA_UNLIKELY(conn.getRefCount()>0))
    {
        BLAZE_ERR_LOG(Log::DB, "[DbConnPool].release: releasing connection("<<&conn<<":" << conn.getName() << ") to pool but refcount is non-zero("<< conn.getRefCount() << ")");

        replace = true;
    }

    // cache off name and mode so we can safely use it after conn has been released
    char8_t connName[64] = { '\0' };
    if (BLAZE_IS_LOGGING_ENABLED(Log::DB, Logging::TRACE))
    {
        blaze_strnzcpy(connName, conn.getName(), sizeof(connName));
    }
    const DbConnBase::Mode mode = conn.getMode();

    // Try to give the connection to someone waiting for one
    lockMutex();

    // deactivate may delete conn, *never* access conn after deactivation
    deactivateClientConection(conn, replace);

    Fiber::EventHandle eventHandle;
    DbConn* waiterConn = nullptr;
    bool squelching = false;
    switch (mode)
    {
        case DbConn::READ_WRITE:
            if (!mMasterConnectionWaiters.empty())
            {
                waiterConn = activateClientConnection(DbConn::READ_WRITE, squelching);
                if (waiterConn != nullptr)
                {
                    ConnectionWaiter waiter = mMasterConnectionWaiters.top();
                    *(waiter.getDbConnPtr()) = waiterConn;
                    eventHandle = waiter.getEventHandle();
                    mMasterConnectionWaiters.pop();
                }
            }
            break;

        case DbConn::READ_ONLY:

            // (As done in activateClientConnection) if no slave connections configured, use the master pool instead.
            GetConnQueue& waiterQueue = (!mSlaves.empty() ? mSlaveConnectionWaiters : mMasterConnectionWaiters);

            if (!waiterQueue.empty())
            {
                waiterConn = activateClientConnection(DbConn::READ_ONLY, squelching);
                if (waiterConn != nullptr)
                {
                    ConnectionWaiter waiter = waiterQueue.top();
                    *(waiter.getDbConnPtr()) = waiterConn;
                    eventHandle= waiter.getEventHandle();
                    waiterQueue.pop();
                }
            }
            break;
    }

    if (eventHandle.isValid())
    {
        BLAZE_TRACE_LOG(Log::DB, "[DbConnPool].release: connection " << connName << " now available; "
                "signaling event " << eventHandle.eventId);
      
        unlockMutex();

        Fiber::signal(eventHandle, ERR_OK);
        
        // We took care of this conn, exit
        return;
    }

    unlockMutex();
}

void DbConnPool::getStatusInfo(DbConnPoolStatus& status) const
{
    status.setName(mDbConfig.getName());
    status.setDatabase(mDbConfig.getDatabase());
    status.setUsingDbThreads(!getUseAsyncDbConns());

    status.setQueuedFibersMaster((uint32_t)mQueuedFibersMaster.get());
    status.setQueuedFibersSlaves((uint32_t)mQueuedFibersSlaves.get());

    if (mMaster != nullptr)
    {
        mMaster->getStatus(status.getMaster());
    }
    if (!mSlaves.empty())
    {
        status.getSlaves().reserve(mSlaves.size());
        for(SlaveList::const_iterator itr = mSlaves.begin(), end = mSlaves.end(); itr != end; ++itr)
        {
            DbInstancePoolStatus* slaveStatus = status.getSlaves().pull_back();
            (*itr).getStatus(*slaveStatus);
        }
    }
}

BlazeRpcError DbConnPool::initialize()
{
    // The DLL/shared object file was actually loaded first in the DbScheduler::createDrivers() call.
    // If that call failed we won't get to here.  This is just going to bump the open ref count and 
    // read in some functions.  It's can't really fail.
    char8_t adminFuncName[256];
    blaze_snzprintf(adminFuncName, sizeof(adminFuncName), "%s_createDbAdmin", DbConnConfig::DbClientTypeToString(mDbConfig.getDbClient()));

    switch(mDbConfig.getDbClient())
    {
        case DbConnConfig::MYSQL:
            mCreateDbAdminFunc = MYSQL_createDbAdmin;
            break;
    }

    BlazeRpcError rc = (mMaster == nullptr ? DB_ERR_OK : mMaster->initialize(mCreateDbAdminFunc));
    if (rc == DB_ERR_OK)
    {
        SlaveList::iterator itr = mSlaves.begin();
        SlaveList::iterator end = mSlaves.end();
        for(; (itr != end) && (rc == DB_ERR_OK); ++itr)
        {
            rc = (*itr).initialize(mCreateDbAdminFunc);
        }
    }

    return rc;
}

void DbConnPool::lockMutex()
{
    if (getUseAsyncDbConns())
        return;
    mMutex.Lock();
}

void DbConnPool::unlockMutex()
{
    if (getUseAsyncDbConns())
        return;
    mMutex.Unlock();
}

bool DbConnPool::getUseAsyncDbConns() const
{
    return mDbConfig.getDbConnConfig().getAsyncDbConns();
}

bool DbConnPool::isWriteReplicationSynchronous() const
{
    // Galera Cluster uses a synchronous replication system
    if (mDbTechType == DBTECH_GALERA_CLUSTER)
        return true;

    // Although Amazon Aurora Replicas likely have less lag than MySQL master/slave
    // (Amazon claims milliseconds vs. seconds), it is still asynchronous.
    return false;
}

bool DbConnPool::getSchemaUpdateBlocksAllTxn() const
{
    // Avoid TRUNCATE TABLE if "Online Schema Upgrades" setting is "Total Order Isolation".
    // Specific Galera setting can be queried via: SHOW VARIABLES LIKE 'wsrep_OSU_method';
    // Value will be: 'TOI' (default for Galera)
    if (mDbTechType == DBTECH_GALERA_CLUSTER)
        return true;

    // Technically Aurora does not have this behavior,
    // but there is a MySQL TRUNCATE bug that causes similar problems:
    //   Fixed in Aurora 2.07.0 - Bug #23070734: CONCURRENT TRUNCATE TABLES CAUSE STALL
    // To determine current version, use: SELECT AURORA_VERSION();
    // As of 2020-05-13 that returns: aurora_version 2.02.5
    // Thus, we will return true for now to avoid the use of TRUNCATE.
    if (mDbTechType == DBTECH_AMAZON_AURORA)
        return true;

    return false;
}

void DbConnPool::reconfigure(const DbConnConfig &config)
{
    BLAZE_TRACE_LOG(Log::DB, "[DbConnPool].reconfigure: Applying DB config ->\n" << config);

    lockMutex();
    if (mMaster != nullptr)
    {
        mMaster->updateMaxConnCount(config.getMaxConnCount());
        mMaster->updateTimeouts(config);
    }
    const auto& configuredReadSlaves = config.getSlaves();
    auto& existingReadSlaves = mDbConfig.getSlaves();

    // find new db slaves
    for (auto& configuredSlave : configuredReadSlaves)
    {
        bool foundSlave = false;
        for (auto& existingSlave : existingReadSlaves)
        {
            if ((blaze_stricmp(existingSlave->getHost(), configuredSlave->getHostname()) == 0) &&
                (existingSlave->getPort() == configuredSlave->getPort()))
            {
                foundSlave = true;
                // Just in case, update owner copy of the db slave config also
                existingSlave->setMaxConnCount(configuredSlave->getMaxConnCount());
                existingSlave->setTimeouts(*configuredSlave);
                break;
            }
        }

        // if we actually found a matching config (hostname:port) then this isn't new, so move on to the next one.  
        if (foundSlave)
            continue;

        // Create a new DbConfig for this new slave, and add the config.
        DbConfig& newDbConfig = mDbConfig.addSlaveConfig(getName(), *configuredSlave);

        // create a new DbInstancePool and add it to the list of slaves
        DbInstancePool* slavePool = BLAZE_NEW DbInstancePool(*this, newDbConfig, DB_SLAVE, DbConnBase::READ_ONLY, mSlavePoolCounter++);
        mSlaves.push_back(*slavePool);

        // initialize and start the DbInstancePool.
        BlazeRpcError err = slavePool->initialize(mCreateDbAdminFunc);

        bool connectOK = true;
        HostNameAddressList::iterator iter = mPrepareReconfigureFailedSlaveList.begin();
        HostNameAddressList::iterator end = mPrepareReconfigureFailedSlaveList.end();
        for(;iter!=end;++iter)
        {
            if ((blaze_stricmp((*iter)->getHostName(), slavePool->getDbConfig().getHost()) == 0) &&
                ((*iter)->getPort() == slavePool->getDbConfig().getPort()))
            {
                connectOK = false;
                break;
            }
        }
        if (err == ERR_OK && connectOK)
            err = slavePool->reconfigureStart();
    }

    mPrepareReconfigureFailedSlaveList.clear();

    // for each existing slave, see if it exists in the new configuration
    SlaveList::iterator it = mSlaves.begin();
    while (it != mSlaves.end())
    {
        DbConfig& dbConfig = it->mDbConfig;
        bool foundSlave = false;
        for (auto& configuredSlave : configuredReadSlaves)
        {
            if ((blaze_stricmp(dbConfig.getHost(), configuredSlave->getHostname()) == 0) &&
                (dbConfig.getPort() == configuredSlave->getPort()))
            {
                foundSlave = true;
                // if it exists in the new configuration, update maxconncount, squelching state
                it->updateMaxConnCount(configuredSlave->getMaxConnCount());
                it->updateTimeouts(*configuredSlave);
                break;
            }
        }
        // if it exists in the new configuration, continue
        if (foundSlave)
        {
            ++it;
            continue;
        }

        DbInstancePool& pool = *it;

        // take it out of the list of available slaves - important to erase first because this is an
        // intrusive list, and the subsequent markForDrain call may immediately delete the object should
        // it contain no active DB connections
        it = mSlaves.erase(it);

        // remove the slave's config from our list -- also important to erase first because the subsequent
        // markForDrain call may immediately delete the object and invalidate dbConfig
        DbConfig::SlaveList::iterator existingSlaveIt = existingReadSlaves.begin();
        DbConfig::SlaveList::iterator existingSlaveItend = existingReadSlaves.end();
        for (; existingSlaveIt != existingSlaveItend; ++existingSlaveIt)
        {
            if ((blaze_stricmp((*existingSlaveIt)->getHost(), dbConfig.getHost()) == 0) && ((*existingSlaveIt)->getPort() == dbConfig.getPort()))
            {
                BLAZE_INFO_LOG(Log::DB, "[DbConnPool].reconfigure: Removed DB slave config " << dbConfig.getName()
                    << " (Host: " << dbConfig.getHost() << ":" << dbConfig.getPort() << ", Schema: " << dbConfig.getDatabase() << ", UserName: " << dbConfig.getUser() << ")");
                delete *existingSlaveIt;
                existingReadSlaves.erase(existingSlaveIt);
                break;
            }
        }

        // tell the DbInstancePool (e.g. the slave) to drain itself, and then delete itself when it is safe
        pool.markForDrain();
    }

    unlockMutex();

    scheduleTrimTimerInternal(0); // schedule immediate trim
    scheduleQueryStatusTimer();
}

void DbConnPool::prepareForReconfigure(const DbConnConfig &config)
{
    const DbConnConfig::DbConnConfigList& configuredReadSlaves = config.getSlaves();
    DbConfig::SlaveList& existingReadSlaves = mDbConfig.getSlaves();

    int index = 0;
    // find new db slaves  
    for (DbConnConfig::DbConnConfigList::const_iterator configuredSlaveIt = configuredReadSlaves.begin(), configuredSlaveItend = configuredReadSlaves.end();
        configuredSlaveIt != configuredSlaveItend;
        ++configuredSlaveIt)
    {
        DbConfig::SlaveList::const_iterator existingSlaveIt = existingReadSlaves.begin();
        DbConfig::SlaveList::const_iterator existingSlaveItend = existingReadSlaves.end();
        for ( ; existingSlaveIt != existingSlaveItend; ++existingSlaveIt)
        {
            if ((blaze_stricmp((*existingSlaveIt)->getHost(), (*configuredSlaveIt)->getHostname()) == 0) &&
                ((*existingSlaveIt)->getPort() == (*configuredSlaveIt)->getPort()))
            {
                break;
            }
        }

        // if we actually found a matching config (hostname:port) then this isn't new, so move on to the next one.  
        if (existingSlaveIt != existingSlaveItend)
            continue;

        // Create a new DbConfig for this new slave, and add the config.
        char8_t slaveName[128];
        blaze_snzprintf(slaveName, sizeof(slaveName), "%s-slv%d", getName(), index++);
        DbConfig* slaveConfig = BLAZE_NEW DbConfig(slaveName, **configuredSlaveIt, mDbConfig);

        // create a new DbInstancePool and add it to the list of slaves
        DbInstancePool* slavePool = BLAZE_NEW DbInstancePool(*this, *slaveConfig, DB_SLAVE, DbConnBase::READ_ONLY, mSlavePoolCounter++);

        // initialize and start the DbInstancePool.
        BlazeRpcError err = slavePool->initialize(mCreateDbAdminFunc);

        if (err != ERR_OK)
        {
            HostNameAddress *failedSlave = BLAZE_NEW HostNameAddress();
            failedSlave->setPort((*configuredSlaveIt)->getPort());
            failedSlave->setHostName((*configuredSlaveIt)->getHostname());
            mPrepareReconfigureFailedSlaveList.push_back(failedSlave);

            BLAZE_ERR_LOG(Log::DB, "[DbConnPool].prepareForReconfigure: failed to start the DbInstancePool with error " << ErrorHelp::getErrorName(err));
        }

        delete slaveConfig;
        delete slavePool;
    }
}

BlazeRpcError DbConnPool::start()
{
    mDbTrimTimerId = INVALID_TIMER_ID;
    mQueryStatusTimerId = INVALID_TIMER_ID;

    BlazeRpcError rc = (mMaster == nullptr ? DB_ERR_OK : mMaster->start());
    if (rc == DB_ERR_OK)
    {
        SlaveList::iterator itr = mSlaves.begin();
        SlaveList::iterator end = mSlaves.end();
        for(; (itr != end) && (rc == DB_ERR_OK); ++itr)
        {
            rc = (*itr).start();
        }
    }

    if (rc == DB_ERR_OK && getMaxConnCountForMode(DbConn::READ_WRITE) != 0)
    {
        mDbTechType = DBTECH_UNKNOWN;

        DbConn* conn = nullptr;
        BlazeRpcError err = getConn(DbConn::READ_WRITE, DB_LOGGER_CATEGORY, conn);
        if (err == ERR_OK && conn != nullptr)
        {
            DbConnPtr connPtr;
            connPtr.attach(conn);

            QueryPtr query = DB_NEW_QUERY_PTR(connPtr);
            if (query != nullptr)
            {
                // Retrieve DB tech specific variables:
                // - wsrep_cluster_address will only be non-empty for Galera
                // - aurora_version will only be non-empty for Aurora
                query->append("SHOW VARIABLES WHERE Variable_name IN ('wsrep_cluster_address', 'aurora_version');");

                DbResultPtr dbResult;
                if (connPtr->executeQuery(query, dbResult) == ERR_OK)
                {
                    mDbTechType = DBTECH_MYSQL_MASTER_SLAVE; // assume generic MySQL

                    for (const DbRow* row : *dbResult)
                    {
                        const char8_t* name = row->getString("Variable_name");
                        const char8_t* val = row->getString("Value");
                        if (name != nullptr && val != nullptr)
                        {
                            if (blaze_stricmp(name, "wsrep_cluster_address") == 0 && strlen(val) > 0)
                            {
                                mDbTechType = DBTECH_GALERA_CLUSTER;
                            }
                            else if (blaze_stricmp(name, "aurora_version") == 0 && strlen(val) > 0)
                            {
                                mDbTechType = DBTECH_AMAZON_AURORA;
                            }
                        }
                    }
                }
            }
        }

        if (mDbTechType != DBTECH_UNKNOWN)
        {
            BLAZE_INFO_LOG(Log::DB, "[DbConnPool].start: DB tech type for DB config (" << mDbConfig.getName() << ") is (" << dbTechToString(mDbTechType) << ")");
        }
        else
        {
            BLAZE_ERR_LOG(Log::DB, "[DbConnPool].start: Failed to determine DB tech type for DB config (" << mDbConfig.getName() << ")");
            rc = ERR_SYSTEM;
        }
    }

    scheduleTrimTimer();
    scheduleQueryStatusTimer();
    return rc;
}

BlazeRpcError DbConnPool::registerDbThread()
{
    EA_ASSERT(!getUseAsyncDbConns());
    BlazeRpcError error = DB_ERR_OK;
    if (mMaster != nullptr)
        error = mMaster->getDbAdmin().registerDbThread();

    if (error == DB_ERR_OK)
    {
        SlaveList::iterator itr = mSlaves.begin();
        SlaveList::iterator end = mSlaves.end();
        for(; (itr != end) && (error == DB_ERR_OK); ++itr)
            error = (*itr).getDbAdmin().registerDbThread();
    }
    return error;
}

BlazeRpcError DbConnPool::unregisterDbThread()
{
    EA_ASSERT(!getUseAsyncDbConns());
    BlazeRpcError error = DB_ERR_OK;
    if (mMaster != nullptr)
        error = mMaster->getDbAdmin().unregisterDbThread();

    if (error == DB_ERR_OK)
    {
        SlaveList::iterator itr = mSlaves.begin();
        SlaveList::iterator end = mSlaves.end();
        for(; (itr != end) && (error == DB_ERR_OK); ++itr)
            error = (*itr).getDbAdmin().unregisterDbThread();
    }
    return error;
}

/*** Protected Methods ***************************************************************************/

/*** Private Methods *****************************************************************************/

// static
const char8_t* DbConnPool::dbTechToString(DbTechType dbTechType)
{
    switch (dbTechType)
    {
        case DBTECH_UNKNOWN:
            return "Unknown";

        case DBTECH_MYSQL_MASTER_SLAVE:
            return "MySQL Master/Slave";

        case DBTECH_GALERA_CLUSTER:
            return "Galera Cluster";

        case DBTECH_AMAZON_AURORA:
            return "Amazon Aurora";
    }
    return "invalid";
}

uint32_t DbConnPool::getMaxConnCountForMode(DbConn::Mode mode) const
{
    switch (mode)
    {
    case DbConn::READ_WRITE:
        return mDbConfig.getMaxConnCount();

    case DbConn::READ_ONLY:
        // (As done in activateClientConnection) if no slave connections configured, we use the master pool instead.
        if (mSlaves.empty())
        {
            return mDbConfig.getMaxConnCount();
        }
        return (mDbConfig.getSummaryMaxConnCount() >= mDbConfig.getMaxConnCount() ? (mDbConfig.getSummaryMaxConnCount() - mDbConfig.getMaxConnCount()) : 0);

    default:
        BLAZE_ERR_LOG(Log::DB, "[DbConnPool].getMaxConnCountForMode: Internal error: unhandled mode(" << DbConn::modeToString(mode) << " for db(" << mDbConfig.getName() << "). Returning 0.");
        return 0;
    };
}

DbConn *DbConnPool::activateClientConnection(DbConn::Mode mode, bool& squelching)
{
    DbConn *result = nullptr;
    
    switch (mode)
    {
        case DbConn::READ_WRITE:
            if (mMaster != nullptr)
            {
                result = mMaster->activateConnection(squelching);
            }
            break;

        case DbConn::READ_ONLY:
            if (!mSlaves.empty())
            {
                // Locate the slave with the most available connections.  This is a guesstimate
                // for which slave is the least loaded.  The number of slaves should always
                // be relatively small so doing a linear search each time it likely less costly
                // than maintaining an ordered list.
                uint64_t freeConns = 0;
                uint32_t unsquelchedCount = 0;                
                SlaveList::iterator itr = mSlaves.begin();
                SlaveList::iterator end = mSlaves.end();

                typedef eastl::vector<DbInstancePool*> SlaveVector;
                SlaveVector bestSlaveVector;
                bestSlaveVector.reserve(mSlaves.size());
                for(; itr != end; ++itr)
                {
                    DbInstancePool& instance = (*itr);
                    
                    if (instance.isSquelching() || instance.isDraining())
                        continue;

                    ++unsquelchedCount;
                    uint64_t count = instance.getInactivePoolCount();
                    if (count > freeConns)
                    {
                        freeConns = count;
                        bestSlaveVector.clear();
                    }
                    
                    if (count == freeConns)
                    {
                        bestSlaveVector.push_back(&instance);
                    }
                }

                if (!bestSlaveVector.empty())
                {
                    SlaveVector::iterator best = bestSlaveVector.begin();
                    eastl::advance(best, Random::getRandomNumber((int32_t)bestSlaveVector.size()));

                    DbInstancePool* bestInstance = *best;
                    result = bestInstance->activateConnection(squelching);
                }
                else
                    squelching = (unsquelchedCount == 0);
            }
            else
            {
                // No slave connections configured so use the master pool instead
                if (mMaster != nullptr)
                {
                    result = mMaster->activateConnection(squelching);
                }
            }
            break;
    }

    BLAZE_TRACE_LOG(Log::DB, "[DbConnPool].activateConnection: db(" << mDbConfig.getName() << ") activated(" << (result != nullptr ? result->getName() : "<no conn available>") <<
        "); mode(" << DbConn::modeToString(mode) <<
        "); squelching(" << (squelching ? "true" : "false") <<
        "); masterConnectionWaiters count(" << mMasterConnectionWaiters.size() <<
        "); slaveConnectionWaiters count(" << mSlaveConnectionWaiters.size() <<
        "); maxConnCount(" << mDbConfig.getMaxConnCount() <<
        "); summaryMaxConnCount(" << mDbConfig.getSummaryMaxConnCount() <<
        "); slave count(" << mSlaves.size() << ").");

    return result;
}

void DbConnPool::deactivateClientConection(DbConn &conn, bool replace) const
{
    EA_ASSERT_MSG(conn.isActive(), "Attempting to release a DB connection that is not active.");
    conn.getDbInstancePool().deactivateConnection(conn, replace);
}

/*************************************************************************************************/
/*!
    \brief DbConnPool

    Default constructor

    \notes
        mMutex needs to be a pointer because it won't compile otherwise.
*/
/*************************************************************************************************/
DbConnPool::DbConnPool(const char8_t* name, const DbConnConfig &config, uint32_t id) : 
    mDbConfig(name, config),
    mId(id),
    mDbTechType(DBTECH_UNKNOWN),
    mMaster(nullptr),
    mStatementInfo(BlazeStlAllocator("DbConnPool::mStatementInfo")),
    mNextStatementId(0),
    mSchemaMapInitialized(false),
    mComponentSchemaMap(BlazeStlAllocator("DbConnPool::mComponentSchemaMap")),
    mCreateDbAdminFunc(nullptr),
    mDbTrimTimerId(INVALID_TIMER_ID),
    mQueryStatusTimerId(INVALID_TIMER_ID),
    mQuerySamplePeriodStart(0),
    mMetricsCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::db_pool, name)),
    mQueuedFibersMaster(mMetricsCollection, "database.dbpool.queuedFibersMaster", [this]() { return mMasterConnectionWaiters.size(); }),
    mQueuedFibersSlaves(mMetricsCollection, "database.dbpool.queuedFibersSlaves", [this]() { return mSlaveConnectionWaiters.size(); })
{
    BLAZE_TRACE_LOG(Log::DB, "[DbConnPool].ctor: Initialize DB config ->\n" << config);

    mSlavePoolCounter = 0;
    if (mDbConfig.getMaxConnCount() != 0)
    {
        mMaster = BLAZE_NEW DbInstancePool(*this, mDbConfig, DB_MASTER, DbConn::READ_WRITE, mSlavePoolCounter++);
    }
    const DbConfig::SlaveList& slaves = mDbConfig.getSlaves();
    DbConfig::SlaveList::const_iterator itr = slaves.begin();
    DbConfig::SlaveList::const_iterator end = slaves.end();    
    for(; itr != end; ++itr)
    {
        if ((*itr)->getMaxConnCount() == 0)
        {
            continue;
        }
        DbInstancePool* slavePool = BLAZE_NEW DbInstancePool(*this, **itr, DB_SLAVE, DbConnBase::READ_ONLY, mSlavePoolCounter++);        
        mSlaves.push_back(*slavePool);
    }

}


/*************************************************************************************************/
/*!
    \brief ~DbConnPool

    Destructor.

    \notes
        TODO: Should also emtpy and destroy all items in the queue.
*/
/*************************************************************************************************/
DbConnPool::~DbConnPool()
{
    if (mDbTrimTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mDbTrimTimerId);
        mDbTrimTimerId = INVALID_TIMER_ID;
    }
    if (mQueryStatusTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mQueryStatusTimerId);
        mQueryStatusTimerId = INVALID_TIMER_ID;
    }

    StatementInfoById::iterator itr = mStatementInfo.begin();
    StatementInfoById::iterator end = mStatementInfo.end();
    for(; itr != end; ++itr)
        delete itr->second;

    SchemaMap::iterator schemaItr = mComponentSchemaMap.begin();
    SchemaMap::iterator schemaEnd = mComponentSchemaMap.end();

    for(; schemaItr != schemaEnd; ++schemaItr)
    {
        delete [] schemaItr->first;
    }

    delete mMaster;

    while (!mSlaves.empty())
    {
        DbInstancePool& instance = mSlaves.front();
        mSlaves.pop_front();

        delete &instance;
    }

    mPrepareReconfigureFailedSlaveList.clear();
}

PreparedStatementId DbConnPool::registerPreparedStatement(
        const char8_t* name, const char8_t* query)
{
    if (mNextStatementId < 0 || name == nullptr || query == nullptr)
    {
        BLAZE_FAIL_LOG(Log::DB, "[DbConnPool::registerPreparedStatement]: Failed to register prepared statment with name = " << 
            (name == nullptr ? "(null)" : name) << " and query = " << (query == nullptr ? "(null)" : query));
        return INVALID_PREPARED_STATEMENT_ID;
    }

    lockMutex();

    PreparedStatementId id = mNextStatementId++;
    PreparedStatementInfo* info = BLAZE_NEW PreparedStatementInfo(id, name, query);
    mStatementInfo[id] = info;

    unlockMutex();

    return id;
}

const PreparedStatementInfo* DbConnPool::getStatementInfo(PreparedStatementId id) const
{
    EA::Thread::AutoMutex am(mMutex);
    StatementInfoById::const_iterator find = mStatementInfo.find(id);
    if (find == mStatementInfo.end())
        return nullptr;
    return find->second;
}

void DbConnPool::addComponentSchemaRecord(const char8_t* name, const uint32_t version)
{
     char8_t* componentName  = BLAZE_NEW_ARRAY(char8_t, strlen(name)+1);
     blaze_strnzcpy(componentName, name, strlen(name)+1);
     mComponentSchemaMap.insert(eastl::make_pair(componentName, version));
}

uint32_t DbConnPool::getComponentSchemaVersion(const char8_t* name) const
{
    SchemaMap::const_iterator find = mComponentSchemaMap.find(name);

    if (find == mComponentSchemaMap.end())
    {
        return 0;
    }

    return find->second;
}

void DbConnPool::updateComponentSchemaVersion(const char8_t* name, uint32_t version)
{
    SchemaMap::iterator find = mComponentSchemaMap.find(name);

    if (find != mComponentSchemaMap.end())
    {
        find->second = version;
    }
    else
    {
        addComponentSchemaRecord(name, version);
    }
}

void DbConnPool::trimOpenConnections()
{
    // Close any DB connections which are over the configured trim limit.  This runs periodically
    // to ensure that we don't unnecessarily keep DB connections open that we don't need.

    BLAZE_TRACE_LOG(Log::DB, "[DbConnPool].trimOpenConnections: db(" << mDbConfig.getName() << ")");

    mDbTrimTimerId = INVALID_TIMER_ID;

    // Trim the slave connections
    SlaveList::iterator itr = mSlaves.begin();
    SlaveList::iterator end = mSlaves.end();
    for(; itr != end; ++itr)
        trimInstancePool(*itr);

    // Trim the master
    if (mMaster != nullptr)
    {
        trimInstancePool(*mMaster);
    }
    scheduleTrimTimer();
}

void DbConnPool::scheduleTrimTimer()
{
    if (mDbTrimTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mDbTrimTimerId);
        mDbTrimTimerId = INVALID_TIMER_ID;
    }

    TimeValue trimPeriod = gDbScheduler->getDbConnectionTrimPeriod();
    if (trimPeriod > 0)
    {
        scheduleTrimTimerInternal(trimPeriod);
    }
}
void DbConnPool::scheduleTrimTimerInternal(const TimeValue& trimPeriod)
{
    if (mDbTrimTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mDbTrimTimerId);
        mDbTrimTimerId = INVALID_TIMER_ID;
    }

    mDbTrimTimerId = gSelector->scheduleFiberTimerCall(
        TimeValue::getTimeOfDay() + trimPeriod,
        this, &DbConnPool::trimOpenConnections,
        "DbConnPool::trimOpenConnections"
        );
}

void DbConnPool::scheduleQueryStatusTimer()
{
    if (mQueryStatusTimerId != INVALID_TIMER_ID)
    {
        gSelector->cancelTimer(mQueryStatusTimerId);
        mQueryStatusTimerId = INVALID_TIMER_ID;
    }

    TimeValue querySamplePeriod = gDbScheduler->getQuerySamplePeriod();
    if (querySamplePeriod > 0)
    {
        mQuerySamplePeriodStart = TimeValue::getTimeOfDay();
        mQueryStatusTimerId = gSelector->scheduleFiberTimerCall(
                mQuerySamplePeriodStart + querySamplePeriod,
                this, &DbConnPool::checkQueryStatus,
                "DbConnPool::checkQueryStatus"
            );
    }
}

void DbConnPool::checkQueryStatus()
{
    mQueryStatusTimerId = INVALID_TIMER_ID;

    // Process query time adjustments on the slave instances
    SlaveList::iterator itr = mSlaves.begin();
    SlaveList::iterator end = mSlaves.end();
    for(; itr != end; ++itr)
        (*itr).checkQueryStatus();

    // Process query time adjustments on the master instance
    if (mMaster != nullptr)
    {
        mMaster->checkQueryStatus();
    }
    scheduleQueryStatusTimer();
}

void DbConnPool::trimInstancePool(DbInstancePool& pool)
{
    if (pool.isDraining())
        return;

    lockMutex();
    // The getTrim list moves the inactive connection to the active list. It does not manipulate the peak connection count. However, the mutex is unlocked here before the
    // connections are fully trimmed. During that time period, if a connection was acquired, we'd see peak connection metric manipulation and put the metric close to max connections (which is wrong). So we set to ignore 
    // any manipulation here.
    pool.allowPeakPoolCountManipulation(false);
    eastl::vector<DbConn*> trimList;
    pool.getTrimList(trimList);
    unlockMutex();

    if (!trimList.empty())
    {
        BLAZE_TRACE_LOG(Log::DB, "[DbConnPool].trimInstancePool: db(" << mDbConfig.getName() << "), trimming " << trimList.size() << " connections.");
    }


    // TODO: 
    // 1. We do not need to cycle the connections to be active in order to trim the pool, the pool can be trimmed without first making the connection go active which obviates the need for allowPeakPoolCountManipulation option.
    // 2. We do not need to trim the pool all at once, that is because the pool may be large. We can just keep track of conn with last usage time added to the inactive list, and schedule a timer with that.
    // 3. There is no need to keep disconnected connections. They have no value because only value of pooling comes from having connections that are already connected.
    eastl::vector<DbConn*>::iterator itr = trimList.begin();
    eastl::vector<DbConn*>::iterator end = trimList.end();
    for(; itr != end; ++itr)
    {
        DbConn* conn = *itr;
        conn->disconnect(DB_ERR_OK);
        release(*conn);
    }

    lockMutex();
    pool.allowPeakPoolCountManipulation(true);
    unlockMutex();
}


} // Blaze

