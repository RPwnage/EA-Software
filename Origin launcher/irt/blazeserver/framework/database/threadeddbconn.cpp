/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/database/threadeddbconn.h"
#include "framework/database/threadedadmin.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/query.h"
#include "framework/connection/selector.h"
#include "framework/system/fiber.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define RECORD_TIME_AND_RESTART(var) \
    { \
        TimeValue now =  TimeValue::getTimeOfDay(); \
        var = now - st; \
        st = now; \
    }

// This macro is used by all the helper functions in this class that need to make calls to the
// DB.  It handles the logic around connecting to the DB if the connection had been lost.
// If the connection is not established and this is the first time this connection has been
// used by its owner then it will attempt reconnection.  If the connection appears connected but
// the query initially fails because of a disconnection then it will attempt reconnection only
// if this is the first operation being issued against this connection by the owner.  This is
// done to prevent a higher level command trying to issue a set of queries (eg. as part of a
// transaction) and having the connection drop part way through the set of queries.  We don't want
// to silently reconnect the connection if in the middle of a transaction.
#define EXECUTE_DB_OPERATION(func) \
    TimerId expiryTimerId = startQueryTimer(timeout); \
 \
    if (!isConnected()) \
    { \
        if (mUseCount == 0) \
            error = connectImpl(); \
        else \
            error = DB_ERR_DISCONNECTED; \
    } \
 \
    if (error == DB_ERR_OK) \
    { \
        while (error == DB_ERR_OK) \
        { \
            error = func; \
            ++mUseCount; \
            if (error != DB_ERR_DISCONNECTED) \
                break; \
 \
            if (mUseCount > 1) \
                break; \
\
            if (mQueryTimedOut) \
            { \
                error = DB_ERR_TIMEOUT; \
                break;\
            } \
 \
            error = connectImpl(); \
        } \
    } \
\
    static_cast<ThreadedAdmin&>(mOwner.getDbInstancePool().getDbAdmin()).cancelKill(expiryTimerId);


/*** Public Methods ******************************************************************************/

ThreadedDbConn::ThreadedDbConn(const DbConfig& config, DbConnBase& owner)
    : DbConnInterface(config, owner),
      mConnected(false),
      mCommandInProgress(false),
      mReleaseWhenFinished(false),
      mRollbackWhenFinished(false),
      mFreeStreamingResultsWhenFinished(false),
      mCurrentQuery(nullptr),
      mCurrentPreparedStatement(nullptr),
      mCurrentDbResultPtr(nullptr),
      mCurrentDbResultsPtr(nullptr),
      mCurrentStreamingDbResultPtr(nullptr),
      mCurrentStreamingDbResult(nullptr),
      mCurrentError(nullptr),
      mSelector(*gSelector)
{
}   

bool ThreadedDbConn::prepareForRelease()
{
    bool result = true;

    lockState();
    //if there's a transaction in progress we can only release when we're finished.
    if (mCommandInProgress)
    {
        result = false;
        mReleaseWhenFinished = true;
    }
    unlockState();

    return result;
}

BlazeRpcError ThreadedDbConn::connect()
{
    BlazeRpcError error = DB_ERR_SYSTEM;

    if (!checkAndSetCommandInProgress())
        return ERR_SYSTEM;

    lockState();
    mCurrentError = &error;
    unlockState();

    if (gIsDbThread)
    {
        connectHelper();
        clearState();
    }
    else
    {
        BlazeRpcError sleepError = executeBlockingHelperAndClearState(&ThreadedDbConn::connectHelper);
        if (sleepError != Blaze::ERR_OK)
            error = (sleepError == Blaze::ERR_TIMEOUT) ? DB_ERR_TIMEOUT : DB_ERR_SYSTEM;
    }

    return error;
}

BlazeRpcError ThreadedDbConn::testConnect(const DbConfig& config)
{
    BlazeRpcError error = DB_ERR_SYSTEM;

    if (!checkAndSetCommandInProgress())
        return ERR_SYSTEM;

    lockState();
    mCurrentError = &error;
    unlockState();

    if (gIsDbThread)
    {
        testConnectHelper(config);
    }
    else
    {
        DbScheduler::DbScheduleJob *job =
            BLAZE_NEW MethodCall1Job<ThreadedDbConn, const DbConfig&, DbScheduler::DbScheduleJob>(
                this, &ThreadedDbConn::testConnectHelper, config);
        BlazeRpcError sleepError = gDbScheduler->queueJob(mOwner.getDbConnPool().getId(), *job, true);
        if (sleepError != Blaze::ERR_OK)
            error = (sleepError == Blaze::ERR_TIMEOUT) ? DB_ERR_TIMEOUT : DB_ERR_SYSTEM;
    }

    clearState();

    return error;
}

