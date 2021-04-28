/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconfig.h"
#include "framework/database/query.h"
#include "framework/database/dbadmin.h"
#include "framework/connection/selector.h"
#include "framework/system/fiber.h"
#include <mariadb/mysqld_error.h>

#include "EASTL/string.h"

namespace Blaze
{

FiberLocalManagedPtr<DbConnMetrics> gDbMetrics;

namespace Metrics
{
namespace Tag
{
TagInfo<DbConnMetrics::DbOp>* db_op = BLAZE_NEW TagInfo<DbConnMetrics::DbOp>("db_op", [](const DbConnMetrics::DbOp& value, Metrics::TagValue&) {
        switch (value)
        {
            case DbConnMetrics::DB_OP_TRANSACTION:
                return "transaction";
            case DbConnMetrics::DB_OP_QUERY:
                return "query";
            case DbConnMetrics::DB_OP_MULTI_QUERY:
                return "multi_query";
            case DbConnMetrics::DB_OP_PREPARED_STATEMENT:
                return "prepared_statement";
            case DbConnMetrics::DB_OP_MAX:
                break;
        }
        return "unknown";
    });

TagInfo<DbConnMetrics::DbThreadStage>* db_thread_stage = BLAZE_NEW TagInfo<DbConnMetrics::DbThreadStage>("db_thread_stage", [](const DbConnMetrics::DbThreadStage& value, Metrics::TagValue&) {
        switch (value)
        {
            case DbConnMetrics::DB_STAGE_SETUP:
                return "setup";
            case DbConnMetrics::DB_STAGE_EXECUTE:
                return "execute";
            case DbConnMetrics::DB_STAGE_CLEANUP:
                return "cleanup";
            case DbConnMetrics::DB_STAGE_MAX:
                break;
        }
        return "unknown";
    });

const char8_t* DB_GET_CONN_CONTEXT = "DbConnPool::getConn";
}
}

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define ADD_QUERY_TIME(startTime) \
    { \
        TimeValue now = TimeValue::getTimeOfDay(); \
        TimeValue queryTime = now - startTime; \
        gDbMetrics->addQueryTime(queryTime, mDbConnInterface->getQuerySetupTimeOnThread(), mDbConnInterface->getQueryExecutionTimeOnThread(), mDbConnInterface->getQueryCleanupTimeOnThread()); \
        mOwner.updateQueryAverageMetrics(now, queryTime); \
    }

#define INCR_QUERY_COUNT() \
    { \
        gDbMetrics->incrDbOp(DbConnMetrics::DB_OP_QUERY); \
    }

#define INCR_PREPARED_STMT_COUNT() \
    { \
        gDbMetrics->incrDbOp(DbConnMetrics::DB_OP_PREPARED_STATEMENT); \
    }

#define INCR_MULTI_QUERY_COUNT() \
    { \
        gDbMetrics->incrDbOp(DbConnMetrics::DB_OP_MULTI_QUERY); \
    }

#define INCR_TXN_COUNT() \
    { \
        gDbMetrics->incrDbOp(DbConnMetrics::DB_OP_TRANSACTION); \
    }

#define INCR_NAMED_QUERY_COUNTS(name, startTime) \
    { \
        if ((gDbScheduler != nullptr) && (name != nullptr)) \
        { \
            gDbScheduler->tickQuery(name, TimeValue::getTimeOfDay() - startTime); \
        } \
        gDbMetrics->incrDbQueryCount(name); \
    }

#define INCR_NAMED_QUERY_SUCCESS_COUNT(name) \
    { \
        if ((gDbScheduler != nullptr) && (name != nullptr)) \
        { \
            gDbScheduler->tickQuerySuccess(name); \
        } \
    }

#define INCR_NAMED_SLOW_QUERY_COUNT(name) \
    { \
        if ((gDbScheduler != nullptr) && (name != nullptr)) \
        { \
            gDbScheduler->tickSlowQuery(name); \
        } \
    }

#define INCR_NAMED_QUERY_FAIL_COUNT(queryName, err) \
    { \
        if ((gDbScheduler != nullptr) && (queryName != nullptr)) \
        { \
            gDbScheduler->tickQueryFailure(queryName, err); \
        } \
    }

#define VERIFY_WORKER_FIBER_OR_DB_THREAD() \
    if (!Fiber::isCurrentlyBlockable() && !gIsDbThread) \
    { \
        EA_ASSERT_MSG(false, "Attempt to call db function outside of a fiber or DB thread."); \
        return DB_ERR_SYSTEM; \
    }

struct KnownResumableErrorCodeMapping
{
    int32_t         mysqlErr;
    BlazeRpcError   blazeErr;
};

static const KnownResumableErrorCodeMapping sKnownResumableErrorCodeMappings[] = {
    { ER_DUP_ENTRY,                     DB_ERR_DUP_ENTRY },
    { ER_NO_SUCH_TABLE,                 DB_ERR_NO_SUCH_TABLE },
    { ER_LOCK_DEADLOCK,                 DB_ERR_LOCK_DEADLOCK },
    { ER_DROP_PARTITION_NON_EXISTENT,   DB_ERR_DROP_PARTITION_NON_EXISTENT },
    { ER_SAME_NAME_PARTITION,           DB_ERR_SAME_NAME_PARTITION },
    { ER_NO_SUCH_THREAD,                DB_ERR_NO_SUCH_THREAD },
    { ER_SIGNAL_EXCEPTION,              ERR_DB_USER_DEFINED_EXCEPTION }
};

bool DbConnBase::isResumableErrorCode(BlazeRpcError error)
{
    for (auto& mapping : sKnownResumableErrorCodeMappings)
    {
        if (mapping.blazeErr == error)
            return true;
    }
    return false;
}

/*************************************************************************************************/
/*!
    \brief Map a MySql error code to the appropriate BlazeRpcError
*/
/*************************************************************************************************/
BlazeRpcError DbConnBase::convertErrorCode(int error)
{
    for (auto& mapping : sKnownResumableErrorCodeMappings)
    {
        if (mapping.mysqlErr == error)
            return mapping.blazeErr;
    }

    // Default unknown errors to trigger a disconnect to ensure that our state gets reset
    return DB_ERR_DISCONNECTED;
}

/*** Public Methods ******************************************************************************/

DbConn::DbConn(const DbConfig& config, DbAdmin& admin, DbInstancePool& owner, uint32_t num, Mode mode) : 
    DbConnBase(config, admin, owner, num, mode)
{
}

void DbConn::assign() 
{     
    DbConnBase::assign(); 
}

///////////////////////////////////////////////////////////////////////////////////////////////////

QueryPtr DbConnBase::newQueryPtr(const char8_t* fileAndLine)
{
    EA_ASSERT_MSG(mValid, "Using invalid connection to obtain a new query.");
    QueryPtr ptr;
    ptr.attach( newQueryInternal(fileAndLine) );
    return ptr;
}

Query* DbConnBase::newQueryInternal(const char8_t* fileAndLine) 
{ 
    Query* query = mDbConnInterface->newQuery(fileAndLine); 
    //  Ownership of resource claimed by fiber since it's allocated on the current fiber.  There are cases
    //  in the framework where a query can exist on the stack and not the heap.  In those rare cases 
    //  FrameworkResource::release() is never called.   An assign() should always be coupled with an eventual release().
    query->assign();
    return query;
}


BlazeRpcError DbConnBase::executeQuery(const QueryPtr& queryPtr, TimeValue timeout)
{
    if (EA_UNLIKELY(!mValid))
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConnBase].executeQuery: invalid connection(" << mName << ")");
        return DB_ERR_SYSTEM;
    }

    // This is guaranteed to be a valid cast as QueryBase is only ever constructed as an object derived from Query
    const Query* query = (const Query *)(&(*queryPtr)); 
    BlazeRpcError error = executeQueryInternal(query, nullptr, timeout);
    return error;
}


