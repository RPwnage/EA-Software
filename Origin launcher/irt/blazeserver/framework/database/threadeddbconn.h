/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_THREADEDDBCONN_H
#define BLAZE_THREADEDDBCONN_H

/*** Include files *******************************************************************************/

#include "framework/database/dbconninterface.h"
#include "framework/database/dbconfig.h"
#include "framework/logger.h"
#include "framework/database/dbresult.h"
#include "framework/database/preparedstatement.h"
#include "framework/database/dberrors.h"
#include "EATDF/time.h"
#include "EASTL/intrusive_list.h"
#include "eathread/eathread_futex.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{
class DbInstancePool;
class Selector;
class Fiber;
class Query;
class DbConnPool;

class ThreadedDbConn : public DbConnInterface
{
    NON_COPYABLE(ThreadedDbConn);

public:
    ThreadedDbConn(const DbConfig& config, DbConnBase& owner);

    bool isConnected() const override { return mConnected; }

    Query* newQuery(const char8_t* comment = nullptr) override = 0;

    //Wrapper execute functions - they call the internal impl functions underneath.
    bool prepareForRelease() override;

    BlazeRpcError DEFINE_ASYNC_RET(connect() override);
    BlazeRpcError DEFINE_ASYNC_RET(testConnect(const DbConfig& config) override);
    void disconnect(BlazeRpcError reason) override;
    
    BlazeRpcError DEFINE_ASYNC_RET(executeQuery(const Query* query, DbResult** result, EA::TDF::TimeValue timeout = 0) override);
    BlazeRpcError DEFINE_ASYNC_RET(executeStreamingQuery(const Query* query, StreamingDbResult** result, EA::TDF::TimeValue timeout = 0) override);
    virtual BlazeRpcError DEFINE_ASYNC_RET(fetchNextStreamingResultRows(StreamingDbResult& result, EA::TDF::TimeValue timeout = 0));
    virtual BlazeRpcError DEFINE_ASYNC_RET(freeStreamingResults(EA::TDF::TimeValue timeout = 0));
    BlazeRpcError DEFINE_ASYNC_RET(executeMultiQuery(const Query* query, DbResults** results, EA::TDF::TimeValue timeout = 0) override);
    BlazeRpcError DEFINE_ASYNC_RET(executePreparedStatement(PreparedStatement* statement, DbResult** result, EA::TDF::TimeValue timeout = 0) override);

    BlazeRpcError DEFINE_ASYNC_RET(startTxn(EA::TDF::TimeValue timeout = 0) override);
    BlazeRpcError DEFINE_ASYNC_RET(commit(EA::TDF::TimeValue timeout = 0) override);
    BlazeRpcError rollback(EA::TDF::TimeValue timeout = 0) override;    

    
    PreparedStatement* createPreparedStatement(const PreparedStatementInfo& info) override = 0;

protected:
    //Abstract implementation functions to be implemented by the implementing object.
    virtual BlazeRpcError executeQueryImpl(const char8_t* query, DbResult** result) = 0;
    virtual BlazeRpcError executeStreamingQueryImpl(const char8_t* query, StreamingDbResult** result) = 0;
    virtual BlazeRpcError fetchNextStreamingResultRowsImpl() = 0;
    virtual BlazeRpcError freeStreamingResultsImpl() = 0;
    virtual BlazeRpcError executeMultiQueryImpl(const char8_t* query, DbResults** results) = 0;
    virtual BlazeRpcError executePreparedStatementImpl(PreparedStatement& stmt, DbResult** result) = 0;
    virtual BlazeRpcError startTxnImpl()=0;
    virtual BlazeRpcError commitImpl()=0;
    virtual BlazeRpcError rollbackImpl()=0;
    virtual BlazeRpcError connectImpl()=0;
    virtual BlazeRpcError testConnectImpl(const DbConfig& config)=0;
    virtual void disconnectImpl(BlazeRpcError reason)=0;
    const char8_t* getLastErrorMessageText() const override = 0;
    void lockState();
    void unlockState();

private:
    void executeQueryHelper();
    void executeStreamingQueryHelper();
    void fetchNextStreamingResultRowsHelper();
    void freeStreamingResultsHelper();
    void executeMultiQueryHelper();
    void executePreparedStatementHelper();
    void startTxnHelper();
    void commitHelper();
    void rollbackHelper();
    void connectHelper();
    void testConnectHelper(const DbConfig& config);
    void disconnectHelper();
    void disconnectAndReleaseHelper();
    
protected:
    bool mConnected;
    bool mCommandInProgress;
    bool mReleaseWhenFinished;
    bool mRollbackWhenFinished;
    bool mFreeStreamingResultsWhenFinished;

    const char8_t* mCurrentQuery;
    PreparedStatement* mCurrentPreparedStatement;
    DbResult** mCurrentDbResultPtr;
    DbResults** mCurrentDbResultsPtr;
    StreamingDbResult** mCurrentStreamingDbResultPtr;
    StreamingDbResult* mCurrentStreamingDbResult;
    BlazeRpcError* mCurrentError;
    EA::TDF::TimeValue mCurrentTimeout;
    BlazeRpcError mCurrentDisconnectReason;

private:
    TimerId startQueryTimer(const EA::TDF::TimeValue& timeout);
    bool checkAndSetCommandInProgress();
    void clearCommandInProgress();
    BlazeRpcError executeBlockingHelperAndClearState(void (ThreadedDbConn::*helperFunc)());
    BlazeRpcError executeDisconnectHelper(void (ThreadedDbConn::*helperFunc)());
    void clearState();

private:
    Selector& mSelector;
    EA::Thread::Futex mStateLock;
};

} // Blaze

#endif // BLAZE_THREADEDDBCONN_H

