/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DBCONN_H
#define BLAZE_DBCONN_H

/*** Include files *******************************************************************************/

#include "framework/system/frameworkresource.h"
#include "framework/database/dbresult.h"
#include "framework/database/preparedstatement.h"
#include "framework/database/dberrors.h"
#include "framework/database/dbconninterface.h"
#include "framework/database/query.h"
#include "framework/system/fiberlocal.h"
#include "framework/system/fibermanager.h"
#include "EASTL/fixed_function.h"
#include "EATDF/time.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#define DB_NEW_QUERY_PTR(conn) conn->newQueryPtr(__FILEANDLINE__);

namespace Blaze
{

class DbInstancePool;
class Selector;
class Fiber;
class Query;
class DbConnPool;
class DbConfig;
class DbAdmin;
class DbConn;
 

class DbConnBase: public FrameworkResource
{
public:
    QueryPtr newQueryPtr(const char8_t* fileAndLine);

    BlazeRpcError DEFINE_ASYNC_RET(executeQuery(const QueryPtr& queryPtr, EA::TDF::TimeValue timeout = 0));
    BlazeRpcError DEFINE_ASYNC_RET(executeQuery(const QueryPtr& queryPtr, DbResultPtr& resultPtr, EA::TDF::TimeValue timeout = 0));
    BlazeRpcError DEFINE_ASYNC_RET(executeStreamingQuery(const QueryPtr& queryPtr, StreamingDbResultPtr& resultPtr, EA::TDF::TimeValue timeout = 0));
    BlazeRpcError DEFINE_ASYNC_RET(executeMultiQuery(const QueryPtr& queryPtr, EA::TDF::TimeValue timeout = 0));
    BlazeRpcError DEFINE_ASYNC_RET(executeMultiQuery(const QueryPtr& queryPtr, DbResultPtrs& results, EA::TDF::TimeValue timeout = 0));
    BlazeRpcError DEFINE_ASYNC_RET(executePreparedStatement(PreparedStatement* statement, EA::TDF::TimeValue timeout = 0));
    BlazeRpcError DEFINE_ASYNC_RET(executePreparedStatement(PreparedStatement* statement, DbResultPtr& result, EA::TDF::TimeValue timeout = 0));
    BlazeRpcError DEFINE_ASYNC_RET(startTxn(EA::TDF::TimeValue timeout = 0, bool commitTxnInProgress = true));
    BlazeRpcError DEFINE_ASYNC_RET(commit(EA::TDF::TimeValue timeout = 0));
    BlazeRpcError rollback(EA::TDF::TimeValue timeout = 0);

    void setPrefetchParameters(int32_t rowCount, int32_t maxMemory) { mDbConnInterface->setPrefetchParameters(rowCount, maxMemory); }
    void getPrefetchParameters(int32_t &rowCount, int32_t& maxMemory) { mDbConnInterface->getPrefetchParameters(rowCount, maxMemory); }
    void clearPrefetchParameters() { mDbConnInterface->clearPrefetchParameters(); }

public:
    typedef enum { READ_WRITE, READ_ONLY } Mode;
    static const char8_t* modeToString(Mode mode);
    static const ConnectionId DB_ADMIN_CONN_ID = UINT32_MAX;
    static bool isResumableErrorCode(BlazeRpcError error);
    static BlazeRpcError convertErrorCode(int error);

    Mode getMode() const { return mMode; }
    const char8_t* getName() const { return mName; }  
    uint32_t getConnNum() const { return mNum; }
    uint32_t getUseCount() const { return mDbConnInterface->getUseCount(); }

    PreparedStatement* getPreparedStatement(PreparedStatementId id) { return mDbConnInterface->getPreparedStatement(id); }
    DbInstancePool& getDbInstancePool() const { return mOwner;}
    DbConnPool& getDbConnPool() const;

    bool isWriteReplicationSynchronous() const;
    bool getSchemaUpdateBlocksAllTxn() const; // If true, avoid TRUNCATE TABLE (blocks all transactions)

    bool isConnected() const { return mDbConnInterface->isConnected(); }
    bool isActive() const { return mActive; }