BlazeRpcError DbConnBase::executeQuery(const QueryPtr& queryPtr,  DbResultPtr& resultPtr, TimeValue timeout)
{
    if (EA_UNLIKELY(!mValid))
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConnBase].executeQuery (with result): invalid connection(" << mName << ")");
        return DB_ERR_SYSTEM;
    }

    // This is guaranteed to be a valid cast as QueryBase is only ever constructed as an object derived from Query
    const Query* query = (const Query *)(&(*queryPtr)); 
    DbResult *result = nullptr;
    BlazeRpcError error = executeQueryInternal(query, &result, timeout);
    resultPtr.attach(result);
    return error;
}

  
BlazeRpcError DbConnBase::executeStreamingQuery(const QueryPtr& queryPtr, StreamingDbResultPtr& resultPtr, TimeValue timeout)
{
    if (EA_UNLIKELY(!mValid))
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConnBase].executeStreamingQuery: invalid connection(" << mName << ")");
        return DB_ERR_SYSTEM;
    }

    // This is guaranteed to be a valid cast as QueryBase is only ever constructed as an object derived from Query
    const Query* query = (const Query *)(&(*queryPtr));     
    StreamingDbResult *result = nullptr;
    BlazeRpcError error = executeStreamingQueryInternal(query, &result, timeout);
    resultPtr.attach(result);
    return error;
}


