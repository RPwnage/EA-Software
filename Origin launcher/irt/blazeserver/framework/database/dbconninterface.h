/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_DBCONNINTERFACE_H
#define BLAZE_DBCONNINTERFACE_H

/*** Include files *******************************************************************************/

#include "framework/database/dbconfig.h"
#include "framework/logger.h"
#include "framework/database/dbresult.h"
#include "framework/database/preparedstatement.h"
#include "framework/database/dberrors.h"
#include "framework/database/mysql/blazemysql.h"
#include "EATDF/time.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/hash_map.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class DbInstancePool;
class Selector;
class Fiber;
class Query;
class DbConnBase;


class DbConnInterface
{
    NON_COPYABLE(DbConnInterface);

public:
    static const uint32_t CONNECT_TIMEOUT;
    static const int32_t UNSET_PREFETCH_ROW_COUNT = -1;
    static const int32_t UNSET_PREFETCH_MAX_MEMORY = -1;

    DbConnInterface(const DbConfig& config, DbConnBase& owner);
    virtual ~DbConnInterface();

    virtual void assign();
   
    virtual bool prepareForRelease() { return true; }
    virtual void release();

    virtual Query* newQuery(const char8_t* comment = nullptr) = 0;
    
    virtual bool isConnected() const = 0;
    virtual BlazeRpcError DEFINE_ASYNC_RET(connect()) = 0;
    virtual BlazeRpcError DEFINE_ASYNC_RET(testConnect(const DbConfig& config)) = 0;
    virtual void disconnect(BlazeRpcError reason) = 0;
    virtual BlazeRpcError DEFINE_ASYNC_RET(executeQuery(const Query* query, DbResult** result, EA::TDF::TimeValue timeout = 0)) = 0;
    virtual BlazeRpcError DEFINE_ASYNC_RET(executeStreamingQuery(const Query* query, StreamingDbResult** result, EA::TDF::TimeValue timeout = 0)) = 0;
    virtual BlazeRpcError DEFINE_ASYNC_RET(executeMultiQuery(const Query* query, DbResults** results, EA::TDF::TimeValue timeout = 0)) = 0;
    virtual BlazeRpcError DEFINE_ASYNC_RET(executePreparedStatement(PreparedStatement* statement, DbResult** result, EA::TDF::TimeValue timeout = 0)) = 0;
    virtual BlazeRpcError DEFINE_ASYNC_RET(startTxn(EA::TDF::TimeValue timeout = 0)) = 0;
    virtual BlazeRpcError DEFINE_ASYNC_RET(commit(EA::TDF::TimeValue timeout = 0)) = 0;
    virtual BlazeRpcError rollback(EA::TDF::TimeValue timeout = 0) = 0;

    bool isTxnInProgress() { return mTxnInProgress; }

    virtual PreparedStatement* createPreparedStatement(const PreparedStatementInfo& info) = 0;
    virtual PreparedStatement* getPreparedStatement(PreparedStatementId id);

    virtual uint32_t getUseCount() const { return mUseCount; }

    const char8_t* getName() const;

    DbConnBase& getOwner() const { return mOwner; }

    virtual EA::TDF::TimeValue calculateQueryTimeout(EA::TDF::TimeValue timeout);
    EA::TDF::TimeValue getQueryTimeout() const { return mQueryTimeout; } // returns last TimeValue calculated by calculateQueryTimeout; currently never called

    virtual const char8_t* getLastErrorMessageText() const = 0;
    virtual void closeStatement(MYSQL_STMT* stmt) = 0;

    // Methods relevant to synchronous db implementation only

    virtual void timeoutQuery() { mQueryTimedOut = true; }
    virtual bool queryTimedOut() const { return mQueryTimedOut; }

    virtual void setPrefetchParameters(int32_t rowCount, int32_t maxMemory);
    virtual void getPrefetchParameters(int32_t& rowCount, int32_t& maxMemory);
    virtual void clearPrefetchParameters();

    EA::TDF::TimeValue getQuerySetupTimeOnThread() { return mQuerySetupTimeOnThread; }
    EA::TDF::TimeValue getQueryExecutionTimeOnThread() { return mQueryExecutionTimeOnThread; }
    EA::TDF::TimeValue getQueryCleanupTimeOnThread() { return mQueryCleanupTimeOnThread; }

protected:

    void closeAllStatements();

    DbConnBase& mOwner;
    bool mTxnInProgress;
    int32_t mPrefetchRowCount;
    int32_t mPrefetchMemory;
    volatile bool mQueryTimedOut;
    EA::TDF::TimeValue mQueryTimeout;

    typedef eastl::hash_map<PreparedStatementId,PreparedStatement*> StatementsById;
    StatementsById mStatements;

    uint32_t mUseCount;
    EA::TDF::TimeValue mQuerySetupTimeOnThread;
    EA::TDF::TimeValue mQueryExecutionTimeOnThread;
    EA::TDF::TimeValue mQueryCleanupTimeOnThread;

};

}// Blaze
#endif // BLAZE_DBCONNINTERFACE_H

