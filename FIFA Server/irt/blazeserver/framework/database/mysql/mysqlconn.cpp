/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class MySqlConn
    This is the wrapper class for the database connection from the driver. Currently
    the implementation is specific to the warapper provided by the mysql++.
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/mysql/mysqlconn.h"
#include "framework/database/mysql/mysqlresult.h"
#include "framework/database/mysql/mysqlpreparedstatement.h"
#include "framework/database/mysql/mysqlquery.h"
#include "framework/database/dbconnpool.h"
#include <mariadb/mysqld_error.h>
#include <mariadb/errmsg.h>

#include "framework/database/dbscheduler.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

BlazeRpcError MySqlConn::connectImpl()
{
    const DbConfig& config = mOwner.getDbInstancePool().getDbConfig();

    BLAZE_TRACE_LOG(Log::DB, "[MySqlConn].connect: conn=" << mOwner.getName() << "; Connecting to database: " << config.getDatabase() << " host: " << config.getHost() 
                    << " user: " << config.getUser() << " port: " << config.getPort());
    //Not supporting the socketname and client flag yet...

    //check if someone wants to re-establish the connection to the
    //database on this object.
    if(isConnected())
    {
        disconnectImpl(DB_ERR_OK);
    }

    // need to re-initialize mMySql state
    initialize();

    BlazeRpcError error = connectCommon(mMySql, config, "connect");
    if (error == DB_ERR_OK)
    {
        if (mysql_autocommit(&mMySql, 1) != 0)
        {
            uint32_t myError = mysql_errno(&mMySql);
            mLastErrorText = mysql_error(&mMySql);
            error = DbConnBase::convertErrorCode(myError);

            BLAZE_ERR_LOG(Log::DB, "[MySqlConn].connect: conn=" << mOwner.getName() << "; failed to enable autocommit: ERROR " << myError 
                            << " (" << mysql_sqlstate(&mMySql) << ") " << mLastErrorText << ", db: " << config.getDatabase() << ", host: " << config.getHost() << ", port: " << config.getPort());
        }
    }

    if (error == DB_ERR_OK)
    {
        mConnected = true;
    }
    else
    {
        disconnectImpl(DB_ERR_OK);
        mConnected = false; 
    }

    return error;
}

BlazeRpcError MySqlConn::testConnectImpl(const DbConfig& config)
{
    BLAZE_TRACE_LOG(Log::DB, "[MySqlConn].testConnect: conn=" << mOwner.getName() << "; Connecting to database: " << config.getDatabase() << " host: " << config.getHost() 
                    << " user: " << config.getUser() << " port: " << config.getPort());

    MYSQL sql;
    initializeCommon(sql, config);
    BlazeRpcError error = connectCommon(sql, config, "testConnect");
    mysql_close(&sql);
    return error;
}