BlazeRpcError DbConnBase::executeMultiQuery(const QueryPtr& queryPtr, TimeValue timeout)
{
    if (EA_UNLIKELY(!mValid))
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConnBase].executeMultiQuery: invalid connection(" << mName << ")");
        return DB_ERR_SYSTEM;
    }

    // This is guaranteed to be a valid cast as QueryBase is only ever constructed as an object derived from Query
    const Query* query = (const Query *)(&(*queryPtr));   
    BlazeRpcError error = executeMultiQueryInternal(query, nullptr, timeout);
    return error;    
}


BlazeRpcError DbConnBase::executeMultiQuery(const QueryPtr& queryPtr, DbResultPtrs& resultPtrs, TimeValue timeout)
{
    if (EA_UNLIKELY(!mValid))
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConnBase].executeMultiQuery (with result): invalid connection(" << mName << ")");
        return DB_ERR_SYSTEM;
    }

    // This is guaranteed to be a valid cast as QueryBase is only ever constructed as an object derived from Query
    const Query* query = (const Query *)(&(*queryPtr));   
    DbResults *results = nullptr;
    BlazeRpcError error = executeMultiQueryInternal(query, &results, timeout);
    if (error == DB_ERR_OK)
    {
        resultPtrs.clear();         // will automatically clean up container and decrement release references to contained DbResultPtr objects.
        if (results->size() > 0)
        {
            resultPtrs.reserve(results->size());
            for (DbResults::const_iterator cit = results->begin(); cit != results->end(); ++cit)
            {
                // attach each result to a DbResultPtr within the resultPtrs container.        
                DbResult *result = *cit;
                resultPtrs.push_back().attach(result);
            }
        }
    }
    // ownership of DbResult objects belongs to the DbResultsPtr container now. 
    // so, clean out the vector before deleting it 
    // (deleting without doing this extra step will wipe out the DbResult references that have been stored off.)
    if (results != nullptr)
    {
        results->clear();
        delete results;
    }
    return error;    
}


BlazeRpcError DbConnBase::executePreparedStatement(PreparedStatement* statement, TimeValue timeout)
{
    if (EA_UNLIKELY(!mValid))
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConnBase].executePreparedStatement: invalid connection(" << mName << ")");
        return DB_ERR_SYSTEM;
    }

    BlazeRpcError error = executePreparedStatementInternal(statement, nullptr, timeout);
    return error;
}


BlazeRpcError DbConnBase::executePreparedStatement(PreparedStatement* statement, DbResultPtr& resultPtr, TimeValue timeout)
{
    if (EA_UNLIKELY(!mValid))
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConnBase].executePreparedStatement (with result): invalid connection(" << mName << ")");
        return DB_ERR_SYSTEM;
    }

    DbResult *result = nullptr;
    BlazeRpcError error = executePreparedStatementInternal(statement, &result, timeout);
    resultPtr.attach(result);
    return error;
}


