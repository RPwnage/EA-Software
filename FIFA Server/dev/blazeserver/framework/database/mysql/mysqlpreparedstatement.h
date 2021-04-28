/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MYSQLPREPAREDSTATEMENT_H
#define BLAZE_MYSQLPREPAREDSTATEMENT_H

/*** Include files *******************************************************************************/

#include "framework/database/preparedstatement.h"
#include "framework/database/mysql/mysqlpreparedstatementrow.h"
#include <mariadb/mysql.h>

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class DbResult;
class MySqlConn;
class MySqlAsyncConn;

class MySqlPreparedStatement : public PreparedStatement
{
    friend class MySqlConn;

    NON_COPYABLE(MySqlPreparedStatement);

public:
    MySqlPreparedStatement(const PreparedStatementInfo& info);
    ~MySqlPreparedStatement() override;

private:
    MYSQL_STMT* mStatement;
    MYSQL_BIND* mBindings;
    MYSQL_BIND* mResultBindings;
    MySqlPreparedStatementRow::ResultDataList mRowData;

    void setupBindParams(size_t paramCount);
    void setupBindResults(MYSQL_FIELD* fields, uint32_t numFields);
    void clearResultBindings();
    void clearAllBindings();

    BlazeRpcError execute(DbResult** result, MySqlConn& conn) override;
    BlazeRpcError execute(DbResult** result, MySqlAsyncConn& conn, TimeValue timeout = 0) override;
    void close(DbConnInterface& conn) override;
};

} // Blaze

#endif // BLAZE_MYSQLPREPAREDSTATEMENT_H

