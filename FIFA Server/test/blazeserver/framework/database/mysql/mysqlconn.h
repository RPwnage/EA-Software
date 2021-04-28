/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/
#ifndef BLAZE_MySqlConn_H
#define BLAZE_MySqlConn_H

/*** Include files *******************************************************************************/
#include "eathread/eathread_mutex.h"
#include "framework/database/dbconfig.h"
#include "framework/database/threadeddbconn.h"
#include "framework/database/mysql/blazemysql.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class DbInstancePool;
class MySqlResultBase;

class MySqlConn : public ThreadedDbConn
{
    NON_COPYABLE(MySqlConn);

    friend class MySqlResult;
    friend class MySqlStreamingResult;
    friend class MySqlPreparedStatement;
    friend class MySqlAdmin;
public:
    MySqlConn(const DbConfig& config, DbConnBase& owner);
    ~MySqlConn() override;

    Query* newQuery(const char8_t* fileAndLine) override;
    void releaseQuery(Query* query) const;

    const char8_t* getErrorString();

    MYSQL* getMySql() { return &mMySql; }

    static bool isLoggable(BlazeRpcError error);

    //  called to retrieve a row from a streaming result.
    typedef eastl::vector<MYSQL_ROW> MySqlRows;
protected:
    BlazeRpcError connectImpl() override;
    BlazeRpcError testConnectImpl(const DbConfig& config) override;
    void disconnectImpl(BlazeRpcError reason) override;
    BlazeRpcError executeQueryImpl(const char8_t* query, DbResult** result) override;
    BlazeRpcError executeStreamingQueryImpl(const char8_t* query, StreamingDbResult** result) override;
    BlazeRpcError fetchNextStreamingResultRowsImpl() override;
    BlazeRpcError freeStreamingResultsImpl() override;
    BlazeRpcError executeMultiQueryImpl(const char8_t* query, DbResults** results) override;
    BlazeRpcError executePreparedStatementImpl(PreparedStatement& stmt, DbResult** result) override;

    BlazeRpcError startTxnImpl() override;
    BlazeRpcError commitImpl() override;
    BlazeRpcError rollbackImpl() override;       
    PreparedStatement* createPreparedStatement(const PreparedStatementInfo& info) override;

    const char8_t* getLastErrorMessageText() const override { return mLastErrorText.c_str(); }
    void closeStatement(MYSQL_STMT* stmt) override { if (stmt != nullptr) { mysql_stmt_close(stmt); } }


private:
    MYSQL mMySql;

    typedef eastl::vector<MYSQL_RES*> MySqlResList;
    MySqlResList mStreamingResults;

    void initialize();
    BlazeRpcError freeResults();
    BlazeRpcError pullResult(const char8_t* query, MySqlResultBase& result, bool isStreaming);

    BlazeRpcError ping();
    BlazeRpcError executeQueryImplCommon(const char8_t* query, bool isStreaming, DbResult** result, StreamingDbResult** streamingResult);

    void initializeCommon(MYSQL& sql, const DbConfig& config) const;
    BlazeRpcError connectCommon(MYSQL& sql, const DbConfig& config, const char8_t* caller);

    bool mInitialized;

    eastl::string mLastErrorText;
};

}// Blaze
#endif // BLAZE_MySqlConn_H

