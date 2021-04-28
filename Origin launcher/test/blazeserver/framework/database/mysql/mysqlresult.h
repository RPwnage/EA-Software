/*************************************************************************************************/
/*!
    \file 


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_MYSQLRESULT_H
#define BLAZE_MYSQLRESULT_H
/*** Include files *******************************************************************************/

#include "EASTL/vector.h"
#include "framework/database/dberrors.h"
#include "framework/database/mysql/mysqlconn.h"
#include "framework/database/mysql/mysqlrow.h"
#include "framework/util/shared/blazestring.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

/*************************************************************************************************/
/*!
    \class MySqlResultBase
    \brief Common base class for returning results from a MySql database operation
*/
/*************************************************************************************************/
class MySqlResultBase
{
    NON_COPYABLE(MySqlResultBase);
    friend class MySqlRow;

public: 
    MySqlResultBase(MYSQL* mysql, const char8_t* connName);
    virtual ~MySqlResultBase();
    virtual BlazeRpcError init(MYSQL_RES* res);

    const char8_t* getColumnName(uint32_t columnIndex) const;

protected:

    struct FieldCompare
    {
        FieldCompare(const MYSQL_FIELD* fields) : mFields(fields) {}
        bool operator() (const uint16_t a, const uint16_t b);
        bool operator() (const uint16_t a, const char8_t* str);
        bool operator() (const char8_t* str, const uint16_t b);
    private:
        const MYSQL_FIELD* mFields;
    };
    typedef eastl::vector<uint16_t> FieldLookup; // uint16_t is plenty big to index up to 64K columns
    
    MYSQL* mMySql;
    uint32_t mRowAffectedCount;
    uint64_t mLastInsertId;
    uint32_t mNumFields; // number of fields in each fetched row
    MYSQL_RES* mResult;
    MYSQL_FIELD* mFields; // field data valid for each fetched row
    FieldLookup mFieldLookup;
    eastl::string mConnInfo;

    virtual void initializeRowAffectedCount();
};

inline bool MySqlResultBase::FieldCompare::operator()(const uint16_t a, const uint16_t b)
{
    return blaze_stricmp(mFields[a].name, mFields[b].name) < 0;
}

inline bool MySqlResultBase::FieldCompare::operator()(const uint16_t a, const char8_t* str)
{
    return blaze_stricmp(mFields[a].name, str) < 0;
}

inline bool MySqlResultBase::FieldCompare::operator()(const char8_t* str, const uint16_t b)
{
    return blaze_stricmp(str, mFields[b].name) < 0;
}

/*************************************************************************************************/
/*!
    \class MySqlResult
    \brief MySql implementation of the DbResult abstract class.
*/
/*************************************************************************************************/
class MySqlResult : public DbResult, public MySqlResultBase
{
    NON_COPYABLE(MySqlResult);

public:
    MySqlResult(MySqlConn& conn);
    MySqlResult(MySqlAsyncConn& conn);
    ~MySqlResult() override;

    BlazeRpcError init(MYSQL_RES* res) override;
    // TODO: need to change this object to be able to fetch the rows one at a time, or to have an addRow(MYSQL_ROW) method, this way it can be populated in a streaming fetch and run parallel with the streaming code that's pulling in rows from the server, thereby lowering the memory footprint, and increasing parallelism.

    uint32_t getAffectedRowCount() const override { return mRowAffectedCount; }
    uint32_t getColumnCount() const override { return mNumFields; }
    uint64_t getLastInsertId() const override { return mLastInsertId; }
    const char8_t* getColumnName(uint32_t columnIndex) const override { return MySqlResultBase::getColumnName(columnIndex); };
    const char8_t* toString(eastl::string& buf, int32_t maxStringLen = 0, int32_t maxBinaryLen = 0) const override;

private:
    typedef eastl::vector<MySqlRow> MySqlRowList;
    
    const RowList &getRowList() const override { return mRows; }

    RowList mRows; // remains for backwards compatibility with DbResult(TODO: remove in favor of custom MySqlRowList iterator adapter?)
    MySqlRowList mMySqlRows; // stores actual MySqlRow objects(MySqlRow object must have a working copy constructor & operator!)
    unsigned long* mDataSizes; // stores value sizes for each cell in each fetched row
};


/*************************************************************************************************/
/*!
    \class MySqlStreamingResult
    \brief MySql implementation of the StreamingDbResult abstract class.
*/
/*************************************************************************************************/
class MySqlStreamingResult : public StreamingDbResult, public MySqlResultBase
{
    NON_COPYABLE(MySqlStreamingResult);

public:
    MySqlStreamingResult(MySqlConn& conn);
    ~MySqlStreamingResult() override;

    const static size_t DEFAULT_PREFETCH_ROW_COUNT = 1000;

    BlazeRpcError init(MYSQL_RES* res) override;
    DbRow* next() override;

    size_t getPrefetchRowCount() const { return mPrefetchRowCount; }
    void addCacheRow(MYSQL_ROW row);

    uint32_t getAffectedRowCount() const override { return mRowAffectedCount; }
    uint32_t getColumnCount() const override { return mNumFields; }
    uint64_t getLastInsertId() const override { return mLastInsertId; }
    const char8_t* getColumnName(uint32_t columnIndex) const override { return MySqlResultBase::getColumnName(columnIndex); };
    const char8_t* toString(eastl::string& buf, int32_t maxStringLen = 0, int32_t maxBinaryLen = 0) const override { return ""; }

private:
    friend class MySqlConn;

    void clearCachedRows();
    MYSQL_RES* getMySqlRes() { return mResult; }

    struct CachedRow
    {
        MYSQL_ROW row;
        unsigned long* dataSizes;
    };
    typedef eastl::vector<CachedRow> CachedRows;

    CachedRows mCachedRows;
    uint32_t mCurrentCachedRowIndex;
    size_t mPrefetchRowCount;

    void* mDataSizeMem;
    void* mRowPtrMem;
    MySqlConn& mConn;
};

class MySqlAsyncStreamingResult : public StreamingDbResult, public MySqlResultBase
{
    NON_COPYABLE(MySqlAsyncStreamingResult);

public:
    MySqlAsyncStreamingResult(MySqlAsyncConn& conn);
    ~MySqlAsyncStreamingResult() override;

    BlazeRpcError init(MYSQL_RES* res) override;
    DbRow* next() override;

    bool hasFetchedAllRows() const { return mFetchedAllRows; }

    uint32_t getAffectedRowCount() const override;
    uint32_t getColumnCount() const override { return mNumFields; }
    uint64_t getLastInsertId() const override { return mLastInsertId; }
    const char8_t* getColumnName(uint32_t columnIndex) const override { return MySqlResultBase::getColumnName(columnIndex); };
    const char8_t* toString(eastl::string& buf, int32_t maxStringLen = 0, int32_t maxBinaryLen = 0) const override { return ""; }

protected:
    void initializeRowAffectedCount() override { mRowAffectedCount = 0; mFetchedAllRows = false; }

private:
    friend class MySqlAsyncConn;

    MySqlAsyncConn& mConn;
    bool mFetchedAllRows;
};

}// Blaze
#endif // BLAZE_MySqlResult_H