DbConnBase::DbConnBase(const DbConfig& config, DbAdmin& admin, DbInstancePool& owner, uint32_t num, Mode mode)
    : mOwner(owner),
      mDbConnInterface(nullptr),
      mNum(num),
      mMode(mode),
      mLogCategory(Log::DB),
      mActive(false),
      mValid(true)
{
    eastl::string modeInstance, numberAsString;
    if(mOwner.getMode() == READ_ONLY)
    {
        modeInstance = "slave_";
        modeInstance.append_sprintf("%" PRIu32, mOwner.getId());
    }
    else
        modeInstance = "master";

    if (mNum == DB_ADMIN_CONN_ID) 
        numberAsString = "Admin";
    else
        numberAsString.append_sprintf("%" PRIu32, mNum);

    blaze_snzprintf(mName, sizeof(mName), "%s_%s/%s/%s", mOwner.getDbConnPool().getName(), modeInstance.c_str(), numberAsString.c_str(),  modeToString(mode));

    mMinQueryRuntime = config.getMinQueryRuntime();
    mMaxQueryRuntime = config.getMaxQueryRuntime();
    mDbConnInterface = admin.createDbConn(config, *this);

    BLAZE_TRACE_LOG(Log::DB, "[DbConn].ctor: created " << DbConnConfig::DbClientTypeToString(config.getDbClient()) << " connection: " << mName << "; minQueryRuntime=" << mMinQueryRuntime.getMillis() << "ms" << "; maxQueryRuntime=" << mMaxQueryRuntime.getMillis() << "ms");
}

DbConnBase::~DbConnBase()
{
    delete mDbConnInterface;
}

void DbConnBase::assignInternal()
{
    mDbConnInterface->assign();
}

void DbConnBase::releaseInternal()
{
    if (EA_UNLIKELY(!mValid))
    {
        BLAZE_WARN_LOG(mLogCategory, "[DbConnBase].releaseInternal: releasing invalid connection(" << mName << ") refcount: " << getRefCount());
        if (getRefCount() == 0)
        {
            delete this;
        }
        return;
    }

    if (mDbConnInterface->prepareForRelease())
    {
        BLAZE_TRACE_DB_LOG(mLogCategory, "Release connection: conn=" << mName);
        mLogCategory = Log::DB;

        // IMPORTANT: Calling mDbConnInterface->release() has the potential to delete 'this', so we cannot touch 'this' after this point.
        mDbConnInterface->release();
    }
}

BlazeRpcError DbConnBase::connect()
{
    if (EA_UNLIKELY(!mValid))
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConnBase].connect: invalid connection(" << mName << ")");
        return DB_ERR_SYSTEM;
    }

    //Not on a DB thread, not in a fiber?  This is bad news.
    VERIFY_WORKER_FIBER_OR_DB_THREAD();

    BLAZE_TRACE_DB_LOG(mLogCategory, "-> Establish connection: conn=" << mName);

    BlazeRpcError error = mDbConnInterface->connect();

    BLAZE_TRACE_DB_LOG(mLogCategory, "<- Establish connection: conn=" << mName << " complete: " << getDbErrorString(error));
    return error;
}

void DbConnBase::disconnect(BlazeRpcError reason)
{

    BLAZE_TRACE_DB_LOG(mLogCategory, "-> Drop connection: conn=" << mName << " reason=" << getDbErrorString(reason));

    mDbConnInterface->disconnect(reason);

    BLAZE_TRACE_DB_LOG(mLogCategory, "<- Drop connection: conn=" << mName << " complete");
}