void ThreadedDbConn::disconnect(BlazeRpcError reason)
{
    if (!checkAndSetCommandInProgress())
        return;

    lockState();
    mCurrentDisconnectReason = reason;
    unlockState();

    if (gIsDbThread)
    {
        if (reason != DB_ERR_TRANSACTION_NOT_COMPLETE)
            disconnectHelper();
        else
            disconnectAndReleaseHelper();
    }
    else
    {
        if (reason != DB_ERR_TRANSACTION_NOT_COMPLETE)
            executeDisconnectHelper(&ThreadedDbConn::disconnectHelper);
        else
            executeDisconnectHelper(&ThreadedDbConn::disconnectAndReleaseHelper);
    }
}

BlazeRpcError ThreadedDbConn::executeQuery(const Query* query, DbResult** result, TimeValue timeout)
{
    BlazeRpcError error = DB_ERR_SYSTEM;

    if (!checkAndSetCommandInProgress())
        return ERR_SYSTEM;

    lockState();
    mCurrentQuery = query->get();
    mCurrentDbResultPtr = result;
    mCurrentError = &error;
    mCurrentTimeout = timeout;
    unlockState();

    if (gIsDbThread)
    {
        executeQueryHelper();
        clearState();
    }
    else
    {
        BlazeRpcError sleepError = executeBlockingHelperAndClearState(&ThreadedDbConn::executeQueryHelper);
        if (sleepError != Blaze::ERR_OK)
            error = (sleepError == Blaze::ERR_TIMEOUT) ? DB_ERR_TIMEOUT : DB_ERR_SYSTEM;

    }

    return error;
}

BlazeRpcError ThreadedDbConn::executeStreamingQuery(const Query* query, StreamingDbResult** result, TimeValue timeout)
{
    BlazeRpcError error = DB_ERR_SYSTEM;

    if (!checkAndSetCommandInProgress())
        return ERR_SYSTEM;

    lockState();
    mCurrentQuery = query->get();
    mCurrentStreamingDbResultPtr = result;
    mCurrentError = &error;
    mCurrentTimeout = timeout;
    unlockState();

    if (gIsDbThread)
    {
        executeStreamingQueryHelper();
        clearState();
    }
    else
    {
        BlazeRpcError sleepError = executeBlockingHelperAndClearState(&ThreadedDbConn::executeStreamingQueryHelper);
        if (sleepError != Blaze::ERR_OK)
            error = (sleepError == Blaze::ERR_TIMEOUT) ? DB_ERR_TIMEOUT : DB_ERR_SYSTEM;
    }

    return error;
}


BlazeRpcError ThreadedDbConn::fetchNextStreamingResultRows(StreamingDbResult& result, TimeValue timeout)
{
    BlazeRpcError error = DB_ERR_SYSTEM;

    if (!checkAndSetCommandInProgress())
        return ERR_SYSTEM;

    lockState();
    mCurrentStreamingDbResult = &result;
    mCurrentError = &error;
    mCurrentTimeout = timeout;
    unlockState();

    if (gIsDbThread)
    {
        fetchNextStreamingResultRowsHelper();
        clearState();
    }
    else
    {
        BlazeRpcError sleepError = executeBlockingHelperAndClearState(&ThreadedDbConn::fetchNextStreamingResultRowsHelper);
        if (sleepError != Blaze::ERR_OK)
            error = (sleepError == Blaze::ERR_TIMEOUT) ? DB_ERR_TIMEOUT : DB_ERR_SYSTEM;
    }

    return error;
}


