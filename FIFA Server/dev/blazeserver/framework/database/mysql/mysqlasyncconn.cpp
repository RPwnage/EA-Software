/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class 

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/logger.h"
#include "framework/tdf/frameworkconfigdefinitions_server.h"
#include "framework/controller/controller.h"
#include "framework/database/dbscheduler.h"
#include "framework/database/dbconn.h"
#include "framework/database/dbconnpool.h"
#include "framework/database/mysql/mysqlresult.h"
#include "framework/database/mysql/mysqlquery.h"
#include "framework/database/mysql/mysqlasyncconn.h"
#include "framework/database/mysql/mysqlpreparedstatement.h"
#include "framework/database/mysql/mysqlpreparedstatementresult.h"
#include "framework/database/mysql/mysqlpreparedstatementrow.h"
#include "framework/database/mysql/mysqladmin.h"
#include "framework/database/mysql/mysqlasyncadmin.h"


namespace Blaze
{


// FUTURE: Use mysql_reset_connection_start/cont() https://dev.mysql.com/doc/refman/5.7/en/mysql-reset-connection.html instead of mysql_close() because that leaves TCP intact. This aborts all queries, and clears the connection back to normal state before returning it to the pool, also optionally we can do a ping before returning connection to the pool.
// MAYBE: use ping to avoid connections from being closed. https://dev.mysql.com/doc/refman/5.7/en/mysql-ping.html, you can use mysql_thread_id() to know if the connection is the same as the last one.

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

static const char8_t* NO_COMMAND = "none";

MySqlAsyncConn::MySqlAsyncConn(const DbConfig& config, DbConnBase& owner)
    : DbConnInterface(config, owner),
    mMySqlThreadId(0),
    mDisconnectReason(ERR_OK),
    mRetryPending(false),
    mScheduledKill(false),
    mInitialized(false),
    mState(STATE_DISCONNECTED),
    mPollFlags(0),
    mNameResolverJobId(NameResolver::INVALID_JOB_ID),
    mChannelHandle(INVALID_SOCKET),
    mPendingCommand(NO_COMMAND)
{
    memset(&mMySql, 0, sizeof(mMySql));
    enablePriorityChannel();
}

MySqlAsyncConn::~MySqlAsyncConn()
{
    disconnect(ERR_OK);
}

Query* MySqlAsyncConn::newQuery(const char8_t* comment)
{
    initialize(mOwner.getDbConnPool().getDbConfig().getDbConnConfig()); // need to ensure mMySql is initialized, to allow use of mysql_real_escape_string()
    return BLAZE_NEW MySqlQuery(mMySql, comment);
}

bool MySqlAsyncConn::isConnected() const
{
    return mState == STATE_CONNECTED;
}

void MySqlAsyncConn::initialize(const DbConnConfig& config)
{
    if (mInitialized)
        return;

    mysql_init(&mMySql);
    mysql_options(&mMySql, MYSQL_OPT_NONBLOCK, 0);

    if (config.getCompression())
        mysql_options(&mMySql, MYSQL_OPT_COMPRESS, 0);

    const char8_t* charset = config.getCharset();
    if ((charset != nullptr) && (charset[0] != '\0'))
        mysql_options(&mMySql, MYSQL_SET_CHARSET_NAME, charset);

    mysql_options(&mMySql, MYSQL_INIT_COMMAND, "SET AUTOCOMMIT=1"); // IMPORTANT: Commands run at startup must be failproof. They can never be allowed to put the connection in unrecoverable state(e.g: infinite loop, deadlock, etc.) since they run immediately on connect and if connect times out while waiting, we never engage our kill() logic to try to kill the corresponding threadid, since we don't have one yet!

    char8_t programNameBuf[128];
    blaze_snzprintf(programNameBuf, sizeof(programNameBuf), "blazeserver/%s", gController->getInstanceName());
    mysql_options4(&mMySql, MYSQL_OPT_CONNECT_ATTR_ADD, "program_name", programNameBuf); // Let mysql know who owns this conn (useful for debugging)

    if (config.getUseSsl())
    {
        my_bool boolVal = true;
        mysql_options(&mMySql, MYSQL_OPT_SSL_ENFORCE, (void*)&boolVal);
    }

    mMySqlThreadId = 0;
    mScheduledKill = false;
    mInitialized = true;
}

BlazeRpcError MySqlAsyncConn::connect()
{
    return connectImpl(mOwner.getDbInstancePool().getDbConfig(), false);
}

BlazeRpcError MySqlAsyncConn::testConnect(const DbConfig& config)
{
    // Create a temporary MySqlAsyncConn to avoid overwriting this conn's state.
    // The temp conn can share this conn's owner; the only method invoked by connectImpl/disconnect which references the owner is getName()
    MySqlAsyncConn tempConn(config, mOwner);
    BlazeRpcError rc = tempConn.connectImpl(config, true);
    tempConn.disconnect(ERR_OK);

    return rc;
}

BlazeRpcError MySqlAsyncConn::connectImpl(const DbConfig& config, bool isTest)
{
    const char8_t* methodName = (isTest ? "testConnect" : "connect");
    const char8_t* callerDesc = (isTest ? "MySqlAsyncConn::testConnect - mysql_real_connect_" : "MySqlAsyncConn::connect - mysql_real_connect_");

    BLAZE_TRACE_LOG(Log::DB, "[MySqlAsyncConn]." << methodName << ": connection(" << getName() << ") connecting to host: " << config.getHost() 
        << ", port: " << config.getPort() << ", database: " << config.getDatabase() << ", user: " << config.getUser() << ", ssl: " << config.getDbConnConfig().getUseSsl());

    if (mState != STATE_DISCONNECTED)
    {
        BLAZE_ERR_LOG(Log::DB, "[MySqlAsyncConn]." << methodName << ": connection(" << getName() << ") must be in disconnected state.");
        return ERR_SYSTEM;
    }

    TimeValue absoluteConnectTimeout = TimeValue::getTimeOfDay() + CONNECT_TIMEOUT * 1000 * 1000;

    mDisconnectReason = ERR_OK; // reset last disconnect reason
    mState = STATE_CONNECTING;

    BlazeError rc = ERR_SYSTEM;
    auto& dbConnConfig = config.getDbConnConfig();
    InetAddress addr(dbConnConfig.getHostname());
    // match MySqlConn behavior which does not resolve "localhost"
    if (blaze_strcmp("localhost", dbConnConfig.getHostname()) == 0)
    {
        addr.setIp(0x7F000001, InetAddress::HOST); // 127.0.0.1
    }
    else
    {
        rc = gNameResolver->resolve(addr, mNameResolverJobId, absoluteConnectTimeout);
        mNameResolverJobId = NameResolver::INVALID_JOB_ID; // always invalidate job id, to ensure we are ready to resolve again
        if (rc != ERR_OK || !addr.isResolved())
        {
            char8_t addrBuf[256];
            mState = STATE_DISCONNECTED;
            mDisconnectReason = rc;
            BLAZE_ERR_LOG(Log::DB, "[MySqlAsyncConn]." << methodName << ": connection(" << getName() << ") failed to resolve hostname=" << addr.toString(addrBuf, sizeof(addrBuf)) << ", blaze_err=" << rc);
            return rc;
        }
    }
    addr.setPort(dbConnConfig.getPort(), InetAddress::HOST);
    mHostAddress = addr;
    
    BLAZE_TRACE_LOG(Log::DB, "[MySqlAsyncConn]." << methodName << ": connection(" << getName() << ") successfully resolved hostname=" << dbConnConfig.getHostname() << ", ip=" << mHostAddress.getIpAsString());

    initialize(dbConnConfig);
    registerChannel(Channel::OP_NONE); // NOTE: registerChannel() must be done before setHandler()

    char8_t ipBuf[256];
    mHostAddress.toString(ipBuf, sizeof(ipBuf), InetAddress::IP_PORT);
    rc = performBlockingOp(callerDesc, ipBuf, absoluteConnectTimeout,
        [this, &dbConnConfig, methodName]() {
            MYSQL* dummy = &mMySql;
            uint32_t clientFlags = CLIENT_MULTI_STATEMENTS;
            auto pollFlags = mysql_real_connect_start(&dummy, &mMySql, mHostAddress.getIpAsString(), dbConnConfig.getUsername(), 
                dbConnConfig.getPassword(), dbConnConfig.getDatabase(), (int) mHostAddress.getPort(InetAddress::HOST), nullptr, clientFlags);
            if (pollFlags != 0)
            {
                // have socket handle after initiating connect
                mChannelHandle = static_cast<ChannelHandle>(mysql_get_socket(&mMySql));
                // NOTE: On Linux setHandler(), can only be called after mChannelHandle is valid, so do it here.
                if (!setHandler(this))
                {
                    BLAZE_ASSERT_LOG(Log::DB, "[MySqlAsyncConn]." << methodName << ": connection(" << getName() << ") failed to set self as handler!");
                    return 0;
                }
            }
            return pollFlags;
        },
        [this](uint32_t flags) {
            MYSQL* dummy = &mMySql;
            // complete the connect handshake
            return mysql_real_connect_cont(&dummy, &mMySql, flags);
        }
    );

    if (rc != ERR_OK)
    {
        disconnect(rc);
        return rc;
    }

    mMySqlThreadId = mysql_thread_id(&mMySql);
    mState = STATE_CONNECTED;

    BLAZE_TRACE_LOG(Log::DB, "[MySqlAsyncConn]." << methodName << ": connection(" << getName() << ") connected to (" << mHostAddress.getHostname() << ":" << mHostAddress.getPort(InetAddress::HOST) << ") with thread_id(" << mMySqlThreadId << ")");

    return rc;
}

void MySqlAsyncConn::disconnect(BlazeRpcError reason)
{
    if (mState == STATE_CONNECTED || mState == STATE_CONNECTING)
        mDisconnectReason = reason; // Retain true disconnect reason for debugging

    if (mPollEvent.isValid())
    {
        // If poll event is valid, disconnect is being issued on a different fiber than the one currently waiting for the poll event, wake up the waiting fiber the latter so that it can cleanly disconnect.
        // NOTE: We override error code that signals normal disconnect to disambiguate from the case of normal event completion. Deliberately use ERR_DISCONNECTED as opposed to DB_ERR_DISCONNECTED 
        // to discriminate a Blaze-triggered disconnect from a DB/network triggered disconnect and send the error handling down the non-reconnecting path inside executeWithRetry().
        Fiber::signal(mPollEvent, (reason == ERR_OK) ? ERR_DISCONNECTED : reason);
        // Deliberately fall through here, if the signaled fiber correctly initiated disconnect handling (which it always should), we'd be all done, otherwise as a safety net we will initiate directly.
    }
    else if (mNameResolverJobId != NameResolver::INVALID_JOB_ID)
    {
        auto jobId = mNameResolverJobId;
        mNameResolverJobId = NameResolver::INVALID_JOB_ID;
        gNameResolver->cancelResolve(jobId);
    }
    
    if (mState == STATE_DISCONNECTING)
    {
        // Already disconnecting, no-op
        return;
    }
    
    mState = STATE_DISCONNECTING;
    
    if (!mStreamingResults.empty())
    {
        BLAZE_TRACE_LOG(Log::DB, "[MySqlAsyncConn].disconnect: connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), clean up " << mStreamingResults.size() << " streaming results.");
        while (!mStreamingResults.empty())
        {
            eastl::hash_set<MySqlAsyncStreamingResult*>::iterator itr = mStreamingResults.begin();
            MySqlAsyncStreamingResult* sresult = *itr;
            mStreamingResults.erase(itr);
            cleanupStreamingResult(sresult);
        }
    }

    closeAllStatements();
    
    if (mInitialized)
    {
        char8_t ipBuf[256];
        mHostAddress.toString(ipBuf, sizeof(ipBuf), InetAddress::IP_PORT);
        BlazeError rc = performBlockingOp("MySqlAsyncConn::disconnect - mysql_close_", ipBuf, TimeValue::getTimeOfDay() + 1 * 1000 * 1000,
            [this]() {
                return mysql_close_start(&mMySql);
            },
            [this](uint32_t flags) {
                return mysql_close_cont(&mMySql, flags);
            }
        );
        if (rc != ERR_OK && rc != ERR_DB_DISCONNECTED)
        {
            BLAZE_WARN_LOG(Log::DB, "[MySqlAsyncConn].disconnect: Couldn't cleanly close connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), blaze_err=" << rc << ", force closed.");
        }
        if (getSlot() != INVALID_SLOT)
        {
            bool removeHandleIfValid = (mChannelHandle != INVALID_SOCKET); // handle channel handle may get ivalidated inside performBlockingOp()
            unregisterChannel(removeHandleIfValid); // explicitly remove the channel handle from the selector here, because setHandler(nullptr) doesn't do it
        }
        setHandler(nullptr); // clear the handler since connect needs to successfully set it again (needed for epollselector)
        mChannelHandle = INVALID_SOCKET;
        mInitialized = false;
    }
    
    mTxnInProgress = false;
    mState = STATE_DISCONNECTED;
}

BlazeRpcError MySqlAsyncConn::executeQuery(const Query* query, DbResult** result, TimeValue timeout)
{
    DbResults* results = nullptr;
    BlazeError rc = executeWithRetry(
        [this, query, &results, timeout] () {
            return executeQueryInternal(query, &results, false, timeout);
        });

    if (rc == ERR_OK)
    {
        if (result && !results->empty())
        {
            *result = results->front();
            results->front() = nullptr; // null out first result to prevent it from being freed.
        }
    }
    if (results)
        delete results;
    return rc;
}

BlazeRpcError MySqlAsyncConn::executeStreamingQuery(const Query* query, StreamingDbResult** result, TimeValue timeout)
{
    return executeWithRetry(
        [this, query, result, timeout] () {
            return executeStreamingQueryInternal(query, result, timeout);
        });
}

BlazeRpcError MySqlAsyncConn::executeMultiQuery(const Query* query, DbResults** results, TimeValue timeout)
{
    return executeWithRetry(
        [this, query, results, timeout] () {
            return executeQueryInternal(query, results, true, timeout);
        });
}

BlazeRpcError MySqlAsyncConn::executePreparedStatement(PreparedStatement* statement, DbResult** result, TimeValue timeout)
{
    return executeWithRetry(
        [this, statement, result, timeout] () {
            return statement->execute(result, *this, timeout);
        });
}

BlazeRpcError MySqlAsyncConn::startTxn(TimeValue timeout)
{
    auto* query = newQuery(__FILEANDLINE__);
    (*query) << "START TRANSACTION";
    BlazeError rc = executeQuery(query, nullptr, timeout);
    delete query;
    mTxnInProgress = (rc == ERR_OK);
    return rc;
}

BlazeRpcError MySqlAsyncConn::commit(TimeValue timeout)
{
    BlazeError rc = DB_ERR_DISCONNECTED;

    if (isConnected())
    {
        // Commit the transaction
        my_bool ret;
        rc = performBlockingOp("MySqlAsyncConn::commit - mysql_commit_", "COMMIT", timeout,
            [this, &ret]() {
                return mysql_commit_start(&ret, &mMySql);
            },
            [this, &ret](uint32_t flags) {
                return mysql_commit_cont(&ret, &mMySql, flags);
            }
        );
    }

    mTxnInProgress = (rc != ERR_OK);
    return rc;
}

BlazeRpcError MySqlAsyncConn::rollback(TimeValue timeout)
{
    BlazeError rc = DB_ERR_DISCONNECTED;

    if (isConnected())
    {
        // Rollback the transaction
        my_bool ret;
        rc = performBlockingOp("MySqlAsyncConn::rollback - mysql_rollback_", "ROLLBACK", timeout,
            [this, &ret]() {
            return mysql_rollback_start(&ret, &mMySql);
        },
            [this, &ret](uint32_t flags) {
            return mysql_rollback_cont(&ret, &mMySql, flags);
        });
    }

    mTxnInProgress = (rc != ERR_OK);
    return rc;
}

PreparedStatement* MySqlAsyncConn::createPreparedStatement(const PreparedStatementInfo& info)
{
    return BLAZE_NEW MySqlPreparedStatement(info);
}

const char8_t* MySqlAsyncConn::getLastErrorMessageText() const
{
    return mLastErrorText.c_str();
}

void MySqlAsyncConn::closeStatement(MYSQL_STMT* stmt)
{
    if (stmt != nullptr)
    {
        my_bool ret;
        performBlockingOp("MySqlAsyncConn::closeStatement - mysql_stmt_close_", "N/A", TimeValue::getTimeOfDay() + 1 * 1000 * 1000,
            [&ret, stmt]() {
                return mysql_stmt_close_start(&ret, stmt);
            },
            [&ret, stmt](uint32_t flags) {
                return mysql_stmt_close_cont(&ret, stmt, flags);
            });
    }
}

void MySqlAsyncConn::assign()
{
    mUseCount = 0;
}

void MySqlAsyncConn::release()
{
    if (mTxnInProgress)
    {
        // Transaction wasn't completed - possibly due to timeout.  Just close this connection
        // to reset the transaction (rather than worrying that subsequent calls to rollback might
        // fail) so that the connection will be in a good state for the next consumer.
        BLAZE_ERR_LOG(Log::DB, "[MySqlAsyncConn].release: connection(" << getName() << "), thread_id(" << mMySqlThreadId << ") had pending transaction, closing connection to reset its state.");

        disconnect(DB_ERR_TRANSACTION_NOT_COMPLETE);
    }

    mOwner.getDbConnPool().release(mOwner);
}

BlazeError MySqlAsyncConn::executeQueryInternal(const Query* queryPtr, DbResults** dbResults, bool isMultiQuery, TimeValue absoluteTimeout)
{
    int32_t status = 0;
    BlazeError rc = performBlockingOp("MySqlAsyncConn::executeQueryInternal - mysql_real_query_", queryPtr->get(), absoluteTimeout,
        [this, queryPtr, &status]() {
            return mysql_real_query_start(&status, &mMySql, queryPtr->get(), queryPtr->length());
        },
        [this, &status](uint32_t flags) {
            return mysql_real_query_cont(&status, &mMySql, flags);
        }); 

    if (rc != ERR_OK)
        return rc;

    DbResults* tempResults = (dbResults == nullptr ? nullptr : BLAZE_NEW DbResults);
    MYSQL_RES *res = nullptr;

    bool firstPass = true;
    uint32_t droppedResults = 0;
    do
    {
        if ((dbResults == nullptr) || (!isMultiQuery && !firstPass))
        {
            // Use mysql_use_result() since we don't want the rows anyway
            res = mysql_use_result(&mMySql);
            if (res != nullptr)
            {
                rc = performBlockingOp("MySqlAsyncConn::executeQueryInternal - mysql_free_result_", "N/A", absoluteTimeout,
                    [res]() {
                        return mysql_free_result_start(res);
                    },
                    [res](uint32_t flags) {
                        return mysql_free_result_cont(res, flags);
                    });

                if (rc != ERR_OK)
                    break;
            }
            ++droppedResults;
        }
        else
        {
            rc = performBlockingOp("MySqlAsyncConn::executeQueryInternal - mysql_store_result_", queryPtr->get(), absoluteTimeout,
                [this, &res]() {
                    return mysql_store_result_start(&res, &mMySql);
                },
                [this, &res](uint32_t flags) {
                    return mysql_store_result_cont(&res, &mMySql, flags);
                });

            if (rc != ERR_OK)
                break;

            MySqlResult* mysqlResult = BLAZE_NEW MySqlResult(*this);

            // WARNING: currently MySqlResult impl requires us to have the full result fetched from the server before we call dbResult->init(res), because the latter will iterate the rows internally. Instead we should have the caller provide the rows one at a time, thereby increasing parallelism, and ensuring that our system uses less buffer memory...
            rc = mysqlResult->init(res);
            if (rc == ERR_OK)
            {
                tempResults->push_back((MySqlResult*)mysqlResult);
            }
            else
            {
                delete mysqlResult;
                break;
            }
        }

        rc = performBlockingOp("MySqlAsyncConn::executeQueryInternal - mysql_next_result_", queryPtr->get(), absoluteTimeout,
            [this, &status]() {
                return mysql_next_result_start(&status, &mMySql);
            },
            [this, &status](uint32_t flags) {
                return mysql_next_result_cont(&status, &mMySql, flags);
            });

        if (rc != ERR_OK)
            break;

        firstPass = false;
    } while (status == 0);

    if (rc == ERR_OK)
    {
        EA_ASSERT_MSG(status == -1, "Status must be -1.");

        uint32_t totalResults = droppedResults;
        if (dbResults)
        {
            *dbResults = tempResults;
            ++totalResults;
        }

        if (!isMultiQuery && totalResults > 1)
            BLAZE_WARN_LOG(Log::DB, "[MySqlAsyncConn].executeQueryInternal: connection(" << getName() << "), thread_id(" << mMySqlThreadId << ") unexpected multiple(" << totalResults << ") results from single query=" << queryPtr->get() << ", (use DbConn::executeMultiQuery() instead)");
    }
    else
    {
        if (tempResults != nullptr)
            delete tempResults;
    }

    return rc;
}

BlazeError MySqlAsyncConn::executeWithRetry(const eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, BlazeError(void)>& func)
{
    BlazeError rc = ERR_OK;
    if (!isConnected())
    {
        if (mUseCount == 0)
            rc = connect();
        else
            rc = DB_ERR_DISCONNECTED;
    }

    while (rc == ERR_OK)
    {
        if (mUseCount == 0)
            mRetryPending = false;

        rc = func();
        ++mUseCount;

        if (rc == ERR_OK)
        {
            // command completed successfully
            break;
        }

        // NOTE: In code below, deliberately disconnect *before* queueing the kill query to the admin, 
        // because kill would invalidate our socket and we don't want to obfuscate our error handling in 
        // performBlockingOp() that takes great pains to always report the socket disconnect reason. 
        // By explicitly disconnecting first we ensure that the socket closure resulting from the kill 
        // is always cleanly and silently handled.
        if (rc == DB_ERR_DISCONNECTED)
        {
            // case 1:
            // mysql connection is not healthy
            // mysql initiated the error (non-resumable)
            //  disconnect mysql connection, kill outstanding commands, if first use, and disconnect succeeded, attempt reconnect
            disconnect(rc);
            scheduleKill();

            if (mUseCount > 1)
            {
                break;
            }

            if (mState != STATE_DISCONNECTED)
            {
                BLAZE_WARN_LOG(Log::DB, "[MySqlAsyncConn].executeWithRetry: connection(" << getName() << "), thread_id(" << mMySqlThreadId << ") failed to disconnect cleanly, will bypass auto-reconnect.");
                break;
            }

            if (!gDbScheduler->getAllowRequests())
            {
                BLAZE_TRACE_LOG(Log::DB, "[MySqlAsyncConn].executeWithRetry: connection(" << getName() << "), thread_id(" << mMySqlThreadId << ") will bypass auto-reconnect, shutdown in progress.");
                break;
            }

            rc = connect();
        }
        else
        {
            // IMPORTANT: We *must* use the rc here to check if the operation is resumable, rather than relying on the result of mysql_errno(&mMySql), 
            // because the latter may be already changed by cleanup error handling logic inside performBlockingOp() that ends up mutating the state 
            // of the mMySql object due to the mandatory unwinding of the error handling by invoking the *_cont(..., MYSQL_WAIT_TIMEOUT).
            if (DbConnBase::isResumableErrorCode(rc))
            {
                // case 2:
                // mysql connection is healthy
                // mysql initiated the error (resumable)
                //  leave mysql connection intact, return error to caller
            }
            else
            {
                // case 3:
                // mysql connection is healthy
                // blaze initiated the error (non-resumable) IE: ERR_TIMEOUT, ERR_DISCONNECT, ERR_CANCELED, ERR_SYSTEM
                //  mysql connection needs to be disconnected and any outstanding commands on it must be killed

                // FUTURE: Once we make use of mysql_reset_connection_start/cont instead of disonnect, we can avoid issuing kills here 
                // in the case when the reset works, because that means the conn is still good, without us having to recreate the 
                // underlying TCP socket thus avoiding imposing additional processing overhead on the blaze server and MYSQL...
                disconnect(rc);
                scheduleKill();
            }
            break;
        }
    }

    return rc;
}

BlazeError MySqlAsyncConn::executeStreamingQueryInternal(const Query* queryPtr, StreamingDbResult** streamingResult, TimeValue absoluteTimeout)
{
    int32_t status = 0;
    BlazeError rc = performBlockingOp("MySqlAsyncConn::executeStreamingQueryInternal - mysql_real_query_", queryPtr->get(), absoluteTimeout,
        [this, queryPtr, &status]() {
            return mysql_real_query_start(&status, &mMySql, queryPtr->get(), queryPtr->length());
        },
        [this, &status](uint32_t flags) {
            return mysql_real_query_cont(&status, &mMySql, flags);
        });

    if (rc != ERR_OK)
        return rc;

    MySqlAsyncStreamingResult* mysqlResult = nullptr;
    MYSQL_RES *res = mysql_use_result(&mMySql);
    if (streamingResult == nullptr)
    {
        // The caller doesn't want the result, so just free it
        if (res != nullptr)
            rc = performBlockingOp("MySqlAsyncConn::executeStreamingQueryInternal - mysql_free_result_", "N/A", absoluteTimeout,
                [res]() {
                    return mysql_free_result_start(res);
                },
                [res](uint32_t flags) {
                    return mysql_free_result_cont(res, flags);
                });
    }
    else
    {
        mysqlResult = BLAZE_NEW MySqlAsyncStreamingResult(*this);
        rc = mysqlResult->init(res);
    }

    if (mysql_more_results(&mMySql))
    {
        // We can't call mysql_next_result until we've fetched all rows from the current streaming query,
        // but we want to allow the caller to fetch the rows as needed via MySqlAsyncStreamingResult::next().

        BLAZE_ERR_LOG(Log::DB, "[MySqlAsyncConn].executeStreamingQueryInternal: connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), query=" << queryPtr->get() << ", streaming multiqueries are not supported; only the first query result will be processed, and this connection cannot be used again after this call.");
    }

    if (rc == ERR_OK)
    {
        if (mysqlResult != nullptr)
        {
            *streamingResult = mysqlResult;
            mStreamingResults.insert(mysqlResult);
        }
    }
    else
    {
        if (streamingResult != nullptr)
            *streamingResult = nullptr;
        if (mysqlResult != nullptr)
            delete mysqlResult;
    }

    return rc;
}


void MySqlAsyncConn::cleanupStreamingResult(MySqlAsyncStreamingResult* result)
{
    if (&result->mConn != this)
    {
        BLAZE_ERR_LOG(Log::DB, "[MySqlAsyncConn].cleanupStreamingResult: Attempt to use connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), host=" << ((mMySql.host_info != nullptr) ? mMySql.host_info : "unknown") << ") to cleanup MySqlAsyncStreamingResult owned by connection(" << result->mConnInfo << ")");
        return;
    }

    if (result->mResult != nullptr)
    {
        MYSQL_RES* res = result->mResult;
        result->mResult = nullptr;
        performBlockingOp("MySqlAsyncConn::cleanupStreamingResult - mysql_free_result_", "N/A", TimeValue::getTimeOfDay() + 1 * 1000 * 1000,
            [res]() {
                return mysql_free_result_start(res);
            },
            [res](uint32_t flags) {
                return mysql_free_result_cont(res, flags);
            }
        );
    }
}

BlazeError MySqlAsyncConn::performBlockingOp(const char8_t* callerDesc, const char8_t* queryDesc, const TimeValue& absoluteTimeout, const eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, int32_t(void)>& startFunc, const eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, int32_t(uint32_t)>& contFunc)
{
    auto getQueryDescPrefix = [this]() { return (mState == STATE_CONNECTED) ? "query=" : "host="; };

    if (mPollEvent.isValid())
    {
        BLAZE_ASSERT_LOG(Log::DB, callerDesc << "start FAILED; connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), " << getQueryDescPrefix() << queryDesc << ", blocking operation(" << mPendingCommand << ") already pending!");
        return ERR_SYSTEM;
    }
    
    BlazeError rc = ERR_OK;
    mPendingCommand = callerDesc;

    // --> Call the mysql_*_start function to prepare the blocking operation
    uint32_t pollFlags = startFunc();
    // <-- Call the mysql_*_start function to prepare the blocking operation
    
    auto mysqlErrNo = mysql_errno(&mMySql);
    if (pollFlags == 0)
    {
        if (mysqlErrNo == 0)
        {
            // This is only supposed to happen for disconnect calls that don't need to block when underlying socket is not connected
            BLAZE_TRACE_LOG(Log::DB, callerDesc << "start PASSED; connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), " << getQueryDescPrefix() << queryDesc);
        }
        else
        {
            mLastErrorText = mysql_error(&mMySql);
            rc = DbConnBase::convertErrorCode(mysqlErrNo); // IMPORTANT: very important to return this code since it's the only way to tell caller that something went wrong
            if (mState == STATE_CONNECTED || mState == STATE_CONNECTING)
            {
                if (mRetryPending)
                {
                    BLAZE_ERR_LOG(Log::DB, callerDesc << "start FAILED; connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), mysql_error=" << mLastErrorText << ", mysql_errno=" << mysqlErrNo << ", " << getQueryDescPrefix() << queryDesc << ", will not retry");
                }
                else
                {
                    mRetryPending = true;
                    BLAZE_WARN_LOG(Log::DB, callerDesc << "start FAILED; connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), mysql_error=" << mLastErrorText << ", mysql_errno=" << mysqlErrNo << ", " << getQueryDescPrefix() << queryDesc << ", will retry once");
                }
            }
        }
    }
    else
    {
        EA_ASSERT(mysqlErrNo == 0);

        if (mState == STATE_CONNECTED && mRetryPending)
        {
            mRetryPending = false;
            BLAZE_INFO_LOG(Log::DB, callerDesc << "start PASSED; connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), " << getQueryDescPrefix() << queryDesc << ", retry complete");
        }
        else
        {
            BLAZE_TRACE_LOG(Log::DB, callerDesc << "start PASSED; connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), " << getQueryDescPrefix() << queryDesc);
        }

        do {
            uint32_t interestOps = 0;
            if ((pollFlags & MYSQL_WAIT_READ))
                interestOps |= Channel::OP_READ;
            if ((pollFlags & MYSQL_WAIT_WRITE))
                interestOps |= Channel::OP_WRITE;
            if (interestOps == 0)
            {
                BLAZE_ASSERT_LOG(Log::DB, callerDesc << "cont FAILED; connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), " << getQueryDescPrefix() << queryDesc << " expected read or write events on channel, but none signaled!");
                rc = ERR_SYSTEM; // This should never happen!
                break;
            }
            EA_ASSERT(interestOps != 0);
            setInterestOps(interestOps);
            // Clear the poll flags in preparation for receiving a new one via onRead/onWrite handlers while fiber is sleeping
            mPollFlags = 0;
            // We'll wake up when something happens on the socket, or a timeout happens

            rc = Fiber::getAndWait(mPollEvent, callerDesc, absoluteTimeout);
            mPollEvent.reset();
            if (rc == ERR_OK)
            {
                pollFlags = mPollFlags;
            }
            else
            {
                // Signal mysql lib to clean up any outstanding connection state
                pollFlags = MYSQL_WAIT_TIMEOUT;
            }
            // --> Call the mysql_*_cont function to continue/complete the blocking operation
            pollFlags = contFunc(pollFlags);
            // <-- Call the mysql_*_cont function to continue/complete the blocking operation
        } while (pollFlags != 0);

        if (rc == ERR_OK)
        {
            // Intentionally only clear interest ops in success path, because any error calls contFunc(MYSQL_WAIT_TIMEOUT) which *always* closes the socket handle, making it an error to set interest ops on this channel
            setInterestOps(Channel::OP_NONE);
            mysqlErrNo = mysql_errno(&mMySql);
            
            if (mysqlErrNo == 0)
            {
                BLAZE_TRACE_LOG(Log::DB, callerDesc << "cont PASSED; connection(" << getName() << "), thread_id(" << mMySqlThreadId << ")");
            }
            else
            {
                mLastErrorText = mysql_error(&mMySql);
                rc = DbConnBase::convertErrorCode(mysqlErrNo); // There was a resumable or a non-resumable database error, convert it to blaze error code
                if (rc == ERR_DB_DISCONNECTED)
                {
                    if (mState != STATE_DISCONNECTING)
                    {
                        BLAZE_ERR_LOG(Log::DB, callerDesc << "cont FAILED; connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), mysql_error=" << mLastErrorText << ", mysql_errno=" << mysqlErrNo << ", " << getQueryDescPrefix() << queryDesc << ", unrecoverable.");
                    }
                }
                else
                {
                    EA_ASSERT(mState == STATE_CONNECTED);
                    // Recoverable mysql error e.g.: ER_DUP_ENTRY
                    BLAZE_TRACE_LOG(Log::DB, callerDesc << "cont FAILED; connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), blaze_error=" << rc.c_str() << ", blaze_errno=" << rc.code << ", " << getQueryDescPrefix() << queryDesc << ", recoverable.");
                }
            }
        }
        else
        {
            // Handle blaze error, ERR_TIMEOUT, ERR_DISCONNECT, ERR_CANCELED, etc
            BLAZE_ERR_LOG(Log::DB, callerDesc << "cont FAILED; connection(" << getName() << "), thread_id(" << mMySqlThreadId << "), blaze_error=" << rc.c_str() << ", blaze_errno=" << rc.code << ", " << getQueryDescPrefix() << queryDesc << ", unrecoverable.");
            EA_ASSERT_MSG(mMySql.net.pvio == nullptr, "MySQL network io state has not been cleared!");
            mChannelHandle = INVALID_SOCKET; // clear the channel handle (because MYSQL_WAIT_TIMEOUT above already closed the socket) to avoid attempting to remove it in disconnect() after it may have alreadly been reused elsewhere
        }
    }

    mPendingCommand = NO_COMMAND;

    return rc;
}

void MySqlAsyncConn::onRead(Channel& channel)
{
    EA_ASSERT(&channel == this);
    mPollFlags |= MYSQL_WAIT_READ;
    if (mPollEvent.isValid())
        Fiber::signal(mPollEvent, ERR_OK);
}

void MySqlAsyncConn::onWrite(Channel& channel)
{
    EA_ASSERT(&channel == this);
    mPollFlags |= MYSQL_WAIT_WRITE;
    if (mPollEvent.isValid())
        Fiber::signal(mPollEvent, ERR_OK);
}

void MySqlAsyncConn::onError(Channel& channel)
{
    EA_ASSERT(&channel == this);
    if (mPollEvent.isValid())
    {
        Fiber::signal(mPollEvent, ERR_SYSTEM);
    }
    else
    {
        // NOTE: Immediately switch to a blocking fiber that will handle a potentially blocking disconnect() 
        // because onError() is called by the selector on the main fiber that cannot block.
        gSelector->scheduleFiberCall(this, &MySqlAsyncConn::disconnect, ERR_SYSTEM, "MySqlAsyncConn::disconnect");
    }
}

void MySqlAsyncConn::onClose(Channel& channel)
{
    EA_ASSERT(&channel == this);
    if (mPollEvent.isValid())
    {
        Fiber::signal(mPollEvent, DB_ERR_DISCONNECTED);
    }
    else
    {
        // NOTE: Immediately switch to a blocking fiber that will handle a potentially blocking disconnect() 
        // because onClose() is called by the selector on the main fiber that cannot block.
        gSelector->scheduleFiberCall(this, &MySqlAsyncConn::disconnect, DB_ERR_DISCONNECTED, "MySqlAsyncConn::disconnect");
    }
}

ChannelHandle MySqlAsyncConn::getHandle()
{
    return mChannelHandle;
}

BlazeRpcError MySqlAsyncConn::ping()
{
    BlazeError rc = ERR_OK;

    if (!isConnected())
        rc = connect();
    
    if (rc != ERR_OK)
        return rc;

    rc = executeWithRetry([this]() {
        int32_t ret = 0;
        auto duration = mOwner.getDbInstancePool().getDbConfig().getMaxConnPingtime();
        auto timeout = TimeValue::getTimeOfDay() + duration;
        return performBlockingOp("MySqlAsyncConn::ping - mysql_ping_", "PING", timeout,
            [this, &ret]() {
                return mysql_ping_start(&ret, &mMySql);
            },
            [this, &ret](uint32_t flags) {
                return mysql_ping_cont(&ret, &mMySql, flags);
        });
    });

    mUseCount = 0; // reset use count because admin conn does not get assign() called on it

    return rc;
}

Blaze::BlazeRpcError MySqlAsyncConn::kill(uint64_t threadId)
{
    BlazeError rc = ERR_OK;

    if (!isConnected())
        rc = connect();

    if (rc != ERR_OK)
        return rc;

    auto* killQuery = newQuery(__FILEANDLINE__);
    *killQuery << "KILL CONNECTION " << threadId;
    auto duration = mOwner.getDbInstancePool().getDbConfig().getMaxConnKilltime();
    auto timeout = TimeValue::getTimeOfDay() + duration;
    rc = executeQuery(killQuery, nullptr, timeout);
    delete killQuery;
    
    mUseCount = 0; // reset use count because admin conn does not get assign() called on it

    return rc;
}

void MySqlAsyncConn::scheduleKill()
{
    // Don't schedule a KILL CONNECTION query on an admin conn, since admin conns are responsible for killing all stalled
    // async connections in their DbInstancePool.
    // (Note that admin conns are only used for pings and KILL CONNECTION queries)
    if (getOwner().getConnNum() == DbConnBase::DB_ADMIN_CONN_ID)
        return;

    if (mScheduledKill)
    {
        BLAZE_WARN_LOG(Log::DB, "[MySqlAsyncConn].scheduleKill: Already scheduled kill for thread_id " << mMySqlThreadId);
        return;
    }
    mScheduledKill = true;

    static_cast<MySqlAsyncAdmin&>(getOwner().getDbInstancePool().getDbAdmin()).killConnection(mMySqlThreadId);

}

} // Blaze