BlazeRpcError MySqlConn::connectCommon(MYSQL& sql, const DbConfig& config, const char8_t* caller)
{
    BlazeRpcError error = DB_ERR_OK;

    char8_t ipAddrBuf[128];
    if (blaze_strcmp("localhost", config.getHost()) == 0)
    {
        blaze_snzprintf(ipAddrBuf, sizeof(ipAddrBuf), "localhost");
    }
    else
    {
        InetAddress ipAddr;
        ipAddr.setHostname(config.getHost());
        // Pre-resolve the IP address. Useful when troubleshooting name server issues obfuscated by MYSQL.
        bool result = NameResolver::blockingResolve(ipAddr);
        if (result)
        {
            blaze_snzprintf(ipAddrBuf, sizeof(ipAddrBuf), ipAddr.getIpAsString());
            BLAZE_TRACE_LOG(Log::DB, "[MySqlConn]." << caller << ": conn=" 
                << mOwner.getName() << "; successfully resolved host: " << config.getHost() << " ip: " << ipAddrBuf << " for db: " << config.getDatabase());
        }
        else
        {
            BLAZE_ERR_LOG(Log::DB, "[MySqlConn]." << caller << ": conn=" 
                << mOwner.getName() << "; failed to resolve host: " << config.getHost() << " for db: " << config.getDatabase());
            return ERR_SYSTEM;
        }
    }

    // Init the connection handler.
    if (mysql_real_connect(&sql, ipAddrBuf, config.getUser(), config.getPassword(),
                config.getDatabase(), config.getPort(), nullptr, CLIENT_MULTI_STATEMENTS))
    {
        BLAZE_TRACE_LOG(Log::DB, "[MySqlConn]." << caller << ": conn=" << mOwner.getName() << "; Created a database connection!");
        //select the database
        if (mysql_select_db(&sql, config.getDatabase()))
        {
            //if we are not able to select the database its as good as no connection
            //so we return as not connected and disconnect the existing connection.
            uint32_t myError = mysql_errno(&sql);
            mLastErrorText = mysql_error(&sql);
            error = DbConnBase::convertErrorCode(myError);

            BLAZE_ERR_LOG(Log::DB, "[MySqlConn]." << caller << ": conn=" << mOwner.getName() << "; There was an error selecting database: ERROR " << myError
                          << " (" << mysql_sqlstate(&sql) << "): " << mLastErrorText << ", db: " << config.getDatabase() << " host: " << config.getHost());
        }
    }
    else
    {
        uint32_t myError = mysql_errno(&sql);
        mLastErrorText = mysql_error(&sql);
        error = DbConnBase::convertErrorCode(myError);

        BLAZE_ERR_LOG(Log::DB, "[MySqlConn]." << caller << ": conn=" << mOwner.getName() << "; mysql_real_connect failed: ERROR " << myError << " ("
                      << mysql_sqlstate(&sql) << "): " << mLastErrorText << ", db: " << config.getDatabase() << " host: " << config.getHost());
    }

    return error;
}

/*************************************************************************************************/
/*!
    \brief <error>

    Returns the error string associated with this connection if any. If there are no
    error the return value will be nullptr.


    \return - Returns the error string.

*/
/*************************************************************************************************/
const char* MySqlConn::getErrorString()
{
    return mysql_error(&mMySql);
}

void MySqlConn::disconnectImpl(BlazeRpcError reason)
{
    if (!mInitialized)
        return;

    if (isConnected())
    {
        BLAZE_TRACE_LOG(Log::DB, "[MySqlConn].disconnectImpl: conn=" << mOwner.getName() << "; disconnecting from server.");
        if (reason != DB_ERR_OK)
        {
            const DbConfig& config = mOwner.getDbInstancePool().getDbConfig();
            const uint32_t mysqlErr = mysql_errno(&mMySql);
            if (mysqlErr != 0)
            {
                mLastErrorText = mysql_error(&mMySql);
                BLAZE_ERR_LOG(Log::DB, "[MySqlConn].disconnectImpl: conn=" 
                    << mOwner.getName() << "; closing reason: " << ErrorHelp::getErrorName(reason)
                    << ", mysql state: " << mysqlErr << " (" << mysql_sqlstate(&mMySql) << "): " << mLastErrorText
                    << ", db: " << config.getDatabase() << " host: " << config.getHost());
            }
            else
            {
                BLAZE_ERR_LOG(Log::DB, "[MySqlConn].disconnectImpl: conn=" 
                    << mOwner.getName() << "; closing reason: " << ErrorHelp::getErrorName(reason) 
                    << ", db: " << config.getDatabase() << " host: " << config.getHost());
            }
        }
    }

    freeStreamingResultsImpl();
    closeAllStatements();
    mysql_close(&mMySql);
    mInitialized = false;
    mConnected = false;
}

BlazeRpcError MySqlConn::freeStreamingResultsImpl()
{    
    if (!mStreamingResults.empty())
    {
        for (MySqlResList::iterator itr = mStreamingResults.begin(), end = mStreamingResults.end(); 
            itr != end; ++itr)
        {
            MYSQL_RES* res = *itr;
            while (mysql_fetch_row(res) != nullptr) {}
            mysql_free_result(res);
        }

        mStreamingResults.clear();
    }
    return ERR_OK;
}