BlazeRpcError ThreadedDbConn::freeStreamingResults(TimeValue timeout)
{
    BlazeRpcError error = DB_ERR_SYSTEM;

    //check the command in progress manually here
    lockState();
    if (!mCommandInProgress)
    {
        mCommandInProgress = true;
        mCurrentError = &error;
        mCurrentTimeout = timeout;
    }
    else
    {
        //set a to cleanup afterwards and then bail
        mFreeStreamingResultsWhenFinished = true;
        unlockState();  //make sure to unlock
        return DB_ERR_OK;
    }
    unlockState();

    if (gIsDbThread)
    {
        freeStreamingResultsHelper();
        clearState();
    }
    else
    {
        BlazeRpcError sleepError = executeBlockingHelperAndClearState(&ThreadedDbConn::freeStreamingResultsHelper);
        if (sleepError != Blaze::ERR_OK)
            error = (sleepError == Blaze::ERR_TIMEOUT) ? DB_ERR_TIMEOUT : DB_ERR_SYSTEM;
    }

    return error;
}



BlazeRpcError ThreadedDbConn::executeMultiQuery(const Query* query, DbResults** results, TimeValue timeout)
{
    BlazeRpcError error = DB_ERR_SYSTEM;

    if (!checkAndSetCommandInProgress())
        return ERR_SYSTEM;

    lockState();
    mCurrentQuery = query->get();
    mCurrentDbResultsPtr = results;
    mCurrentError = &error;
    mCurrentTimeout = timeout;
    unlockState();

    if (gIsDbThread)
    {
        executeMultiQueryHelper();
        clearState();
    }
    else
    {
        BlazeRpcError sleepError = executeBlockingHelperAndClearState(&ThreadedDbConn::executeMultiQueryHelper);
        if (sleepError != Blaze::ERR_OK)
            error = (sleepError == Blaze::ERR_TIMEOUT) ? DB_ERR_TIMEOUT : DB_ERR_SYSTEM;
    }

    return error;
}

BlazeRpcError ThreadedDbConn::executePreparedStatement(PreparedStatement* statement, DbResult** result, TimeValue timeout)
{
    BlazeRpcError error = DB_ERR_SYSTEM;

    if (!checkAndSetCommandInProgress())
        return ERR_SYSTEM;

    lockState();
    mCurrentPreparedStatement = statement;
    mCurrentDbResultPtr = result;
    mCurrentError = &error;
    mCurrentTimeout = timeout;
    unlockState();

    if (gIsDbThread)
    {
        executePreparedStatementHelper();
        clearState();
    }
    else
    {
        BlazeRpcError sleepError = executeBlockingHelperAndClearState(&ThreadedDbConn::executePreparedStatementHelper);
        if (sleepError != Blaze::ERR_OK)
            error = (sleepError == Blaze::ERR_TIMEOUT) ? DB_ERR_TIMEOUT : DB_ERR_SYSTEM;
    }

    return error;
}


BlazeRpcError ThreadedDbConn::startTxn(TimeValue timeout)
{
    BlazeRpcError error = DB_ERR_SYSTEM;

    if (!checkAndSetCommandInProgress())
        return ERR_SYSTEM;

    lockState();
    mCurrentError = &error;
    mCurrentTimeout = timeout;
    unlockState();

    if (gIsDbThread)
    {
        startTxnHelper();
        clearState();
    }
    else
    {
        BlazeRpcError sleepError = executeBlockingHelperAndClearState(&ThreadedDbConn::startTxnHelper);

        if (sleepError != Blaze::ERR_OK)
        {
            if (error == DB_ERR_OK)
            {
                // The start transaction call completed but there was a system error so rollback
                // the transaction before returning.  We either do it directly or tell the thread to 
                // schedule it when its finally done with the transaction
                bool doRollback = true;
                lockState();
                {
                    if (mCommandInProgress)
                    {
                        //the DB command is still going, so just set a flag to roll us back when done
                        mRollbackWhenFinished = true;
                        doRollback = false;
                    }
                }
                unlockState();

                //if the command wasn't in progress, just go ahead and rollback now.
                if (doRollback)
                {
                    error = rollback(timeout);
                }
            }
            error = (sleepError == Blaze::ERR_TIMEOUT) ? DB_ERR_TIMEOUT : DB_ERR_SYSTEM;
        }
    }

    if (error == DB_ERR_OK)
        mTxnInProgress = true;
    else
        mTxnInProgress = false;

    return error;
}