BlazeRpcError DbConnBase::executeQueryInternal(const Query* query, DbResult** result, TimeValue timeout)
{
    //Not on a DB thread, not in a fiber?  This is bad news.
    VERIFY_WORKER_FIBER_OR_DB_THREAD();

    if (result != nullptr)
        *result = nullptr;

    BlazeRpcError error = checkQuery(query);
    if (error != DB_ERR_OK)
        return error;

    BLAZE_TRACE_DB_LOG(mLogCategory, "-> Query execute: conn=" << mName << "\n    " << query->get());

    error = checkPrerequisites(timeout);
    TimeValue st = TimeValue::getTimeOfDay();
    if (error == DB_ERR_OK)
    {
        error = mDbConnInterface->executeQuery(query, result, timeout);
        ADD_QUERY_TIME(st);
        INCR_NAMED_QUERY_COUNTS(query->getName(), st);
        INCR_QUERY_COUNT();
    }

    if (error == DB_ERR_OK)
    {
        INCR_NAMED_QUERY_SUCCESS_COUNT(query->getName());

        if ((TimeValue::getTimeOfDay() - st).getMillis() > mMaxQueryRuntime.getMillis())
        {
            INCR_NAMED_SLOW_QUERY_COUNT(query->getName());
            BLAZE_WARN_LOG(mLogCategory, "[DbConn].executeQuery: Slow query exceeded the default query time ("
                 << mMaxQueryRuntime.getMillis() << "ms) conn=" << mName << " " << query->get());
        }
    }
    else
    {
        INCR_NAMED_QUERY_FAIL_COUNT(query->getName(), error);

        if ((result != nullptr) && (*result != nullptr))
        {
            delete *result;
            *result = nullptr;
        }
    }

    if (error == DB_ERR_TIMEOUT)
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConn].executeQuery: DB_ERR_TIMEOUT Timeout query after " 
            << (TimeValue::getTimeOfDay().getSec() - st.getSec()) << " seconds conn=" << mName << " " << query->get());
    }

    eastl::string logBuf;
    BLAZE_TRACE_DB_LOG(mLogCategory, "<- Query result: conn=" << mName << " code=" << getDbErrorString(error) << " : " 
                       << ((error != DB_ERR_OK) || (result == nullptr || *result == nullptr) ? "" : (*result)->toString(logBuf)));

    mOwner.incrementQueryCount(error);

    //  assign results to the fiber.
    if (result != nullptr && *result != nullptr)
    {
        (*result)->assign();
    }

    return error;
}

BlazeRpcError DbConnBase::executeStreamingQueryInternal(const Query* query, StreamingDbResult** result, TimeValue timeout)
{
    if (result != nullptr)
        *result = nullptr;

    BlazeRpcError error = checkQuery(query);
    if (error != DB_ERR_OK)
        return error;

    BLAZE_TRACE_DB_LOG(mLogCategory, "-> Query execute: conn=" << mName << "\n    " << query->get());

    error = checkPrerequisites(timeout);
    TimeValue st = TimeValue::getTimeOfDay();
    if (error == DB_ERR_OK)
    {
        error  = mDbConnInterface->executeStreamingQuery(query, result, timeout);
        ADD_QUERY_TIME(st);
        INCR_NAMED_QUERY_COUNTS(query->getName(), st);
        INCR_QUERY_COUNT();
    }

    if (error == DB_ERR_OK)
    {
        INCR_NAMED_QUERY_SUCCESS_COUNT(query->getName());

        if ((TimeValue::getTimeOfDay() - st).getMillis() > mMaxQueryRuntime.getMillis())
        {
            INCR_NAMED_SLOW_QUERY_COUNT(query->getName());
            BLAZE_WARN_LOG(mLogCategory, "[DbConn].executeStreamingQuery: Slow query exceeded the default query time ("
                 << mMaxQueryRuntime.getMillis() << "ms) conn=" << mName << " " << query->get());
        }
    }
    else
    {
        INCR_NAMED_QUERY_FAIL_COUNT(query->getName(), error);

        if ((result != nullptr) && (*result != nullptr))
        {
            delete *result;
            *result = nullptr;
        }
    }

    if (error == DB_ERR_TIMEOUT)
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConn].executeStreamingQuery: DB_ERR_TIMEOUT Timeout query after " 
            << (TimeValue::getTimeOfDay().getSec() - st.getSec()) << " seconds conn=" << mName << " " << query->get());
    }

    eastl::string logBuf;
    BLAZE_TRACE_DB_LOG(mLogCategory, "<- Query result: conn=" << mName << " code=" << getDbErrorString(error) << " : " 
                       << ((error != DB_ERR_OK) || (result == nullptr || *result == nullptr) ? "" : (*result)->toString(logBuf)));

    mOwner.incrementQueryCount(error);

   //  assign results to the fiber.
    if (result != nullptr && *result != nullptr)
    {
        (*result)->assign();        
    }
    return error;
}