BlazeRpcError MySqlConn::fetchNextStreamingResultRowsImpl()
{
    size_t rowCount = 0;
    MYSQL_RES *res = nullptr;
    lockState();
        if (mCurrentStreamingDbResult != nullptr)
        {
            //res can't go away while we're operating on it, because its released by this conn
            //after the fact.
            res = ((MySqlStreamingResult*)mCurrentStreamingDbResult)->getMySqlRes();     
            rowCount = ((MySqlStreamingResult*)mCurrentStreamingDbResult)->getPrefetchRowCount();
        }
    unlockState();

    if (res != nullptr)
    {
        for (uint32_t i = 0; i < rowCount; ++i)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row == nullptr)
            {
                break;         
            }
            
            //See if the result is still with us, if so, push the row on the back.
            lockState();
            if (mCurrentStreamingDbResult != nullptr)
            {        
                ((MySqlStreamingResult*)mCurrentStreamingDbResult)->addCacheRow(row);
            }
            unlockState();
            
        }
    }

    return ERR_OK;
}

BlazeRpcError MySqlConn::executeQueryImpl(const char8_t* query, DbResult** result)
{
    return executeQueryImplCommon(query, false, result, nullptr);
}

BlazeRpcError MySqlConn::executeQueryImplCommon(const char8_t* query, bool isStreaming, DbResult** result, StreamingDbResult** streamingResult)
{
    BlazeRpcError rc;

    BLAZE_TRACE_LOG(Log::DB, "[MySqlConn].executeQueryImpl: conn=" << mOwner.getName() << "; Executing query: " << query );

    if (result != nullptr)
        *result = nullptr;
    if (streamingResult != nullptr)
        *streamingResult = nullptr;

    if(mysql_real_query(&mMySql, query, (uint32_t)eastl::CharStrlen(query)) == 0)
    {
        if (((isStreaming && (streamingResult != nullptr)) || (!isStreaming && (result != nullptr))) && !queryTimedOut())
        {
            // The caller only wants the MySqlResult if pullResult() is successful, 
            // but we always have to allocate it here to handle resource allocation / freeing properly.
            MySqlResultBase* mySqlResult = nullptr;
            if (isStreaming)
            {
                mySqlResult = BLAZE_NEW MySqlStreamingResult(*this);
            }
            else
            {
                mySqlResult = BLAZE_NEW MySqlResult(*this);
            }
            rc = pullResult(query, *mySqlResult, isStreaming);
            if (rc == DB_ERR_OK)
            {
                if (!isStreaming && result != nullptr)
                {
                    *result = static_cast<MySqlResult*>(mySqlResult);
                }
                else if (isStreaming && streamingResult != nullptr)
                {
                    *streamingResult = static_cast<MySqlStreamingResult*>(mySqlResult);
                }
            }
            else
            {
                // pullResult() will always pull and free all MYSQL_RES objects from the
                // connection in case of error; therefore, no need to free results here.
                if(rc != DB_ERR_DISCONNECTED)
                {
                    mLastErrorText = mysql_error(&mMySql);
                    BLAZE_ERR_LOG(Log::DB, "[MySqlConn].executeQueryImpl: conn=" << mOwner.getName() << "; Failed building resultset for query -> " 
                                  << query << " <-: " << mLastErrorText << " (" << mysql_errno(&mMySql) << ")");
                }
                delete mySqlResult;
            }
        }
        else
        {
            rc = freeResults();
            if (queryTimedOut())
            {
                // if the query timed out, we must call disconnectImpl() manually,
                // because we'll be overriding the DB_ERR_DISCONNECTED return code,
                // which means that the calling EXECUTE_WITH_RECONNECT_RETRY()
                // macro will not know to call disconnectImpl() for us!
                disconnectImpl(rc);
                rc = DB_ERR_TIMEOUT;
            }
            else if(isLoggable(rc))
            {
                mLastErrorText = mysql_error(&mMySql);
                BLAZE_ERR_LOG(Log::DB, "[MySqlConn].executeQueryImpl: conn=" << mOwner.getName() << "; Error freeing results for query -> " 
                              << query << " <-: " << mLastErrorText << " (" << mysql_errno(&mMySql) << ")");
            }
        }
    }
    else
    {
        uint32_t myError = mysql_errno(&mMySql);
        rc = DbConnBase::convertErrorCode(myError);
        if (isLoggable(rc))
        {
            mLastErrorText = mysql_error(&mMySql);
            if (myError == CR_SERVER_GONE_ERROR && getUseCount() == 0)
            {
                BLAZE_TRACE_LOG( Log::DB, "[MySqlConn].executeQueryImpl: conn=" << mOwner.getName() << "; Error executing the query -> " 
                                 << query << " <-: " << mLastErrorText << " (" << myError << ") - reconnecting and restarting operation");

                if (rc == DB_ERR_DISCONNECTED)
                {
                   disconnectImpl(DB_ERR_OK);
                }
            }
            else
            {
                BLAZE_WARN_LOG(Log::DB, "[MySqlConn].executeQueryImpl: conn=" << mOwner.getName() << "; Unexpected error executing the query -> " 
                               << query << " <-: " << mLastErrorText << " (" << myError << ")");

                if (rc == DB_ERR_DISCONNECTED)
                {
                   disconnectImpl(rc);
                }
            }
        }
    }

    return rc;
}