BlazeRpcError ThreadedDbConn::commit(TimeValue timeout)
{
    BlazeRpcError error = DB_ERR_OK;

    if (!checkAndSetCommandInProgress())
        return ERR_SYSTEM;

    lockState();
    mCurrentError = &error;
    mCurrentTimeout = timeout;
    unlockState();

    if (gIsDbThread)
    {
        commitHelper();
        clearState();
    }
    else
    {
        BlazeRpcError sleepError = executeBlockingHelperAndClearState(&ThreadedDbConn::commitHelper);
        if (sleepError != Blaze::ERR_OK)
            error = (sleepError == Blaze::ERR_TIMEOUT) ? DB_ERR_TIMEOUT : DB_ERR_SYSTEM;
    }

    if (error == DB_ERR_OK)
        mTxnInProgress = false;

    return error;
}

BlazeRpcError ThreadedDbConn::rollback(TimeValue timeout)
{
    BlazeRpcError error = DB_ERR_OK;

    if (!checkAndSetCommandInProgress())
        return ERR_SYSTEM;

    lockState();
    mCurrentError = &error;
    mCurrentTimeout = timeout;
    unlockState();

    if (gIsDbThread)
    {
        rollbackHelper();
        clearState();  
    }
    else
    {
        BlazeRpcError sleepError = executeBlockingHelperAndClearState(&ThreadedDbConn::rollbackHelper);
        if (sleepError != Blaze::ERR_OK)
            error = (sleepError == Blaze::ERR_TIMEOUT) ? DB_ERR_TIMEOUT : DB_ERR_SYSTEM;
    }

    if (error == DB_ERR_OK)
        mTxnInProgress = false;

    return error;
}


/*** Private Methods *****************************************************************************/

void ThreadedDbConn::executeQueryHelper()
{
    TimeValue st = TimeValue::getTimeOfDay();
    BlazeRpcError error = DB_ERR_OK;
    
    DbResult* result = nullptr;
    lockState();
    bool useResult = (mCurrentDbResultPtr != nullptr);
    char8_t* query = (mCurrentQuery != nullptr) ? blaze_strdup(mCurrentQuery) : nullptr;
    TimeValue timeout = mCurrentTimeout;
    unlockState();
    
    RECORD_TIME_AND_RESTART(mQuerySetupTimeOnThread);

    if (query != nullptr)
    {
        EXECUTE_DB_OPERATION(executeQueryImpl(query, useResult ? &result : nullptr));
        BLAZE_FREE(query); 
    }

    RECORD_TIME_AND_RESTART(mQueryExecutionTimeOnThread);
    
    //lock and attempt to set the result
    lockState();
    if (mCurrentDbResultPtr != nullptr)
    {
        *mCurrentDbResultPtr = result;
    }
    else if (result != nullptr)
    {
        delete result;
    }

    if (mCurrentError != nullptr)
    {
        *mCurrentError = error;
    }
    clearCommandInProgress(); //clears the lock
    RECORD_TIME_AND_RESTART(mQueryCleanupTimeOnThread);
}

void ThreadedDbConn::executeStreamingQueryHelper()
{
    TimeValue st = TimeValue::getTimeOfDay();
    BlazeRpcError error = DB_ERR_OK;

    StreamingDbResult* result = nullptr;
    lockState();
    bool useResult = (mCurrentStreamingDbResultPtr != nullptr);  //No need to lock here, because it won't go from nullptr to not nullptr, only not nullptr to nullptr, in which case we don't care
    char8_t* query = (mCurrentQuery != nullptr) ? blaze_strdup(mCurrentQuery) : nullptr;
    TimeValue timeout = mCurrentTimeout;
    unlockState();

    RECORD_TIME_AND_RESTART(mQuerySetupTimeOnThread);

    if (query != nullptr)
    {
        EXECUTE_DB_OPERATION(executeStreamingQueryImpl(query, useResult ? &result : nullptr));
        BLAZE_FREE(query);
    }

    RECORD_TIME_AND_RESTART(mQueryExecutionTimeOnThread);

    lockState();
    if (mCurrentStreamingDbResultPtr != nullptr)
    {
        *mCurrentStreamingDbResultPtr = result;
    }
    else if (result != nullptr)
    {
        delete result;
    }

    if (mCurrentError != nullptr)
    {
        *mCurrentError = error;
    }
    clearCommandInProgress(); //clears the lock

    RECORD_TIME_AND_RESTART(mQueryCleanupTimeOnThread);
}