    bool isTxnInProgress() { return mDbConnInterface->isTxnInProgress(); }

    int32_t getLogCategory() const { return mLogCategory; }
    const char8_t* getLastError() { return mDbConnInterface->getLastErrorMessageText(); }

protected:
    friend class MySqlAdmin;
    friend class MySqlAsyncAdmin;
    friend class DbConnPool;
    friend class DbConnInterface;
    friend class DbScheduler;

    const char8_t* getTypeName() const override { return "DbConnBase"; }

    BlazeRpcError DEFINE_ASYNC_RET(connect());
    void disconnect(BlazeRpcError reason);

    DbConnInterface& getDbConnInterface() { return *mDbConnInterface; }

    void setLogCategory(int32_t logCategory) { mLogCategory = logCategory; }

    void activate() { mActive = true; }
    void deactivate() { mActive = false; }
    void invalidate() { mValid = false; }

    void timeoutQuery() { mDbConnInterface->timeoutQuery(); }
    bool queryTimedOut() const { return mDbConnInterface->queryTimedOut(); }

protected:
    DbConnBase(const DbConfig& config, DbAdmin& admin, DbInstancePool& owner, uint32_t num, Mode mode);
    ~DbConnBase() override;

    void assignInternal() override;
    void releaseInternal() override;

    Blaze::BlazeRpcError executeQueryInternal(const Query* query, DbResult** result, EA::TDF::TimeValue timeout = 0);
    Blaze::BlazeRpcError executeStreamingQueryInternal(const Query* query, StreamingDbResult** result, EA::TDF::TimeValue timeout = 0);
    Blaze::BlazeRpcError executeMultiQueryInternal(const Query* query, DbResults** results, EA::TDF::TimeValue timeout = 0);
    Blaze::BlazeRpcError executePreparedStatementInternal(PreparedStatement* statement, DbResult** result, EA::TDF::TimeValue timeout = 0);
    
    Query* newQueryInternal(const char8_t* fileAndLine);

private:
    BlazeRpcError checkQuery(const Query* query);
    BlazeRpcError checkPrerequisites(EA::TDF::TimeValue& timeout);

private:
    //needs access to releaseInternal, and there's no actual inheritance path directly down to ThreadedDbConn
    friend class ThreadedDbConn;                                  

    DbInstancePool& mOwner;
    DbConnInterface* mDbConnInterface;
    uint32_t mNum;
    Mode mMode;
    char8_t mName[64];
    int32_t mLogCategory;
    bool mActive;
    bool mValid;
    EA::TDF::TimeValue mMinQueryRuntime;
    EA::TDF::TimeValue mMaxQueryRuntime;
};

//  DbConnPtr interface definition 
typedef FrameworkResourcePtr<DbConnBase> DbConnPtr;

//  Deprecated DbConn interface 
//  intrusive list node for the DbInstancePool.
struct DbConnNode : public eastl::intrusive_list_node
{
    DbConn& getDbConn();
    const DbConn& getDbConn() const;
};

class DbConn : public DbConnBase, private DbConnNode
{
    NON_COPYABLE(DbConn);

    friend struct DbConnNode;
    friend class DbInstancePool;            // accessible to DbInstancePool so it can add/remove DbConnNode from its intrusive list.

#ifdef BLAZE_ENABLE_DEPRECATED_DATABASE_INTERFACE
public:
    [[deprecated("use DbConnPtr instead")]]
    Query* newQuery(const char8_t* fileAndLine = nullptr) { return static_cast<Query*>(DbConnBase::newQueryInternal(fileAndLine)); }

    [[deprecated("use DbConnPtr instead")]]
    BlazeRpcError DEFINE_ASYNC_RET(executeQuery(const Query* query, DbResult** result, EA::TDF::TimeValue timeout = 0)) {
        return executeQueryInternal(query, result, timeout);
    }

    [[deprecated("use DbConnPtr instead")]]
    BlazeRpcError DEFINE_ASYNC_RET(executeStreamingQuery(const Query* query, StreamingDbResult** result, EA::TDF::TimeValue timeout = 0)) {
        return executeStreamingQueryInternal(query, result, timeout);
    }

