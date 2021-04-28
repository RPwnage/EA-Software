/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_MYSQLPREPAREDSTATEMENTRESULT_H
#define BLAZE_MYSQLPREPAREDSTATEMENTRESULT_H
/*** Include files *******************************************************************************/

#include "framework/database/mysql/mysqlconn.h"
#include "framework/database/mysql/mysqlpreparedstatementrow.h"
#include "framework/database/dbrow.h"
#include "framework/database/dberrors.h"
#include "EASTL/string.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class MySqlPreparedStatementResult : public DbResult
{
    NON_COPYABLE(MySqlPreparedStatementResult);

public:
    MySqlPreparedStatementResult() {}
    ~MySqlPreparedStatementResult() override;

    BlazeRpcError init(MYSQL_STMT* stmt, MYSQL_RES* meta, MYSQL_BIND* bind, MySqlPreparedStatementRow::ResultDataList& rowData);
    BlazeError initAsync(MySqlAsyncConn& conn, const char8_t* queryDesc, MYSQL_STMT* stmt, MYSQL_RES* meta, MYSQL_BIND* bind, MySqlPreparedStatementRow::ResultDataList& rowData, TimeValue absoluteTimeout);

    uint32_t getAffectedRowCount() const override { return mRowAffectedCount; }
    uint64_t getLastInsertId() const override { return mLastInsertId; }

    const char8_t* toString(eastl::string& buf,
            int32_t maxStringLen = 0, int32_t maxBinaryLen = 0) const override;


private:
    const RowList &getRowList() const override { return mRows; }

private:

    uint32_t mRowAffectedCount;
    uint64_t mLastInsertId;

    RowList mRows;
};

}// Blaze
#endif // BLAZE_MYSQLPREPAREDSTATEMENTRESULT_H