void ThreadedDbConn::fetchNextStreamingResultRowsHelper()
{
    BlazeRpcError error = DB_ERR_OK;
    TimeValue st = TimeValue::getTimeOfDay();

    lockState();            
    TimeValue timeout = mCurrentTimeout;
    unlockState();

    RECORD_TIME_AND_RESTART(mQuerySetupTimeOnThread);

    EXECUTE_DB_OPERATION(fetchNextStreamingResultRowsImpl());

    RECORD_TIME_AND_RESTART(mQueryExecutionTimeOnThread);

    //lock and attempt to set the row
    lockState();
    if (mCurrentError != nullptr)
    {
        *mCurrentError = error;
    }
    clearCommandInProgress(); //clears the lock

    RECORD_TIME_AND_RESTART(mQueryCleanupTimeOnThread);

}

void ThreadedDbConn::freeStreamingResultsHelper()
{
    BlazeRpcError error = DB_ERR_OK;
    TimeValue st = TimeValue::getTimeOfDay();

    lockState();            
    TimeValue timeout = mCurrentTimeout;
    unlockState();

    RECORD_TIME_AND_RESTART(mQuerySetupTimeOnThread);

    EXECUTE_DB_OPERATION(freeStreamingResultsImpl());

    RECORD_TIME_AND_RESTART(mQueryExecutionTimeOnThread);

    //lock and attempt to set the row
    lockState();
    if (mCurrentError != nullptr)
    {
        *mCurrentError = error;
    }
    clearCommandInProgress(); //clears the lock

    RECORD_TIME_AND_RESTART(mQueryExecutionTimeOnThread);
}

void ThreadedDbConn::executeMultiQueryHelper()
{
    TimeValue st = TimeValue::getTimeOfDay();
    BlazeRpcError error = DB_ERR_OK;

    DbResults* results = nullptr;
    lockState();
    bool useResults = (mCurrentDbResultsPtr != nullptr); 
    char8_t* query = (mCurrentQuery != nullptr) ? blaze_strdup(mCurrentQuery) : nullptr;
    TimeValue timeout = mCurrentTimeout;
    unlockState();

    RECORD_TIME_AND_RESTART(mQuerySetupTimeOnThread);

    if (query != nullptr)
    {
        EXECUTE_DB_OPERATION(executeMultiQueryImpl(query, useResults ? &results : nullptr));
        BLAZE_FREE(query);
    }

    RECORD_TIME_AND_RESTART(mQueryExecutionTimeOnThread);

    lockState();
    if (mCurrentDbResultsPtr != nullptr)
    {
        *mCurrentDbResultsPtr = results;
    }
    else if (results != nullptr)
    {
        delete results;
    }

    if (mCurrentError != nullptr)
    {
        *mCurrentError = error;
    }
    clearCommandInProgress(); //clears the lock

    RECORD_TIME_AND_RESTART(mQueryCleanupTimeOnThread);

}

void ThreadedDbConn::executePreparedStatementHelper()
{
    TimeValue st = TimeValue::getTimeOfDay();
    BlazeRpcError error = DB_ERR_OK;

    DbResult* result = nullptr;
    lockState();
    bool useResult = (mCurrentDbResultPtr != nullptr); 
    PreparedStatement* stmt = mCurrentPreparedStatement;
    TimeValue timeout = mCurrentTimeout;
    unlockState();

    RECORD_TIME_AND_RESTART(mQuerySetupTimeOnThread);

    if (stmt != nullptr)
    {
        if (stmt->getIsRegistered())
        {
            EXECUTE_DB_OPERATION(executePreparedStatementImpl(*stmt, useResult ? &result : nullptr)); 
        }
        else
        {
            // Is this the best place to close the statement if not registered?
            stmt->close(*this);
        }
    }

    RECORD_TIME_AND_RESTART(mQueryExecutionTimeOnThread);

    //lock and attempt to set the result
    lockState();
    if (mCurrentDbResultPtr != nullptr)
    {
        *mCurrentDbResultPtr = result;
    }
    else if (result != nullptr)
    {
        delete result;
    }

    if (mCurrentError != nullptr)
    {
        *mCurrentError = error;
    }
    clearCommandInProgress(); //clears the lock

    RECORD_TIME_AND_RESTART(mQueryCleanupTimeOnThread);
}