BlazeRpcError MySqlConn::executeStreamingQueryImpl(const char8_t* query, StreamingDbResult** result)
{
    return executeQueryImplCommon(query, true, nullptr, result);
}

void MySqlConn::initialize()
{
    // Either the connection is not yet initialized, or it is freshly disconnected
    if (mInitialized)
        return;

    mInitialized = true;

    initializeCommon(mMySql, mOwner.getDbInstancePool().getDbConfig());
}

void MySqlConn::initializeCommon(MYSQL& sql, const DbConfig& config) const
{
    mysql_init(&sql);

    if (config.useCompression())
        mysql_options(&sql, MYSQL_OPT_COMPRESS, 0);

    const char8_t* charset = config.getCharset();
    if ((charset != nullptr) && (charset[0] != '\0'))
        mysql_options(&sql, MYSQL_SET_CHARSET_NAME, charset);

    mysql_options(&sql, MYSQL_OPT_CONNECT_TIMEOUT, &CONNECT_TIMEOUT);
}

BlazeRpcError MySqlConn::freeResults()
{
    int32_t status; // more results? -1=no, >0=error, 0=yes
    // even though multiple results may not be expected, we must
    // free all pending results and return the connection to a
    // valid state so that it can be used by subsequent queries
    do
    {
        // we don't use mysql_store_result() because mysql_use_result()
        // is more efficient since we do not want the rows anyway
        MYSQL_RES* res = mysql_use_result(&mMySql);
        if(res != nullptr)
            mysql_free_result(res);
    }
    while((status = mysql_next_result(&mMySql)) == 0);

    BlazeRpcError rc = DB_ERR_OK;
    if (status >= 0)
    {
        rc = DbConnBase::convertErrorCode(mysql_errno(&mMySql));
        if (isLoggable(rc))
        {
            mLastErrorText = mysql_error(&mMySql);
            BLAZE_WARN_LOG(Log::DB, "[MySqlConn].freeResults: conn=" << mOwner.getName() << "; Unexpected error(" << mLastErrorText
                << ") while freeing results.");
        }
    }

    return rc;
}