BlazeRpcError DbConnBase::executeMultiQueryInternal(const Query* query, DbResults** results, TimeValue timeout)
{
    //Not on a DB thread, not in a fiber?  This is bad news.
    VERIFY_WORKER_FIBER_OR_DB_THREAD();

    if (results != nullptr)
        *results = nullptr;
        
    BlazeRpcError error = checkQuery(query);
    if (error != DB_ERR_OK)
        return error;

    BLAZE_TRACE_DB_LOG(mLogCategory, "-> Multi-query execute: conn=" << mName << "\n    " << query->get());

    error = checkPrerequisites(timeout);
    TimeValue st = TimeValue::getTimeOfDay();
    if (error == DB_ERR_OK)
    {
        error = mDbConnInterface->executeMultiQuery(query, results, timeout);
        ADD_QUERY_TIME(st);
        INCR_NAMED_QUERY_COUNTS(query->getName(), st);
        INCR_MULTI_QUERY_COUNT();
    }

    if (error == DB_ERR_OK)
    {
        INCR_NAMED_QUERY_SUCCESS_COUNT(query->getName());

        if ((TimeValue::getTimeOfDay() - st).getMillis() > mMaxQueryRuntime.getMillis())
        {
            INCR_NAMED_SLOW_QUERY_COUNT(query->getName());
            BLAZE_WARN_LOG(mLogCategory, "[DbConn].executeMultiQuery: Slow query exceeded the default query time ("
                 << mMaxQueryRuntime.getMillis() << "ms) conn=" << mName << " " << query->get());
        }
    }
    else
    {
        INCR_NAMED_QUERY_FAIL_COUNT(query->getName(), error);

        if ((results != nullptr) && (*results != nullptr))
        {
            delete *results;
            *results = nullptr;
        }
    }

    if (error == DB_ERR_TIMEOUT)
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConn].executeMultiQuery: DB_ERR_TIMEOUT Timeout query after " 
            << (TimeValue::getTimeOfDay().getSec() - st.getSec()) << " seconds conn=" << mName << " " << query->get());
    }


    if (BLAZE_IS_TRACE_DB_ENABLED(mLogCategory))
    {
        eastl::string logBuf;
        logBuf.append_sprintf("<- Multi-query result: conn=%s code=%s",
                mName, getDbErrorString(error));
        if ((error == DB_ERR_OK) && (results != nullptr) && (*results != nullptr) && ((*results)->size() > 0))
        {
            DbResults::iterator itr = (*results)->begin();
            DbResults::iterator end = (*results)->end();
            for(int32_t idx = 0; itr != end; ++itr, ++idx)
            {
                if (!(*itr)->empty())
                {
                    logBuf.append_sprintf("\n  Result %d: ", idx);
                    (*itr)->toString(logBuf);
                }
            }
        }
        BLAZE_TRACE_DB_LOG(mLogCategory, "" << logBuf.c_str());
    }

    //  assign results to the fiber.
    if (results != nullptr && *results != nullptr)
    {
        DbResults::iterator itr = (*results)->begin();
        DbResults::iterator end = (*results)->end();
        for(; itr != end; ++itr)
        {
            if (*itr != nullptr)
            {
                (*itr)->assign();
            }
        }
    }

    mOwner.incrementMultiQueryCount(error);
    return error;
}