void ThreadedDbConn::startTxnHelper()
{
    TimeValue st = TimeValue::getTimeOfDay();
    
    lockState();
    TimeValue timeout = mCurrentTimeout;
    unlockState();

    RECORD_TIME_AND_RESTART(mQuerySetupTimeOnThread);

    BlazeRpcError error = DB_ERR_OK;
    EXECUTE_DB_OPERATION(startTxnImpl());

    RECORD_TIME_AND_RESTART(mQueryExecutionTimeOnThread);

    lockState();
    if (mCurrentError != nullptr)
    {
        *mCurrentError = error;
    }
    clearCommandInProgress(); //clears the lock

    RECORD_TIME_AND_RESTART(mQueryCleanupTimeOnThread);

}

void ThreadedDbConn::commitHelper()
{
    BlazeRpcError error;

    TimeValue st = TimeValue::getTimeOfDay();
    lockState();
    TimeValue timeout = mCurrentTimeout;
    unlockState();
    
    RECORD_TIME_AND_RESTART(mQuerySetupTimeOnThread);

    if (!isConnected())
    {
        error = DB_ERR_DISCONNECTED;
    }
    else
    {
        TimerId expiryTimerId = startQueryTimer(timeout);

        error = commitImpl();

        static_cast<ThreadedAdmin&>(mOwner.getDbInstancePool().getDbAdmin()).cancelKill(expiryTimerId);
    }
    
    RECORD_TIME_AND_RESTART(mQueryExecutionTimeOnThread);

    lockState();
    if (mCurrentError != nullptr)
    {
        *mCurrentError = error;
    }
    clearCommandInProgress(); //clears the lock
    
    RECORD_TIME_AND_RESTART(mQueryCleanupTimeOnThread);
}

void ThreadedDbConn::rollbackHelper()
{
    TimeValue st = TimeValue::getTimeOfDay();
    BlazeRpcError error;

    lockState();
    TimeValue timeout = mCurrentTimeout;
    unlockState();

    RECORD_TIME_AND_RESTART(mQuerySetupTimeOnThread);

    if (!isConnected())
    {
        error = DB_ERR_DISCONNECTED;
    }
    else
    {
        TimerId expiryTimerId = startQueryTimer(timeout);

        error = rollbackImpl();

        static_cast<ThreadedAdmin&>(mOwner.getDbInstancePool().getDbAdmin()).cancelKill(expiryTimerId);
    }

    RECORD_TIME_AND_RESTART(mQueryExecutionTimeOnThread);


    lockState();
    if (mCurrentError != nullptr)
    {
        *mCurrentError = error;
    }
    clearCommandInProgress(); //clears the lock

    RECORD_TIME_AND_RESTART(mQueryCleanupTimeOnThread);
}

void ThreadedDbConn::connectHelper()
{
    TimeValue st = TimeValue::getTimeOfDay();

    RECORD_TIME_AND_RESTART(mQuerySetupTimeOnThread);

    BlazeRpcError error = connectImpl();
    RECORD_TIME_AND_RESTART(mQueryExecutionTimeOnThread);


    lockState();
    if (mCurrentError != nullptr)
    {
        *mCurrentError = error;
    }
    clearCommandInProgress(); //clears the lock
    RECORD_TIME_AND_RESTART(mQueryCleanupTimeOnThread);
}

void ThreadedDbConn::testConnectHelper(const DbConfig& dbConfig)
{
    BlazeRpcError error = testConnectImpl(dbConfig);
    lockState();
    if (mCurrentError != nullptr)
    {
        *mCurrentError = error;
    }
    clearCommandInProgress();
}

void ThreadedDbConn::disconnectHelper()
{
    TimeValue st = TimeValue::getTimeOfDay();
    RECORD_TIME_AND_RESTART(mQuerySetupTimeOnThread);

    disconnectImpl(mCurrentDisconnectReason);
    RECORD_TIME_AND_RESTART(mQueryExecutionTimeOnThread);


    lockState();  //must lock before calling clearCommandInProgress
    clearCommandInProgress(); //clears the lock

    RECORD_TIME_AND_RESTART(mQueryCleanupTimeOnThread);
}

void ThreadedDbConn::disconnectAndReleaseHelper()
{
    disconnectHelper();
    
    // we *must* release the owner DbConn back to it's pool because the 
    // calling fiber did not do so itself, since it may have timed out 
    // and couldn't wait for the disconnect() operation to complete.
    mOwner.getDbInstancePool().getDbConnPool().release(mOwner);
}

