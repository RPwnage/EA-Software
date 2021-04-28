/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class MySqlResult
*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/
#include "framework/blaze.h"
#include "EASTL/sort.h"
#include "framework/database/dbconn.h"
#include "framework/database/mysql/mysqlasyncconn.h"
#include "framework/database/mysql/mysqlrow.h"
#include "framework/database/mysql/mysqlresult.h"

namespace Blaze
{

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

MySqlResultBase::MySqlResultBase(MYSQL* mysql, const char8_t* connName)
:   mMySql(mysql),
    mRowAffectedCount(0),
    mLastInsertId(0),
    mNumFields(0),
    mResult(nullptr),
    mFields(nullptr)
{
    mConnInfo.append_sprintf("%s host %s", connName, ((mysql->host_info != nullptr) ? mysql->host_info : "unknown"));
}

MySqlResultBase::~MySqlResultBase()
{
    if (mResult != nullptr)
    {
        mysql_free_result(mResult);
    }
}

/*************************************************************************************************/
/*!
    \brief getColumnName
        retrieves the name of a column given its index
    \param[in] columnIndex - the index of the column to retrieve
    \returns - the name of the column
*/
/*************************************************************************************************/
const char8_t* MySqlResultBase::getColumnName(uint32_t columnIndex) const 
{
    if (mFields == nullptr || columnIndex >= mNumFields)
    {
        return nullptr; 
    }
    return mFields[columnIndex].name;
}

/*************************************************************************************************/
/*!
    \brief Initialize this instance with the given MySql result
*/
/*************************************************************************************************/
BlazeRpcError MySqlResultBase::init(MYSQL_RES* res)
{
    EA_ASSERT(mResult == nullptr);
    mResult = res;
    if (mResult != nullptr) //there are some rows
    {
        mFields = mysql_fetch_fields(mResult);
        mNumFields = mysql_num_fields(mResult);
        if (mNumFields > 0)
        {
            mFieldLookup.resize(mNumFields);
            for(uint16_t idx = 0; idx < mNumFields; ++idx)
            {
                // populate the index array
                mFieldLookup[idx] = idx;
            }
            // initialize the index array to refer to fields in sorted order
            eastl::sort(mFieldLookup.begin(), mFieldLookup.end(), FieldCompare(mFields));
        }
        else
        {
            // NOTE: We need to print an error here because mysql_errno(mMySql) will not be descriptive enough.
            
            BLAZE_ERR_LOG(Log::DB, " [MySqlResult].init: conn=" << mConnInfo << "; columns(" << mNumFields << "). Error " << mysql_error(mMySql));
            return DbConnBase::convertErrorCode(mysql_errno(mMySql));
        }
    }
    else // check if it was non select statement
    {
        if(mysql_field_count(mMySql) == 0)
        {
            // query was okay, it was not a SELECT though
            initializeRowAffectedCount();
            mLastInsertId = mysql_insert_id(mMySql);
        }
        else
        {
            return DbConnBase::convertErrorCode(mysql_errno(mMySql));
        }
    }
    return DB_ERR_OK;
}

void MySqlResultBase::initializeRowAffectedCount()
{
    mRowAffectedCount = (uint32_t)mysql_affected_rows(mMySql);
}

MySqlResult::MySqlResult(MySqlConn& conn)
:   MySqlResultBase(&conn.mMySql, conn.getName()),
    mDataSizes(nullptr)
{
}

MySqlResult::MySqlResult(MySqlAsyncConn& conn)
    : MySqlResultBase(&conn.mMySql, conn.getName()),
    mDataSizes(nullptr)
{
}

MySqlResult::~MySqlResult()
{
    if (mDataSizes != nullptr)
    {
        delete [] mDataSizes;
    }
}

/*************************************************************************************************/
/*!
    \brief Initialize this instance with the given MySql result
*/
/*************************************************************************************************/
BlazeRpcError MySqlResult::init(MYSQL_RES* res)
{
    BlazeRpcError err = MySqlResultBase::init(res);
    if ((err == DB_ERR_OK) && (res != nullptr))
    {
        const size_t numRows = static_cast<size_t>(mysql_num_rows(res));
        if (numRows > 0)
        {
            if (mNumFields > 0)
            {
                mDataSizes = BLAZE_NEW_ARRAY(unsigned long, numRows*mNumFields);
                mMySqlRows.reserve(numRows);
                mRows.reserve(numRows);
                MYSQL_ROW row = mysql_fetch_row(mResult);
                for(unsigned long* dataPtr = mDataSizes; row != nullptr; row = mysql_fetch_row(mResult), dataPtr += mNumFields)
                {
                    const unsigned long* datasizes = mysql_fetch_lengths(mResult);
                    if(datasizes != nullptr)
                    {
                        memcpy(dataPtr, datasizes, mNumFields*sizeof(dataPtr[0]));
                    }
                    else
                    {
                        memset(dataPtr, 0, mNumFields*sizeof(dataPtr[0]));
                    }
                    mMySqlRows.push_back(MySqlRow(this, row, dataPtr));
                    mRows.push_back(&mMySqlRows.back()); // TODO: eventually get rid of mRows, for now also have to cache the pointer
                }
            }
            else
            {
                // NOTE: We need to print an error here because mysql_errno(&mConn->mMySql) will not be descriptive enough.
                BLAZE_ERR_LOG(Log::DB, " [MySqlResult].init: conn=" << mConnInfo << "; rows(" << numRows << "), columns(" << mNumFields << "). Error "
                    << mysql_error(mMySql));
                return DbConnBase::convertErrorCode(mysql_errno(mMySql));
            }
        } // NOTE: It's OK to have numRows == 0
    }
    return err;
}

/*************************************************************************************************/
/*!
    \brief convert the result to a string
*/
/*************************************************************************************************/
const char8_t* MySqlResult::toString(eastl::string& buf, int32_t maxStringLen, int32_t maxBinaryLen) const
{
    if (!mMySqlRows.empty())
    {
        MySqlRowList::const_iterator itr = mMySqlRows.begin();
        MySqlRowList::const_iterator end = mMySqlRows.end();
        for(int32_t idx = 0; itr != end; ++itr, ++idx)
        {
            buf.append_sprintf("\n    %d: ", idx);
            itr->toString(buf, maxStringLen, maxBinaryLen);
        }
    }
    return buf.c_str();
}

MySqlStreamingResult::MySqlStreamingResult(MySqlConn& conn)
:   MySqlResultBase(&conn.mMySql, conn.getName()),
    mCurrentCachedRowIndex(0),
    mPrefetchRowCount(0),
    mDataSizeMem(nullptr),
    mRowPtrMem(nullptr),
    mConn(conn)
{
}

MySqlStreamingResult::~MySqlStreamingResult()
{
    clearCachedRows();

    BLAZE_FREE(mDataSizeMem);
    BLAZE_FREE(mRowPtrMem);  

    //toss off a hint to free the result.
    BlazeRpcError err = mConn.freeStreamingResults(mConn.calculateQueryTimeout(0));
    if (err != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::DB, "[MySqlStreamingResult].~MySqlStreamingResult: Conn " << mConnInfo <<  " failed to free sql result object.")
    }
    mResult = nullptr;
}