BlazeRpcError DbConnBase::executePreparedStatementInternal(PreparedStatement* statement, DbResult** result, TimeValue timeout)
{
    //Not on a DB thread, not in a fiber?  This is bad news.
    VERIFY_WORKER_FIBER_OR_DB_THREAD();

    if (result != nullptr)
        *result = nullptr;
        
    eastl::string buf;
    BLAZE_TRACE_DB_LOG(mLogCategory, "-> Prepared-statement execute: conn=" << mName << " " << statement->toString(buf));

    BlazeRpcError error = checkPrerequisites(timeout);
    TimeValue st = TimeValue::getTimeOfDay();
    if (error == DB_ERR_OK)
    {
        error = mDbConnInterface->executePreparedStatement(statement, result, timeout);
        ADD_QUERY_TIME(st);
        INCR_NAMED_QUERY_COUNTS(statement->getName(), st);
        INCR_PREPARED_STMT_COUNT();
    }

    if (error == DB_ERR_OK)
    {
        INCR_NAMED_QUERY_SUCCESS_COUNT(statement->getName());

        if ((TimeValue::getTimeOfDay() - st).getMillis() > mMaxQueryRuntime.getMillis())
        {
            INCR_NAMED_SLOW_QUERY_COUNT(statement->getName());
            BLAZE_WARN_LOG(mLogCategory, "[DbConn].executePreparedStatement: Slow query exceeded the default query time ("
                 << mMaxQueryRuntime.getMillis() << "ms) conn=" << mName << " " << statement->getQuery());
        }
    }
    else
    {
        INCR_NAMED_QUERY_FAIL_COUNT(statement->getName(), error);

        if ((result != nullptr) && (*result != nullptr))
        {
            delete *result;
            *result = nullptr;
        }
    }

    if (error == DB_ERR_TIMEOUT)
    {
        eastl::string buf2;
        BLAZE_ERR_LOG(mLogCategory, "[DbConn].executePreparedStatement: DB_ERR_TIMEOUT Timeout query after " 
            << (TimeValue::getTimeOfDay().getSec() - st.getSec()) << " seconds conn=" << mName << " " << statement->toString(buf2));
    }

    eastl::string logBuf;
    BLAZE_TRACE_DB_LOG(mLogCategory, "<- Prepared-statement result: conn=" << mName << " code=" << getDbErrorString(error) << " : " 
                       << ((error != DB_ERR_OK) || (result == nullptr || *result == nullptr) ? "" : (*result)->toString(logBuf)));

    mOwner.incrementPreparedStatementCount(error);

    //  assign results to the fiber.
    if (result != nullptr && *result != nullptr)
    {
        (*result)->assign();
    }
    return error;
}

BlazeRpcError DbConnBase::startTxn(TimeValue timeout, bool commitTxnInProgress)
{
    if (EA_UNLIKELY(!mValid))
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConnBase].startTxn: invalid connection(" << getName() << ")");
        return DB_ERR_SYSTEM;
    }

    //Not on a DB thread, not in a fiber?  This is bad news.
    VERIFY_WORKER_FIBER_OR_DB_THREAD();

    BLAZE_TRACE_DB_LOG(mLogCategory, "-> Start transaction begin: conn=" << mName);

    BlazeRpcError error = DB_ERR_OK;
    if (!commitTxnInProgress && mDbConnInterface->isTxnInProgress())
    {
        error = ERR_DB_PREVIOUS_TRANSACTION_IN_PROGRESS;
    }
    else 
    {
        error = checkPrerequisites(timeout);
        if (error == DB_ERR_OK)
        {
            TimeValue st = TimeValue::getTimeOfDay();
            error = mDbConnInterface->startTxn(timeout);
            ADD_QUERY_TIME(st);
            INCR_TXN_COUNT();
        }
    }    

    BLAZE_TRACE_DB_LOG(mLogCategory, "<- Start transaction end: conn=" << mName << " code=" << getDbErrorString(error));

    return error;
}