BlazeRpcError MySqlConn::pullResult(const char8_t* query, MySqlResultBase& result, bool isStreaming)
{
    BlazeRpcError rc;
    
    MYSQL_RES* res = nullptr;
    if (isStreaming)
    {
        res = mysql_use_result(&mMySql);
        
        //We store this result to ensure we clear it out later see freeStreamingResultsImpl
        mStreamingResults.push_back(res);
    }
    else
    {
        res = mysql_store_result(&mMySql);
    }
    // Even when mysql_store_result() returns nullptr, the caller is expecting us 
    // to give back a MySqlResult object containing AffectedRowCount and LastInsertId,
    // MySqlResult::init() method handles nullptr MYSQL_RES* without issue.
    // IMPORTANT: We MUST call MySqlResult::init() before calling mysql_next_result()
    // which will advance the state of the connection where we can no longer obtain
    // AffectedRowCount and LastInsertId!
    rc = result.init(res);
    if (rc == DB_ERR_OK)
    {
        if (!isStreaming)
        {
            int32_t numResults = 1;
            int32_t status; // more results? -1=no, >0=error, 0=yes
            // even though multiple results are not expected, we must
            // free all pending results and return the connection to a
            // valid state so that it can be used by subsequent queries
            while ((status = mysql_next_result(&mMySql)) == 0)
            {
                // we don't use mysql_store_result() because mysql_use_result()
                // is more efficient since we do not want the rows anyway
                res = mysql_use_result(&mMySql);
                if (res != nullptr)
                    mysql_free_result(res);
                ++numResults;
            }
            if (numResults > 1)
            {
                BLAZE_WARN_LOG(Log::DB,  "[MySqlConn].pullResult: conn=" << mOwner.getName() << "; Unexpected multiple(" << numResults 
                               << ") results from single query -> " << query << " <-: (use DbConn::executeMultiQuery() instead)");
            }
            if (status > 0)
            {
                rc = DbConnBase::convertErrorCode(mysql_errno(&mMySql));
                if (isLoggable(rc))
                {
                    mLastErrorText = mysql_error(&mMySql);
                    BLAZE_WARN_LOG(Log::DB,  "[MySqlConn].pullResult: conn=" << mOwner.getName() << "; Unexpected error(" << mLastErrorText
                            << ") pulling result.");
                }
            }
        }
    }
    else
    {
        rc = DbConnBase::convertErrorCode(mysql_errno(&mMySql));
        if (isLoggable(rc))
        {
            mLastErrorText = mysql_error(&mMySql);
            BLAZE_WARN_LOG(Log::DB,  "[MySqlConn].pullResult: conn=" << mOwner.getName() << "; Unexpected error(" << mLastErrorText
                    << ") initializing result.");
        }
    }
    return rc;
}