/*************************************************************************************************/
/*!
    \brief Initialize this instance with the given MySql result
*/
/*************************************************************************************************/
BlazeRpcError MySqlStreamingResult::init(MYSQL_RES* res)
{
    BlazeRpcError result = MySqlResultBase::init(res);

    int32_t row, mem;
    mConn.getOwner().getPrefetchParameters(row, mem);
    mPrefetchRowCount = (row == DbConnInterface::UNSET_PREFETCH_ROW_COUNT) ? DEFAULT_PREFETCH_ROW_COUNT : row;

    mCachedRows.clear();
    mCachedRows.reserve(mPrefetchRowCount);

    BLAZE_FREE(mDataSizeMem);
    mDataSizeMem = BLAZE_ALLOC(mNumFields * mPrefetchRowCount * sizeof(long));
    
    BLAZE_FREE(mRowPtrMem);
    mRowPtrMem = BLAZE_ALLOC(mNumFields * mPrefetchRowCount * sizeof(char8_t*));

    return result;
}

/*************************************************************************************************/
/*!
    \brief next

    Fetches the next result in the result set from the database

    \note - the caller is responsible for de-allocating the DbRow object that is returned.
    \return - the next row in the result set, or nullptr if there are none.
*/
/*************************************************************************************************/
DbRow* MySqlStreamingResult::next()
{
    if (mCurrentCachedRowIndex == mCachedRows.size())
    {
        clearCachedRows();            // assert that we're starting out with an empty cache.
        mCurrentCachedRowIndex = 0;
        BlazeRpcError err = mConn.fetchNextStreamingResultRows(*this, mConn.calculateQueryTimeout(0));
        if (err != ERR_OK || mCachedRows.empty())
        {
            return nullptr;
        }
    }

    CachedRow& cache = mCachedRows[mCurrentCachedRowIndex++];
       
    return BLAZE_NEW MySqlRow(this, cache.row, cache.dataSizes);
}

