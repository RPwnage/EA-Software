/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class MySqlPreparedStatementResult
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "framework/database/dbconn.h"
#include "framework/database/mysql/mysqlasyncconn.h"
#include "framework/database/mysql/mysqlpreparedstatementresult.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

/*************************************************************************************************/
/*!
    \brief Initialize this instance with the given MySql prepared statement
*/
/*************************************************************************************************/
BlazeRpcError MySqlPreparedStatementResult::init(MYSQL_STMT* stmt, MYSQL_RES* meta, MYSQL_BIND* bind,
    MySqlPreparedStatementRow::ResultDataList& rowData)
{
    BlazeRpcError error = DB_ERR_OK;

    if (meta != nullptr)
    {
        // This was a SELECT statement so grab the result set

        mRowAffectedCount = 0;
        mLastInsertId = 0;

        uint32_t numFields = mysql_num_fields(meta);
        MYSQL_FIELD *fields = mysql_fetch_fields(meta);
       
        if (mysql_stmt_store_result(stmt) != 0)
        {
            uint32_t errcode = mysql_stmt_errno(stmt);
            BLAZE_ERR_LOG(Log::DB, "[MySqlPreparedStatementResult].init: mysql_stmt_store_result() failed: " << mysql_stmt_error(stmt) << " (" << errcode << ")");
            error = DbConnBase::convertErrorCode(errcode);
        }
        else
        {
            // Read all the rows from the DB
            my_ulonglong rowCount = mysql_num_rows(meta);
            mRows.reserve((size_t)rowCount);
            bool rebind = false;
            while (true)
            {
                // MySql client takes a *copy* when the initial results binding is done, so if we need to re-allocate any buffers
                // we need to re-issue the bind command so that MySql client has the up-to-date buffer pointers
                if (rebind)
                {
                    rebind = false;
                    if (mysql_stmt_bind_result(stmt, bind) != 0)
                    {
                        uint32_t errcode = mysql_stmt_errno(stmt);
                        BLAZE_ERR_LOG(Log::DB, "[MySqlPreparedStatementResult].init: mysql_stmt_bind_result() failed: "
                                      << mysql_stmt_error(stmt) << " (" << errcode << ")");
                        error = DbConnBase::convertErrorCode(errcode);
                        break;
                    }
                }

                int rc = mysql_stmt_fetch(stmt);
                if (rc == MYSQL_DATA_TRUNCATED)
                {
                    // Truncation occurred so update the buffer sizes and refetch the
                    // truncated columns
                    for(uint32_t idx = 0; idx < numFields; ++idx)
                    {
                        MySqlPreparedStatementRow::ResultData& data = rowData[idx];
                        if ((data.isBinaryType() || data.isStringType()) && data.mError)
                        {
                            // This column was truncated so reallocate the buffer large enough to
                            // hold the entire column data and then refetch
                            bind[idx].buffer_length = data.mLength;
                            data.allocData(data.mLength);
                            bind[idx].buffer = data.u.mData;
                            rebind = true;
                            if (mysql_stmt_fetch_column(stmt, &bind[idx], idx, 0) != 0)
                            {
                                uint32_t errcode = mysql_stmt_errno(stmt);
                                BLAZE_ERR_LOG(Log::DB, "[MySqlPreparedStatementResult].init: mysql_stmt_fetch_column() failed: " << mysql_stmt_error(stmt) << " (" << errcode << ")");
                                error = DbConnBase::convertErrorCode(errcode);
                                break;;
                            }
                        }
                    }
                }
                else if (rc == MYSQL_NO_DATA)
                {
                    // No more rows so exit the loop
                    break;
                }
                else if (rc != 0)
                {
                    uint32_t errcode = mysql_stmt_errno(stmt);
                    BLAZE_ERR_LOG(Log::DB, "[MySqlPreparedStatementResult].init: mysql_stmt_fetch() failed: " << mysql_stmt_error(stmt) << " (" << errcode << ")");
                    error = DbConnBase::convertErrorCode(errcode);
                    break;
                }

                // Success so create and add row
                MySqlPreparedStatementRow* row = BLAZE_NEW MySqlPreparedStatementRow(
                        rowData, fields, numFields);
                mRows.push_back(row);
            }
        }
        mysql_free_result(meta);
    }
    else
    {
        // query was okay, it was not a SELECT though
        mRowAffectedCount = (uint32_t)mysql_stmt_affected_rows(stmt);
        mLastInsertId = mysql_stmt_insert_id(stmt);
    }
    return error;
}