    [[deprecated("use DbConnPtr instead")]]
    BlazeRpcError DEFINE_ASYNC_RET(executeMultiQuery(const Query* query, DbResults** results, EA::TDF::TimeValue timeout = 0)) {
        return executeMultiQueryInternal(query, results, timeout);
    }

    [[deprecated("use DbConnPtr instead")]]
    BlazeRpcError DEFINE_ASYNC_RET(executePreparedStatement(PreparedStatement* statement, DbResult** result, EA::TDF::TimeValue timeout = 0)) {
        return executePreparedStatementInternal(statement, result, timeout);
    }
#endif

public:
    DbConn(const DbConfig& config, DbAdmin& admin, DbInstancePool& owner, uint32_t num, Mode mode); 
    ~DbConn() override {}
    virtual void release() { FrameworkResource::release(); } 
    void assign();
};

inline DbConn& DbConnNode::getDbConn() { return *static_cast<DbConn*>(this); }
inline const DbConn& DbConnNode::getDbConn() const { return *static_cast<const DbConn*>(this); }

class DbConnMetrics
{
public:
    // Size of function capture argument space (increase if needed)
    static const uint32_t FIXED_FUNCTION_SIZE_BYTES = 24; 
    typedef enum { DB_OP_TRANSACTION, DB_OP_QUERY, DB_OP_MULTI_QUERY, DB_OP_PREPARED_STATEMENT, DB_OP_MAX } DbOp;
    typedef enum { DB_STAGE_SETUP, DB_STAGE_EXECUTE, DB_STAGE_CLEANUP, DB_STAGE_MAX } DbThreadStage;

    DbConnMetrics()
    {
        clear();
    }

    void clear()
    {
        memset(mDbOps, 0, sizeof(mDbOps));
        mQueryTime = 0;

        mOnThreadTime[DB_STAGE_SETUP] = 0;
        mOnThreadTime[DB_STAGE_EXECUTE] = 0;
        mOnThreadTime[DB_STAGE_CLEANUP] = 0;
    }

    void addQueryTime(const EA::TDF::TimeValue& qt, const EA::TDF::TimeValue& setup, const EA::TDF::TimeValue& execute, const EA::TDF::TimeValue& cleanup)
    {
        mQueryTime += qt;
        mOnThreadTime[DB_STAGE_SETUP] += setup;
        mOnThreadTime[DB_STAGE_EXECUTE] += execute;
        mOnThreadTime[DB_STAGE_CLEANUP] += cleanup;
    }

    void incrDbOp(DbOp op) { ++mDbOps[op]; }
    void incrDbQueryCount(const char* name)
    {
        FiberAccountingInfo& accInfo = gFiberManager->getFiberAccountingInfo(Fiber::getCurrentContext());
        DbQueryCountMap& counts = accInfo.getDbQueryCounts();
        ++counts[name];
    }

    void iterateDbOpCounts(eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, void (DbOp, uint64_t)> func)
    {
        for(int32_t idx = 0; idx < DB_OP_MAX; ++idx)
            func((DbOp)idx, mDbOps[idx]);
    }

    void iterateDbOnThreadTimes(eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, void (DbThreadStage, const EA::TDF::TimeValue&)> func)
    {
        for(int32_t idx = 0; idx < DB_STAGE_MAX; ++idx)
            func((DbThreadStage)idx, mOnThreadTime[idx]);
    }

    EA::TDF::TimeValue getQueryTime() const { return mQueryTime; }
    
private:
    uint64_t mDbOps[DB_OP_MAX];

    EA::TDF::TimeValue mQueryTime;

    EA::TDF::TimeValue mOnThreadTime[DB_STAGE_MAX];

};

namespace Metrics
{
    namespace Tag
    {
        extern TagInfo<DbConnMetrics::DbOp>* db_op;
        extern TagInfo<DbConnMetrics::DbThreadStage>* db_thread_stage;

        extern const char8_t* DB_GET_CONN_CONTEXT;
    }
}

extern FiberLocalManagedPtr<DbConnMetrics> gDbMetrics;


} // Blaze

#endif // BLAZE_DBCONN_H