void MySqlStreamingResult::addCacheRow(MYSQL_ROW row)
{
    const unsigned long* dataSizes = mysql_fetch_lengths(mResult);
    size_t dataSizesSize = sizeof(long) * mNumFields;

    //Copy the row and data length into the cache
    CachedRow& cacheEntry = mCachedRows.push_back();   
 
    //data sizes and row pointer memory is preallocated based on the fetch size.  This code calcs where 
    //we are in the memory and assigns those pointers from the prealloced blocks.

    size_t memOffset = mCachedRows.size() - 1; //indexed into the prealloced memory for rows/datasize.
    cacheEntry.dataSizes = (unsigned long*) ((char8_t*) mDataSizeMem + (dataSizesSize * memOffset));
    memcpy(cacheEntry.dataSizes, dataSizes, dataSizesSize);

    cacheEntry.row = (MYSQL_ROW) ((char8_t*) mRowPtrMem + (sizeof(char8_t*) * mNumFields * memOffset));
    
    //We don't want to have lots of allocations for each field, so we store all the memory for the row
    //in a single mem block

    //calc the total size for the row and alloc
    size_t rowMemSize = 0;
    for (size_t colCount = 0; colCount < mNumFields; ++colCount)
    {
        rowMemSize += dataSizes[colCount] + 1;  //leave a space for the nullptr pointer      
    }

    //Allocate the row memory
    char8_t* memPtr = (char8_t*) BLAZE_ALLOC(rowMemSize);

    //step through the row memory and assign it to each ptr
    for (size_t colCount = 0; colCount < mNumFields; ++colCount)
    {        
        memcpy(memPtr, row[colCount], dataSizes[colCount]);
        memPtr[dataSizes[colCount]] = '\0'; //truncate the data - the data isn't guaranteed a trailing nullptr.
        cacheEntry.row[colCount] = memPtr;
        memPtr += dataSizes[colCount] + 1; //adjust the memptr for the next column.
    }
}

void MySqlStreamingResult::clearCachedRows()
{
    for (CachedRows::iterator itr = mCachedRows.begin(), end = mCachedRows.end(); itr != end; ++itr)
    {
        if (itr->row != nullptr)
        {
            //head pointer of the row is stored in the first entry - the rest are part of the same block of mem.
            BLAZE_FREE(itr->row[0]);
        }
    }
    mCachedRows.clear();
}

MySqlAsyncStreamingResult::MySqlAsyncStreamingResult(MySqlAsyncConn& conn)
    : MySqlResultBase(&conn.mMySql, conn.getName()),
    mConn(conn),
    mFetchedAllRows(false)
{
}

MySqlAsyncStreamingResult::~MySqlAsyncStreamingResult()
{
    if (mResult != nullptr)
    {
        mConn.mStreamingResults.erase(this);
        mConn.cleanupStreamingResult(this);
    }
}

BlazeRpcError MySqlAsyncStreamingResult::init(MYSQL_RES* res)
{
    return MySqlResultBase::init(res);
}

uint32_t MySqlAsyncStreamingResult::getAffectedRowCount() const
{
    if (!mFetchedAllRows)
        BLAZE_ASSERT_LOG(Log::DB, " [MySqlAsyncStreamingResult].getAffectedRowCount: conn=" << mConnInfo << "; attempt to get row affected count before all rows have been fetched!");

    return mRowAffectedCount;
}

DbRow* MySqlAsyncStreamingResult::next()
{
    if (mFetchedAllRows || mResult == nullptr)
        return nullptr;

    MYSQL_ROW row;
    auto rc = mConn.performBlockingOp("MySqlAsyncStreamingResult::next - mysql_fetch_row_", "N/A", mConn.calculateQueryTimeout(0),
        [this, &row]() {
            return mysql_fetch_row_start(&row, mResult);
        },
        [this, &row](uint32_t flags) {
            return mysql_fetch_row_cont(&row, mResult, flags);
        });

    if (rc != ERR_OK)
        return nullptr;

    if (row != nullptr)
        return BLAZE_NEW MySqlRow(this, row, mysql_fetch_lengths(mResult));

    mRowAffectedCount = (uint32_t)mysql_affected_rows(mMySql);
    mFetchedAllRows = true;
    return nullptr;
}

} // Blaze