BlazeError MySqlPreparedStatementResult::initAsync(MySqlAsyncConn& conn, const char8_t* queryDesc, MYSQL_STMT* stmt, MYSQL_RES* meta, MYSQL_BIND* bind, MySqlPreparedStatementRow::ResultDataList& rowData, TimeValue absoluteTimeout)
{
    BlazeError rc = ERR_OK;

    if (meta != nullptr)
    {
        // This was a SELECT statement so grab the result set
        int32_t status = 0;
        mRowAffectedCount = 0;
        mLastInsertId = 0;

        uint32_t numFields = mysql_num_fields(meta);
        MYSQL_FIELD *fields = mysql_fetch_fields(meta);

        // NOTE: We intentionally don't use mysql_stmt_store_result_* since it is not actually necessary 
        // to fetch the entire result before iterating over the results, we are better off actually achieving
        // some parallel execution between the blaze and mysql servers.

        // Read all the rows from the DB
        my_ulonglong rowCount = mysql_num_rows(meta);
        mRows.reserve((size_t)rowCount);
        bool rebind = false;
        while (true)
        {
            // MySql client takes a *copy* when the initial results binding is done, so if we need to re-allocate any buffers
            // we need to re-issue the bind command so that MySql client has the up-to-date buffer pointers
            if (rebind)
            {
                rebind = false;
                if (mysql_stmt_bind_result(stmt, bind) != 0)
                {
                    uint32_t errcode = mysql_stmt_errno(stmt);
                    BLAZE_ERR_LOG(Log::DB, "[MySqlPreparedStatementResult].initAsync: mysql_stmt_bind_result() failed: " << mysql_stmt_error(stmt) << " (" << errcode << ")");
                    rc = DbConnBase::convertErrorCode(errcode);
                    break;
                }
            }

            rc = conn.performBlockingOp("MySqlPreparedStatementResult::initAsync - mysql_stmt_fetch_", queryDesc, absoluteTimeout,
                [&status, stmt]() {
                    return mysql_stmt_fetch_start(&status, stmt);
                },
                [&status, stmt](uint32_t flags) {
                    return mysql_stmt_fetch_cont(&status, stmt, flags);
                });

            if (rc != ERR_OK)
            {
                break;
            }
            else if (status == MYSQL_DATA_TRUNCATED)
            {
                // Truncation occurred so update the buffer sizes and refetch the
                // truncated columns
                for (uint32_t idx = 0; idx < numFields; ++idx)
                {
                    MySqlPreparedStatementRow::ResultData& data = rowData[idx];
                    if ((data.isBinaryType() || data.isStringType()) && data.mError)
                    {
                        // This column was truncated so reallocate the buffer large enough to
                        // hold the entire column data and then refetch
                        bind[idx].buffer_length = data.mLength;
                        data.allocData(data.mLength);
                        bind[idx].buffer = data.u.mData;
                        rebind = true;
                        if (mysql_stmt_fetch_column(stmt, &bind[idx], idx, 0) != 0)
                        {
                            uint32_t errcode = mysql_stmt_errno(stmt);
                            BLAZE_ERR_LOG(Log::DB, "[MySqlPreparedStatementResult].initAsync: mysql_stmt_fetch_column() failed: " << mysql_stmt_error(stmt) << " (" << errcode << ")");
                            rc = DbConnBase::convertErrorCode(errcode);
                            break;
                        }
                    }
                }
            }
            else if (status == MYSQL_NO_DATA)
            {
                // No more rows so exit the loop
                break;
            }
            else
                ; // Default normal case, all is well
                  // Success so create and add row
            MySqlPreparedStatementRow* row = BLAZE_NEW MySqlPreparedStatementRow(rowData, fields, numFields);
            mRows.push_back(row);
        }

        rc = conn.performBlockingOp("MySqlPreparedStatementResult::initAsync - mysql_free_result_", queryDesc, absoluteTimeout,
            [meta]() {
                return mysql_free_result_start(meta);
            },
            [meta](uint32_t flags) {
                return mysql_free_result_cont(meta, flags);
            });
    }
    else
    {
        // query was okay, it was not a SELECT though
        mRowAffectedCount = (uint32_t)mysql_stmt_affected_rows(stmt);
        mLastInsertId = mysql_stmt_insert_id(stmt);
    }
    return rc;
}

MySqlPreparedStatementResult::~MySqlPreparedStatementResult()
{
    // Delete the row data itself
    for (RowList::iterator itr = mRows.begin() ; itr < mRows.end() ; ++itr)
    {
        delete *itr;
    }
}

const char8_t* MySqlPreparedStatementResult::toString(eastl::string& buf,
        int32_t maxStringLen, int32_t maxBinaryLen) const
{
    if (mRows.size() > 0)
    {
        RowList::const_iterator itr = mRows.begin();
        RowList::const_iterator end = mRows.end();
        for(int32_t idx = 0; itr != end; ++itr, ++idx)
        {
            MySqlPreparedStatementRow* row = static_cast<MySqlPreparedStatementRow*>(*itr);

            buf.append_sprintf("\n    %d: ", idx);
            row->toString(buf, maxStringLen, maxBinaryLen);
        }
    }
    return buf.c_str();
}

} // Blaze