BlazeRpcError DbConnBase::commit(TimeValue timeout)
{
    if (EA_UNLIKELY(!mValid))
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConnBase].commit: invalid connection(" << getName() << ")");
        return DB_ERR_SYSTEM;
    }

    //Not on a DB thread, not in a fiber?  This is bad news.
    VERIFY_WORKER_FIBER_OR_DB_THREAD();

    BLAZE_TRACE_DB_LOG(mLogCategory, "-> Commit transaction begin: conn=" << mName);

    BlazeRpcError error = checkPrerequisites(timeout);
    if (error == DB_ERR_OK)
    {
        TimeValue st = TimeValue::getTimeOfDay();
        error = mDbConnInterface->commit(timeout);
        ADD_QUERY_TIME(st);
    }

    BLAZE_TRACE_DB_LOG(mLogCategory, "<- Commit transaction end: conn=" << mName << " code=" << getDbErrorString(error));
    mOwner.incrementCommitCount(error);
    return error;
}

BlazeRpcError DbConnBase::rollback(TimeValue timeout)
{
    if (EA_UNLIKELY(!mValid))
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConnBase].rollback: invalid connection(" << getName() << ")");
        return DB_ERR_SYSTEM;
    }

    //Not on a DB thread, not in a fiber?  This is bad news.
    VERIFY_WORKER_FIBER_OR_DB_THREAD();

    BLAZE_TRACE_DB_LOG(mLogCategory, "-> Rollback transaction begin: conn=" << mName);

    BlazeRpcError error = checkPrerequisites(timeout);
    if (error == DB_ERR_OK)
    {
        TimeValue st = TimeValue::getTimeOfDay();
        error = mDbConnInterface->rollback(timeout);
        ADD_QUERY_TIME(st);
    }

    BLAZE_TRACE_DB_LOG(mLogCategory, "<- Rollback transaction end: conn=" << mName << " code=" << getDbErrorString(error));
    mOwner.incrementRollbackCount(error);
    return error;
}

DbConnPool& DbConnBase::getDbConnPool() const
{
    return mOwner.getDbConnPool();
}

bool DbConnBase::isWriteReplicationSynchronous() const
{
    return mOwner.getDbConnPool().isWriteReplicationSynchronous();
}

bool DbConnBase::getSchemaUpdateBlocksAllTxn() const
{
    return mOwner.getDbConnPool().getSchemaUpdateBlocksAllTxn();
}


BlazeRpcError DbConnBase::checkQuery(const Query* query)
{
    if ((query == nullptr) || (query->get() == nullptr))
    {
        BLAZE_ERR_LOG(mLogCategory, "[DbConn].checkQuery: conn=" << mName << "; Attempting to execute an empty query from context "<< Fiber::getCurrentContext());

        return DB_ERR_SYSTEM;
    }

    return DB_ERR_OK;
}

BlazeRpcError DbConnBase::checkPrerequisites(TimeValue& timeout)
{
    // Ensure that the DbScheduler hasn't been shutdown
    if (EA_UNLIKELY(!gDbScheduler->getAllowRequests()))
    {
        BLAZE_TRACE_DB_LOG(mLogCategory, "[DbConn].checkPrerequisites: conn=" << mName << " code=DB_ERR_SYSTEM (cancelled due to DB scheduler denying requests");    
        return DB_ERR_SYSTEM;
    }

    if (EA_UNLIKELY(!Fiber::isCurrentlyBlockable()))
    {
        return DB_ERR_SYSTEM;
    }

    // Make sure that there is still enough time to allow the query to run
    timeout = mDbConnInterface->calculateQueryTimeout(timeout);
    if ((timeout - TimeValue::getTimeOfDay()) < mMinQueryRuntime)
    {
        return DB_ERR_TIMEOUT;    
    }

    return DB_ERR_OK;
}

// static
const char8_t* DbConnBase::modeToString(Mode mode)
{
    switch (mode)
    {
        case READ_ONLY:
            return "ro";

        case READ_WRITE:
            return "rw";
    }
    return "invalid";
}

} // namespace Blaze

