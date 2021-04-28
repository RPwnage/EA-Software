/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_MYSQL_ASYNC_CONN_H
#define BLAZE_MYSQL_ASYNC_CONN_H

/*** Include files *******************************************************************************/

#include <mariadb/mysql.h>
#include "EASTL/intrusive_list.h"
#include "EASTL/fixed_function.h"
#include "framework/database/dbconninterface.h"
#include "framework/database/query.h"
#include "framework/database/dbresult.h"


/*** Defines/Macros/Constants/Typedefs ***********************************************************/

// References
//  https://mariadb.com/kb/en/library/non-blocking-api-reference/
//  https://mariadb.com/kb/en/library/using-the-non-blocking-library/

namespace Blaze
{

class DbConnConfig;
class DbConfig;
class DbConnBase;
class MySqlAsyncStreamingResult;

class MySqlAsyncConn : public DbConnInterface, public Channel, protected ChannelHandler
{
    friend class MySqlAsyncAdmin;
    friend class MySqlResult;
    friend class MySqlAsyncStreamingResult;
    friend class MySqlPreparedStatement;
    friend class MySqlPreparedStatementResult;
public:
    MySqlAsyncConn(const DbConfig& config, DbConnBase& owner);
    ~MySqlAsyncConn() override;

    Query* newQuery(const char8_t* comment) override;
    bool isConnected() const override;
    BlazeRpcError connect() override;
    BlazeRpcError testConnect(const DbConfig& config) override;
    void disconnect(BlazeRpcError reason) override;
    BlazeRpcError executeQuery(const Query* query, DbResult** result, TimeValue timeout) override;
    BlazeRpcError executeStreamingQuery(const Query* query, StreamingDbResult** result, TimeValue timeout) override;
    BlazeRpcError executeMultiQuery(const Query* query, DbResults** results, TimeValue timeout) override;
    BlazeRpcError executePreparedStatement(PreparedStatement* statement, DbResult** result, TimeValue timeout) override;
    BlazeRpcError startTxn(TimeValue timeout) override;
    BlazeRpcError commit(TimeValue timeout) override;
    BlazeRpcError rollback(TimeValue timeout) override;
    PreparedStatement* createPreparedStatement(const PreparedStatementInfo& info) override;
    const char8_t* getLastErrorMessageText() const override;
    void closeStatement(MYSQL_STMT* stmt) override;
    void assign() override;
    void release() override;

private:
    // Size of function capture argument space (increase if needed)
    static const uint32_t FIXED_FUNCTION_SIZE_BYTES = 32;

    BlazeError executeQueryInternal(const Query* queryPtr, DbResults** dbResults, bool isMultiQuery, TimeValue timeout);
    BlazeError executeStreamingQueryInternal(const Query* queryPtr, StreamingDbResult** streamingResult, TimeValue timeout);
    BlazeRpcError connectImpl(const DbConfig& config, bool isTest);
    void initialize(const DbConnConfig& config);
    void cleanupStreamingResult(MySqlAsyncStreamingResult* result);
    void scheduleKill();
    BlazeRpcError ping();
    BlazeRpcError kill(uint64_t threadId);
    BlazeError performBlockingOp(const char8_t* callerDesc, const char8_t* queryDesc, const TimeValue& absoluteTimeout, const eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, int32_t(void)>& startFunc, const eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, int32_t(uint32_t)>& contFunc);
    BlazeError executeWithRetry(const eastl::fixed_function<FIXED_FUNCTION_SIZE_BYTES, BlazeError(void)>& func);
    ChannelHandle getHandle() override;
    void onRead(Channel& channel) override;
    void onWrite(Channel& channel) override;
    void onError(Channel& channel) override;
    void onClose(Channel& channel) override;

private:
    enum State
    {
        STATE_DISCONNECTED,
        STATE_CONNECTING,
        STATE_CONNECTED,
        STATE_DISCONNECTING
    };

    InetAddress mHostAddress;

    MYSQL mMySql;
    uint64_t mMySqlThreadId;
    BlazeRpcError mDisconnectReason;
    bool mRetryPending;
    bool mScheduledKill;
    bool mInitialized;

    State mState;
    uint32_t mPollFlags;
    uint32_t mNameResolverJobId;
    ChannelHandle mChannelHandle; // we need to cache the socket handle instead of relying on mysql_get_socket(&mMySqlContext) because MYSQL client lib frees the socket data structure immediately on connection failure without clearing it making it impossible to access the socket handle safely in case the connection setup fails...
    Fiber::EventHandle mPollEvent;
    const char8_t* mPendingCommand;

    eastl::hash_set<MySqlAsyncStreamingResult*> mStreamingResults;

    eastl::string mLastErrorText;

};

} // Blaze

#endif // BLAZE_MYSQL_API_H