void ThreadedDbConn::lockState()
{
    mStateLock.Lock();
}

void ThreadedDbConn::unlockState()
{
    mStateLock.Unlock();
}

TimerId ThreadedDbConn::startQueryTimer(const TimeValue& timeout)
{
    mQueryTimedOut = false;
    ThreadedAdmin& admin = static_cast<ThreadedAdmin&>(mOwner.getDbInstancePool().getDbAdmin());
    return admin.killQuery(timeout, mOwner);
}

bool ThreadedDbConn::checkAndSetCommandInProgress()
{
    bool result = false;
   
    lockState();
    if (!mCommandInProgress)
    {
        mCommandInProgress = true;
        result = true;
    }
    else
    {
        BLAZE_ERR_LOG(Log::DB, "[ThreadedDbConn::checkAndSetCommandInProgress]: Attempted to use conn " << 
            mOwner.getName() << " while connection was in use on another thread.");
    }
    unlockState();

    return result;
}


void ThreadedDbConn::clearCommandInProgress()
{
    //The state should be locked _before_ calling this function!
    const bool doFreeResults = mFreeStreamingResultsWhenFinished;
    mFreeStreamingResultsWhenFinished = false;
    const bool doRollback = mRollbackWhenFinished;
    mRollbackWhenFinished = false;
    unlockState();

    if (mConnected)
    {
        if (doFreeResults)
        {
            freeStreamingResultsImpl();
        }

        if (doRollback)
        {
            rollbackImpl();
        }
    }

    lockState();
    mCommandInProgress = false;
    const bool doRelease = mReleaseWhenFinished;
    mReleaseWhenFinished = false;
    unlockState();

    //We do the release outside the lock, because the release can result in immediate assignment to a waiting thread, which would then fail if the lock wasn't undone.
    if (doRelease)
    {
        mOwner.releaseInternal(); 
    }
}

BlazeRpcError ThreadedDbConn::executeBlockingHelperAndClearState(void (ThreadedDbConn::*helperFunc)())
{
    DbScheduler::DbScheduleJob *job = 
        BLAZE_NEW MethodCallJob<ThreadedDbConn, DbScheduler::DbScheduleJob>(this, helperFunc);
    job->setPriority(Fiber::getCurrentPriority());
    bool jobWasQueued = false;

    BlazeRpcError sleepError = gDbScheduler->queueJob(mOwner.getDbConnPool().getId(), *job, true, &jobWasQueued);
    clearState();
    // reset the db conn state and release the conn if the queued job is not run due to early outs
    if (sleepError != ERR_OK && !jobWasQueued)
    {
        lockState();
        clearCommandInProgress(); //clears the lock
    }
    return sleepError;
}

BlazeRpcError ThreadedDbConn::executeDisconnectHelper(void (ThreadedDbConn::*helperFunc)())
{
    DbScheduler::DbScheduleJob *job = 
        BLAZE_NEW MethodCallJob<ThreadedDbConn, DbScheduler::DbScheduleJob>(this, helperFunc);
    job->setPriority(Fiber::getCurrentPriority());
    // if we are trying to disconnect a fiber that timed out(cancelled = true), schedule a non-blocking disconnect
    BlazeRpcError sleepError = gDbScheduler->queueJob(mOwner.getDbConnPool().getId(), *job, !Fiber::isCurrentlyCancelled());
    // IMPORTANT: executeDisconnectHelper must *never* call clearState(), because:
    // 1. The disconnect helpers *never* use the connection state for signalling the fiber
    // 2. The disconnectAndReleaseHelper() can release the connection from within the thread, 
    //    which means another db operation might already be scheduled on this connection by the time
    //    the server thread that invoked ThreadedDbConn::disconnect() wakes up and resumes execution; 
    //    therefore, we are forbidden(!) to call *any* mutating methods on this ThreadedDbConn 
    //    after the above call to gDbScheduler->queueJob() returns.
    return sleepError;
}

void ThreadedDbConn::clearState()
{
    lockState();
    mCurrentQuery = nullptr;
    mCurrentPreparedStatement = nullptr;
    mCurrentDbResultPtr = nullptr;
    mCurrentDbResultsPtr = nullptr;
    mCurrentStreamingDbResultPtr = nullptr;
    mCurrentStreamingDbResult = nullptr;
    mCurrentError = nullptr;
    unlockState();
}
} // namespace Blaze

