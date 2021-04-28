/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"
#include "framework/database/mysql/mysqlresult.h"
#include "framework/database/mysql/mysqlpreparedstatement.h"
#include "framework/database/mysql/mysqlpreparedstatementresult.h"
#include "framework/database/mysql/mysqlconn.h"
#include "framework/database/mysql/mysqlasyncconn.h"
#include "framework/database/dbconfig.h"
#include "framework/database/dbconnpool.h"
#include <mariadb/errmsg.h>

//lint -esym(613, Blaze::MySqlPreparedStatement::mBindings)


namespace Blaze
{

MySqlPreparedStatement::MySqlPreparedStatement(const PreparedStatementInfo& info)
    : PreparedStatement(info),
      mStatement(nullptr),
      mBindings(nullptr),
      mResultBindings(nullptr)
{
}


MySqlPreparedStatement::~MySqlPreparedStatement()
{
    // Every MySqlPreparedStatement is registered with a DbConnInterface,
    // and is only deleted once the DbConnInterface is destroyed.
    // Since DbConnInterfaces always disconnect before they're destroyed, 
    // and always close all registered statements on disconnect,
    // mStatement should always be nullptr at this point.
    EA_ASSERT(mStatement == nullptr);

    clearAllBindings();
}

void MySqlPreparedStatement::close(DbConnInterface& conn)
{
    conn.closeStatement(mStatement);
    mStatement = nullptr;
    clearAllBindings();
}

void MySqlPreparedStatement::setupBindParams(size_t paramCount)
{
    if (mBindings == nullptr)
    {
        mBindings = BLAZE_NEW_ARRAY(MYSQL_BIND, paramCount);
        memset(mBindings, 0, sizeof(*mBindings) * paramCount);
    }

    ParamList::iterator itr = mParams.begin();
    ParamList::iterator end = mParams.end();
    for (size_t index = 0; (itr != end) && (index < paramCount); ++itr, ++index)
    {
        ParamData& data = *itr;
        switch (data.mType)
        {
        case ParamData::TYPE_NONE:
            EA_FAIL_MSG("Parameter not set for prepared statement.");
            break;

        case ParamData::TYPE_INT8:
            mBindings[index].is_unsigned = 0;
            mBindings[index].buffer_type = MYSQL_TYPE_TINY;
            mBindings[index].buffer = (char*)&data.mInt8;
            mBindings[index].is_null = 0;
            mBindings[index].length = 0;
            break;

        case ParamData::TYPE_INT16:
            mBindings[index].is_unsigned = 0;
            mBindings[index].buffer_type = MYSQL_TYPE_SHORT;
            mBindings[index].buffer = (char*)&data.mInt16;
            mBindings[index].is_null = 0;
            mBindings[index].length = 0;
            break;

        case ParamData::TYPE_INT32:
            mBindings[index].is_unsigned = 0;
            mBindings[index].buffer_type = MYSQL_TYPE_LONG;
            mBindings[index].buffer = (char*)&data.mInt32;
            mBindings[index].is_null = 0;
            mBindings[index].length = 0;
            break;

        case ParamData::TYPE_INT64:
            mBindings[index].is_unsigned = 0;
            mBindings[index].buffer_type = MYSQL_TYPE_LONGLONG;
            mBindings[index].buffer = (char*)&data.mInt64;
            mBindings[index].is_null = 0;
            mBindings[index].length = 0;
            break;

        case ParamData::TYPE_UINT8:
            mBindings[index].is_unsigned = 1;
            mBindings[index].buffer_type = MYSQL_TYPE_TINY;
            mBindings[index].buffer = (char*)&data.mUInt8;
            mBindings[index].is_null = 0;
            mBindings[index].length = 0;
            break;

        case ParamData::TYPE_UINT16:
            mBindings[index].is_unsigned = 1;
            mBindings[index].buffer_type = MYSQL_TYPE_SHORT;
            mBindings[index].buffer = (char*)&data.mUInt16;
            mBindings[index].is_null = 0;
            mBindings[index].length = 0;
            break;

        case ParamData::TYPE_UINT32:
            mBindings[index].is_unsigned = 1;
            mBindings[index].buffer_type = MYSQL_TYPE_LONG;
            mBindings[index].buffer = (char*)&data.mUInt32;
            mBindings[index].is_null = 0;
            mBindings[index].length = 0;
            break;

        case ParamData::TYPE_UINT64:
            mBindings[index].is_unsigned = 1;
            mBindings[index].buffer_type = MYSQL_TYPE_LONGLONG;
            mBindings[index].buffer = (char*)&data.mUInt64;
            mBindings[index].is_null = 0;
            mBindings[index].length = 0;
            break;

        case ParamData::TYPE_STRING:
            mBindings[index].buffer_type = MYSQL_TYPE_VAR_STRING;
            if (data.mString == nullptr)
            {
                mBindings[index].buffer = nullptr;
                mBindings[index].buffer_length = 0;
                mBindings[index].is_null = &data.mIsNull;
                mBindings[index].length = 0;
            }
            else
            {
                mBindings[index].buffer = const_cast<char8_t*>(data.mString);
                mBindings[index].buffer_length = (uint32_t)strlen(data.mString);
                mBindings[index].is_null = 0;
                mBindings[index].length = &mBindings[index].buffer_length; // TODO: is this safe?
            }
            break;

        case ParamData::TYPE_DOUBLE:
            mBindings[index].buffer_type = MYSQL_TYPE_DOUBLE;
            mBindings[index].buffer = (char*)&data.mDouble;
            mBindings[index].is_null = 0;
            mBindings[index].length = 0;
            break;

        case ParamData::TYPE_FLOAT:
            mBindings[index].buffer_type = MYSQL_TYPE_FLOAT;
            mBindings[index].buffer = (char*)&data.mFloat;
            mBindings[index].is_null = 0;
            mBindings[index].length = 0;
            break;

        case ParamData::TYPE_BINARY:
            mBindings[index].buffer_type = MYSQL_TYPE_VAR_STRING;
            if (data.mBinary.data == nullptr)
            {
                mBindings[index].buffer = nullptr;
                mBindings[index].buffer_length = 0;
                mBindings[index].is_null = &data.mIsNull;
                mBindings[index].length = 0;
            }
            else
            {
                mBindings[index].buffer
                    = const_cast<char8_t*>(reinterpret_cast<const char8_t*>(data.mBinary.data));
                mBindings[index].buffer_length = (uint32_t)data.mBinary.len;
                mBindings[index].is_null = 0;
                mBindings[index].length = &mBindings[index].buffer_length; // TODO: is this safe?
            }
            break;
        }
    }
}

void MySqlPreparedStatement::setupBindResults(MYSQL_FIELD* fields, uint32_t numFields)
{
    mResultBindings = BLAZE_NEW_ARRAY(MYSQL_BIND, numFields);
    memset(mResultBindings, 0, numFields * sizeof(*mResultBindings));

    mRowData.reserve(numFields);

    static const uint32_t DEFAULT_BLOB_ALLOC_SIZE = 2048;

    for (uint32_t idx = 0; idx < numFields; ++idx)
    {
        mRowData.push_back();
        MySqlPreparedStatementRow::ResultData& data = mRowData.back();

        data.mType = mResultBindings[idx].buffer_type = fields[idx].type;
        mResultBindings[idx].is_unsigned = (fields[idx].flags & UNSIGNED_FLAG) ? 1 : 0;
        mResultBindings[idx].is_null = &data.mIsNull;
        mResultBindings[idx].error = &data.mError;
        mResultBindings[idx].length = &data.mLength;

        switch (fields[idx].type)
        {
        case MYSQL_TYPE_TINY:
            mResultBindings[idx].buffer = &data.u.mInt8;
            break;
        case MYSQL_TYPE_SHORT:
            mResultBindings[idx].buffer = &data.u.mInt16;
            break;
        case MYSQL_TYPE_LONG:
            mResultBindings[idx].buffer = &data.u.mInt32;
            break;
        case MYSQL_TYPE_LONGLONG:
            mResultBindings[idx].buffer = &data.u.mInt64;
            break;
        case MYSQL_TYPE_VAR_STRING:
        case MYSQL_TYPE_STRING:
        case MYSQL_TYPE_VARCHAR:
            data.allocData(fields[idx].length);
            mResultBindings[idx].buffer = data.u.mData;
            mResultBindings[idx].buffer_length = fields[idx].length;
            break;
        case MYSQL_TYPE_DOUBLE:
            mResultBindings[idx].buffer = &data.u.mDouble;
            break;
        case MYSQL_TYPE_FLOAT:
            mResultBindings[idx].buffer = &data.u.mFloat;
            break;

        case MYSQL_TYPE_TINY_BLOB:
        case MYSQL_TYPE_MEDIUM_BLOB:
        case MYSQL_TYPE_LONG_BLOB:
        case MYSQL_TYPE_BLOB:
            data.allocData(DEFAULT_BLOB_ALLOC_SIZE);
            mResultBindings[idx].buffer = data.u.mData;
            mResultBindings[idx].buffer_length = DEFAULT_BLOB_ALLOC_SIZE;
            break;

        default:
            BLAZE_WARN_LOG(Log::DB, "[PreparedStatement].setupBindResults: Unhandled MySql data type(" << fields[idx].type << ") with field name(" << fields[idx].name << "), table(" << fields[idx].table << "), db(" << fields[idx].db << "); ignoring.");
            mResultBindings[idx].buffer_type = MYSQL_TYPE_NULL;
            break;
        }
    }
}

void MySqlPreparedStatement::clearResultBindings()
{
    if (mResultBindings != nullptr)
    {
        delete[] mResultBindings;
        mResultBindings = nullptr;
    }
    mRowData.clear();
}

void MySqlPreparedStatement::clearAllBindings()
{
    if (mBindings != nullptr)
    {
        delete[] mBindings;
        mBindings = nullptr;
    }
    clearResultBindings();
}

BlazeRpcError MySqlPreparedStatement::execute(DbResult** outResult, MySqlConn& conn)
{
    if (mStatement == nullptr)
    {
        mStatement = mysql_stmt_init(&conn.mMySql);
        if (mStatement == nullptr)
            return DB_ERR_SYSTEM;

        if (mysql_stmt_prepare(mStatement, mQuery, (uint32_t)strlen(mQuery)))
        {
            const DbConfig& dbConfig = conn.getOwner().getDbInstancePool().getDbConfig();
            uint32_t errCode = mysql_stmt_errno(mStatement);
            BlazeRpcError error = DbConnBase::convertErrorCode(errCode);

            if (errCode == CR_SERVER_GONE_ERROR && conn.getUseCount() == 0)
            {
                BLAZE_TRACE_LOG(Log::DB, "[MySqlPreparedStatement].execute: conn=" << conn.getName() << "; mysql_stmt_prepare() failed:  ERROR "
                    << errCode << " (" << mysql_sqlstate(&conn.mMySql) << "): " << mysql_error(&conn.mMySql) << ", db: " << dbConfig.getDatabase()
                    << " host: " << dbConfig.getHost() << " - reconnecting and restarting operation.");
            }
            else
            {
                BLAZE_ERR_LOG(Log::DB, "[MySqlPreparedStatement].execute: conn=" << conn.getName() << "; mysql_stmt_prepare() failed:  ERROR "
                    << errCode << " (" << mysql_sqlstate(&conn.mMySql) << "): " << mysql_error(&conn.mMySql) << ", db: " << dbConfig.getDatabase()
                    << " host: " << dbConfig.getHost());
            }
            close(conn);
            return error;
        }
    }

    setupBindParams(mysql_stmt_param_count(mStatement));

    if (outResult != nullptr)
        *outResult = nullptr;
    if (mysql_stmt_bind_param(mStatement, mBindings))
    {
        uint32_t errcode = mysql_stmt_errno(mStatement);
        BLAZE_ERR_LOG(Log::DB, "[MySqlPreparedStatement].execute: mysql_stmt_bind_param() failed: " << mysql_stmt_error(mStatement) << " (" << errcode << ")");
        close(conn);
        return DbConnBase::convertErrorCode(errcode);
    }

    if (mysql_stmt_execute(mStatement) != 0)
    {
        uint32_t errcode = mysql_stmt_errno(mStatement);
        BlazeRpcError err = DbConnBase::convertErrorCode(errcode);
        if (MySqlConn::isLoggable(err))
        {
            if (errcode == CR_SERVER_GONE_ERROR && conn.getUseCount() == 0)
            {
                BLAZE_TRACE_LOG(Log::DB, "[MySqlPreparedStatement].execute: mysql_stmt_execute() failed: " << mysql_stmt_error(mStatement) 
                                << " (" << errcode << ") - restarting operation");
            }
            else
            {
                BLAZE_WARN_LOG(Log::DB, "[MySqlPreparedStatement].execute: mysql_stmt_execute() failed: " << mysql_stmt_error(mStatement) 
                               << " (" << errcode << ")");
            }
        }
        close(conn);
        return err;
    }

    MYSQL_RES* meta = mysql_stmt_result_metadata(mStatement);
    if (meta != nullptr && mResultBindings == nullptr)
    {
        uint32_t numFields = mysql_num_fields(meta);
        MYSQL_FIELD* fields = mysql_fetch_fields(meta);
        setupBindResults(fields, numFields);

        if (mysql_stmt_bind_result(mStatement, mResultBindings) != 0)
        {
            uint32_t errcode = mysql_stmt_errno(mStatement);
            BLAZE_ERR_LOG(Log::DB, "[MySqlPreparedStatement].execute: mysql_stmt_bind_result() failed: " << mysql_stmt_error(mStatement)
                          << " (" << errcode << ")");
            mysql_free_result(meta);
            close(conn);
            return DbConnBase::convertErrorCode(errcode);
        }
    }
    MySqlPreparedStatementResult* mySqlResult = BLAZE_NEW MySqlPreparedStatementResult;
    BlazeRpcError error = mySqlResult->init(mStatement, meta, mResultBindings, mRowData);
    if (error != DB_ERR_OK)
    {
        delete mySqlResult;
        close(conn);
        return error;
    }

    // Need to check this flag for a timeout since killing a dead query causes mysql_real_query
    // to return success rather than failure even though the query didn't complete
    if (conn.queryTimedOut())
    {
        delete mySqlResult;
        mySqlResult = nullptr;
        close(conn);
        return DB_ERR_TIMEOUT;
    }

    if (outResult != nullptr)
    {
        *outResult = mySqlResult;
    }
    else if (mySqlResult != nullptr)
    {
        //If the user didn't want a result and we have one, kill ours.
        delete mySqlResult;
    }

    // This implementation supports capturing the *first* resultset returned by MySql as a result
    // of executing the prepared statement.  However, some prepared statements (e.g. prepared CALL statements)
    // return multiple resultsets by design (one resultset containing data, followed by a second containing
    // status details, etc.).  See https://dev.mysql.com/doc/refman/5.5/en/c-api-prepared-call-statements.html
    // 
    // That said, we need to clear out the remaining resultsets in order to return this connection to a usable state.
    // If we don't, the next attempt to make a mysql call on this connection will return CR_COMMANDS_OUT_OF_SYNC,
    // and the connection will be forcefully closed.

    // First, we need to clear the result bindings so that they get recreated as needed for the next execute().
    clearResultBindings();

    if (mysql_more_results(&conn.mMySql))
    {
        // Get the statement ready to handle the next resultset, which we are just going to swallow.
        mysql_stmt_free_result(mStatement);

        int32_t droppedResultsets = 0;
        while (mysql_stmt_next_result(mStatement) == 0)
        {
            MYSQL_RES* droppedResultset = mysql_stmt_result_metadata(mStatement);
            if (droppedResultset != nullptr)
            {
                mysql_free_result(droppedResultset);
                ++droppedResultsets;
            }

            // Get the statement ready to handle the next resultset, which we are just going to swallow.
            mysql_stmt_free_result(mStatement);
        }

        if (droppedResultsets > 0)
        {
            BLAZE_WARN_LOG(Log::DB, "[MySqlPreparedStatement].execute: Dropping " << droppedResultsets << " extra resultsets returned from prepared statement '" << getQuery() << "'.");
        }
    }

    return DB_ERR_OK;
}

BlazeRpcError MySqlPreparedStatement::execute(DbResult** result, MySqlAsyncConn& conn, TimeValue timeout)
{
    TimeValue absoluteTimeout = 0;
    if (timeout != 0)
    {
        absoluteTimeout = TimeValue::getTimeOfDay() + timeout;
    }

    BlazeError rc;
    int32_t status = 0;
    if (mStatement == nullptr)
    {
        mStatement = mysql_stmt_init(&conn.mMySql);
        if (mStatement == nullptr)
        {
            EA_FAIL();
            // bad news
            return DB_ERR_SYSTEM;
        }

        rc = conn.performBlockingOp("MySqlPreparedStatement::execute - mysql_stmt_prepare_", getQuery(), absoluteTimeout,
            [this, &status]() {
                return mysql_stmt_prepare_start(&status, mStatement, getQuery(), (uint32_t)strlen(getQuery()));
            },
            [this, &status](uint32_t flags) {
                return mysql_stmt_prepare_cont(&status, mStatement, flags);
            });

        if (rc != ERR_OK)
        {
            close(conn);
            return rc;
        }
    }

    setupBindParams(mysql_stmt_param_count(mStatement));

    if (result != nullptr)
        *result = nullptr;

    if (mysql_stmt_bind_param(mStatement, mBindings))
    {
        auto errcode = mysql_stmt_errno(mStatement);
        BLAZE_ERR_LOG(Log::DB, "[MySqlPreparedStatement].execute: mysql_stmt_bind_param() failed: " << mysql_stmt_error(mStatement) << " (" << errcode << ")");
        close(conn);
        return DbConnBase::convertErrorCode(errcode);
    }

    rc = conn.performBlockingOp("MySqlPreparedStatement::execute - mysql_stmt_execute_", getQuery(), absoluteTimeout,
        [this, &status]() {
            return mysql_stmt_execute_start(&status, mStatement);
        },
        [this, &status](uint32_t flags) {
            return mysql_stmt_execute_cont(&status, mStatement, flags);
        });

    if (rc != ERR_OK)
    {
        close(conn);
        return rc;
    }

    MYSQL_RES* meta = mysql_stmt_result_metadata(mStatement);
    if (meta != nullptr && mResultBindings == nullptr)
    {
        auto numFields = mysql_num_fields(meta);
        MYSQL_FIELD* fields = mysql_fetch_fields(meta);

        setupBindResults(fields, numFields);

        if (mysql_stmt_bind_result(mStatement, mResultBindings) != 0)
        {
            uint32_t errcode = mysql_stmt_errno(mStatement);
            BLAZE_ERR_LOG(Log::DB, "[MySqlPreparedStatement].execute: mysql_stmt_bind_result() failed: " << mysql_stmt_error(mStatement)
                << " (" << errcode << ")");
            // Note: there is no need to directly call mysql_free_result; closing the statement will clear the result set on the client
            close(conn);
            return DbConnBase::convertErrorCode(errcode);
        }
    }

    MySqlPreparedStatementResult* mySqlResult = BLAZE_NEW MySqlPreparedStatementResult;
    rc = mySqlResult->initAsync(conn, getQuery(), mStatement, meta, mResultBindings, mRowData, absoluteTimeout);
    if (rc != ERR_OK)
    {
        delete mySqlResult;
        close(conn);
        return rc;
    }

    // clear the result bindings so that they get recreated as needed for the next execute().
    clearResultBindings();

    if (mysql_stmt_more_results(mStatement) != 0)
    {
        // NOTE: we currently do not support multiple results from a stored procedure, to avoid leaving the statement in a half-consumed state instead of walking and freeing all the results from the server, we simply reset the prepared statement which puts it back to prepared state and ready for next execution. If support for multiple result sets is desirable, we should rewrite this implementation using the following example for guidance: https://dev.mysql.com/doc/refman/5.5/en/c-api-prepared-call-statements.html
        BLAZE_TRACE_LOG(Log::DB, "[MySqlPreparedStatement].execute: Ignored trailing results returned by query=" << getQuery());

        // Note: this does not clear the result set on the client; the caller must still close the statement to complete cleanup
        // (re-executing the statement also clears the result set: https://mariadb.com/kb/en/library/mysql_stmt_reset/)
        my_bool resetResult = 0;
        rc = conn.performBlockingOp("MySqlPreparedStatement::execute - mysql_stmt_reset_", getQuery(), absoluteTimeout,
            [this, &resetResult]() {
                return mysql_stmt_reset_start(&resetResult, mStatement);
            },
            [this, &resetResult](uint32_t flags) {
                return mysql_stmt_reset_cont(&resetResult, mStatement, flags);
            });
        if (rc != ERR_OK)
        {
            delete mySqlResult;
            close(conn);
            return rc;
        }
    }

    if (result != nullptr)
    {
        *result = mySqlResult;
    }
    else if (mySqlResult != nullptr)
    {
        //If the user didn't want a result and we have one, delete ours.
        delete mySqlResult;
    }

    return rc;
}

} // namespace Blaze