BlazeRpcError MySqlConn::executeMultiQueryImpl(const char8_t* query, DbResults** results)
{
    BlazeRpcError error = DB_ERR_OK;

    if (results != nullptr)
        *results = nullptr;

    BLAZE_TRACE_LOG(Log::DB, "[MySqlConn].executeMultiQueryImpl: conn=" << mOwner.getName() << "; Executing query: """ << query <<"""[eoq]");
    if (mysql_real_query(&mMySql, query, (uint32_t)eastl::CharStrlen(query)) != 0)
    {
        //Error handling..
        uint32_t errcode = mysql_errno(&mMySql);
        error = DbConnBase::convertErrorCode(errcode);
        if (isLoggable(error))
        {
            mLastErrorText = mysql_error(&mMySql);
            if (errcode == CR_SERVER_GONE_ERROR && getUseCount() == 0)
            {
                BLAZE_TRACE_LOG(Log::DB, "[MySqlConn].executeMultiQueryImpl: conn=" << mOwner.getName() << "; Error executing the query -> " 
                                << query << " <-: " << mLastErrorText << " (" << errcode << ") - reconnecting and restarting operation");
            }
            else
            {
                BLAZE_WARN_LOG(Log::DB, "[MySqlConn].executeMultiQueryImpl: conn=" << mOwner.getName() << "; Unexpected Error executing the query -> " 
                               << query << " <-: " << mLastErrorText << " (" << errcode << ")");
            }

            if (error == DB_ERR_DISCONNECTED)
            {
                disconnectImpl(error);
            }
        }
        return error;
    }

    DbResults *mySqlResults = nullptr;
    if (error == DB_ERR_OK)
    {
        mySqlResults = BLAZE_NEW DbResults();
        int status =1;
        do
        {
            //Check if all results needs to be fetched.
            MYSQL_RES* res = mysql_store_result( &mMySql );
            MySqlResult* result = BLAZE_NEW MySqlResult(*this);
            error = result->init(res);
            if (error != DB_ERR_OK)
            {
                if (error == DB_ERR_DISCONNECTED)
                    disconnectImpl(error);
                delete result;
                break;
            }
            mySqlResults->push_back(result);

            //more results? -1=no, >0=error, 0=yes
            if ((status = mysql_next_result(&mMySql)) > 0)
            {
                uint32_t myError = mysql_errno(&mMySql);
                error = DbConnBase::convertErrorCode(myError);
                if (isLoggable(error))
                {
                    const DbConfig& config = mOwner.getDbInstancePool().getDbConfig();
                    mLastErrorText = mysql_error(&mMySql);
                    BLAZE_ERR_LOG(Log::DB, "[MySqlConn].executeMultiQueryImpl: conn=" << mOwner.getName() << "; Unexpected error getting the next result: ERROR " 
                                  << myError << " (" << mysql_sqlstate(&mMySql) << "): " << mLastErrorText << ", db: " << config.getDatabase() << " host: " << config.getHost());
                }
                break;
            }
        } while(status == 0);

        // Need to check this flag for a timeout since killing a dead query causes mysql_real_query
        // to return success rather than failure even though the query didn't complete
        if (queryTimedOut() && (error == DB_ERR_OK))
        {
            error = DB_ERR_TIMEOUT;
        }

        if (error != DB_ERR_OK)
        {
            delete mySqlResults;
            mySqlResults = nullptr;
        }
    }

    if (error == DB_ERR_DISCONNECTED)
    {
        disconnectImpl(error);
    }

    if (results != nullptr)
    {
        *results = mySqlResults;
    }
    else if (mySqlResults != nullptr)
    {
        //If the user didn't want results and we have them, delete the results
        delete mySqlResults;
    }

    return error;
}

BlazeRpcError MySqlConn::executePreparedStatementImpl(PreparedStatement& statement, DbResult** result)
{
    BlazeRpcError error = statement.execute(result, *this);
    if (error == DB_ERR_DISCONNECTED)
    {
        if (getUseCount() == 0)
            disconnectImpl(DB_ERR_OK);
        else
            disconnectImpl(error);
    }
    
    return error;
}

/*************************************************************************************************/
/*!
    \brief <startTxn>

    Starts the database transaction.

    \return - Returns false if there was an issue with the initiating the transaction.
    code should handle this situation to not further go ahead with requests.

*/
/*************************************************************************************************/

BlazeRpcError MySqlConn::startTxnImpl()
{
    // Begin the transaction set by turning off autocommit.
    if(!mysql_autocommit(&mMySql,0))
        return DB_ERR_OK;

    uint32_t myError = mysql_errno(&mMySql);

    BlazeRpcError error = DbConnBase::convertErrorCode(myError);
    if (isLoggable(error))
    {
        mLastErrorText = mysql_error(&mMySql);
        const DbConfig& config = mOwner.getDbInstancePool().getDbConfig();
        if (myError == CR_SERVER_GONE_ERROR && getUseCount() == 0)
        {
            BLAZE_TRACE_LOG(Log::DB, "[MySqlConn].startTxnImpl: conn=" << mOwner.getName() << "; Error starting the transaction: ERROR " << myError 
                            << " (" << mysql_sqlstate(&mMySql) << "): " << mLastErrorText << ", db: " << config.getDatabase() << ", host: " << config.getHost() 
                            << " - reconnecting and restarting transaction.");

            if (error == DB_ERR_DISCONNECTED)
                disconnectImpl(DB_ERR_OK);
        }
        else
        {
            BLAZE_ERR_LOG(Log::DB, "[MySqlConn].startTxnImpl: conn=" << mOwner.getName() << "; Error starting the transaction: ERROR " << myError 
                          << " (" << mysql_sqlstate(&mMySql) << "): " << mLastErrorText << ", db: " << config.getDatabase() << ", host: " << config.getHost());

            if (error == DB_ERR_DISCONNECTED)
                disconnectImpl(error);
        }
    }

    return error;
}

/*************************************************************************************************/
/*!
    \brief <commit>

    commits the database transaction.

    \return - Returns false if there was an issue with the commiting the transaction.
    code should handle this situation to not further go ahead with requests.

*/
/*************************************************************************************************/

BlazeRpcError MySqlConn::commitImpl()
{
    // Commit the transaction
    const char8_t* func = "mysql_commit";
    if (mysql_commit(&mMySql) == 0)
    {
        // Re-enable autocommit now that the transaction is complete
        func = "mysql_autocommit";
        if (mysql_autocommit(&mMySql, 1) == 0)
            return DB_ERR_OK;
    }

    uint32_t myError = mysql_errno(&mMySql);

    BlazeRpcError error = DbConnBase::convertErrorCode(myError);
    if (isLoggable(error))
    {
        const DbConfig& config = mOwner.getDbInstancePool().getDbConfig();
        mLastErrorText = mysql_error(&mMySql);
        BLAZE_ERR_LOG(Log::DB, "[MySqlConn].commitImpl: conn=" << mOwner.getName() << "; Error commiting (" << func << ") the transaction: ERROR " << myError 
                      << " (" << mysql_sqlstate(&mMySql) << "): " << mLastErrorText << ", db: " << config.getDatabase() << " host: " << config.getHost());

        if (error == DB_ERR_DISCONNECTED)
            disconnectImpl(error);
    }

    return error;
}

/*************************************************************************************************/
/*!
    \brief <rollback>

    Rollbacks the database transaction.

    \return - Returns false if there was an issue with the rollback the transaction.
    code should handle this situation to not further go ahead with requests.

*/
/*************************************************************************************************/

BlazeRpcError MySqlConn::rollbackImpl()
{
    // Rollback the transaction
    const char8_t* func = "mysql_rollback";
    if (mysql_rollback(&mMySql) == 0)
    {
        // Re-enable autocommit now that the transaction is complete
        func = "mysql_autocommit";
        if (mysql_autocommit(&mMySql, 1) == 0)
            return DB_ERR_OK;
    }

    uint32_t myError = mysql_errno(&mMySql);
    BlazeRpcError error = DbConnBase::convertErrorCode(myError);
    if (isLoggable(error))
    {
        const DbConfig& config = mOwner.getDbInstancePool().getDbConfig();
        mLastErrorText = mysql_error(&mMySql);
        BLAZE_ERR_LOG(Log::DB, "[MySqlConn].rollbackImpl: conn=" << mOwner.getName() << "; Error commiting (" << func << ") the transaction: ERROR " 
                      << myError << " (" << mysql_sqlstate(&mMySql) << "): " << mLastErrorText << ", db: " << config.getDatabase() << " host: " << config.getHost());

        if (error == DB_ERR_DISCONNECTED)
            disconnectImpl(error);
    }

    return error;
}

MySqlConn::MySqlConn(const DbConfig& conf, DbConnBase& owner)
    : ThreadedDbConn(conf, owner), mInitialized(false)
{
    memset(&mMySql, 0, sizeof(mMySql));
}

MySqlConn::~MySqlConn()
{
    disconnectImpl(DB_ERR_OK);
}

PreparedStatement* MySqlConn::createPreparedStatement(const PreparedStatementInfo& info)
{
    return BLAZE_NEW MySqlPreparedStatement(info);
}

bool MySqlConn::isLoggable(BlazeRpcError error)
{
    switch (error)
    {
        case DB_ERR_OK:
            return false;
        case DB_ERR_DUP_ENTRY:
            return false;
        default: 
            return true;
    }
}

Query* MySqlConn::newQuery(const char8_t* fileAndLine)
{
    initialize(); // need to initialize mMySql state, to make use of mysql_real_escape_string()
    return BLAZE_NEW MySqlQuery(*getMySql(), fileAndLine);
}

void MySqlConn::releaseQuery(Query* query) const
{
    delete static_cast<MySqlQuery*>(query);
}

BlazeRpcError MySqlConn::ping()
{
    BlazeRpcError error = DB_ERR_OK;

    if (!isConnected())
    {
        error = connectImpl();
    }

    if (error == DB_ERR_OK)
    {
        int32_t myErr = mysql_ping(&mMySql);
        if (myErr != 0)
            error = DbConnBase::convertErrorCode(myErr);
    }

    if (error == DB_ERR_DISCONNECTED)
    {
        disconnectImpl(error);
    }
    return error;
}

}// Blaze
