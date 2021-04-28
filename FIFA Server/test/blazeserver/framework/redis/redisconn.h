/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_CONN_H
#define BLAZE_REDIS_CONN_H

/*** Include files *******************************************************************************/

#include "EASTL/intrusive_list.h"
#include "framework/tdf/frameworkconfigdefinitions_server.h"
#include "framework/system/frameworkresource.h"
#include "framework/protocol/shared/heat2encoder.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/redis/rediscommand.h"
#include "framework/redis/redisresponse.h"
#include "framework/redis/redisscriptregistry.h"
#include "framework/util/dispatcher.h"
#include "framework/redis/rediscluster.h"
#include "hiredis/async.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#define REDIS_CONN_ERROR(error, description) ((gRedisManager->setLastError(error) << "[" << mHostAddress << ", '" << mNodeName.c_str() << "'] " <<  description).get() ? error : REDIS_ERR_SYSTEM)

namespace Blaze
{

class RedisConn : public Channel, protected ChannelHandler
{
public:
    typedef Functor2<RedisResponsePtr, RedisError> ResponseCb;

public:
    RedisConn(const InetAddress &addr, const char8_t* nodeName, const char8_t *password, RedisCluster& redisCluster);
    ~RedisConn() override;

    RedisError connect();
    bool disconnect();

    bool isConnected() const { return (mState == STATE_CONNECTED); }

    const InetAddress &getHostAddress() const { return mHostAddress; }

    /*! ************************************************************************************************/
    /*!
        \brief Sends the command to the Redis server, and returns immediately.

        \param[in] command - The command to send to the Redis server.

        \return ERR_OK if the command was sent successfully, otherwise an error.
    ****************************************************************************************************/
    RedisError send(const RedisCommand &command, RedisResponsePtr &response);

    /*! ************************************************************************************************/
    /*!
        \brief Sends the command to the Redis server, and blocks until a response is received.

        \param[in] command - The command to send to the Redis server.
        \param[out] response - If no error occurs while sending the command or receiving the response, 
                               the response contains a RedisResponse object populated with RESP data.
                               (See https://redis.io/topics/protocol for details about the possible RESP data types.)
        \param[out] redirectAddress - If the response is an -ASK or -MOVED error, the redirectAddress will be
                                populated with the ip and port of the node that the command should be redirected to.

        \return REDIS_ERR_OK is returned if a response was received and the response type is not a RESP error.

               If the response type is a RESP error, then the corresponding RedisError is returned if applicable;
               otherwise, REDIS_ERR_COMMAND_FAILED is returned.
    ****************************************************************************************************/
    RedisError exec(const RedisCommand &command, RedisResponsePtr &response, InetAddress* redirectAddress);

    RedisError refreshServerInfo();
    RedisError verifyNodeName();

    const char8_t *getServerInfo(const char8_t *sectionName, const char8_t *valueName, const char8_t *defaultValue = "") const;

    bool getReadOnly() const { return mReadOnly; }

    ChannelHandle getHandle() override { return mSocketFd; }

    RedisCluster& getRedisCluster() const { return mRedisCluster; }
    const char8_t* getNodeName() const { return mNodeName.c_str(); }

protected:
    friend class RedisCluster;
    RedisError makeScriptCommand(const RedisScript &redisScript, const RedisCommand &command, RedisCommand &scriptCommand);
    void setReadOnly(bool readOnly) { mReadOnly = readOnly; }

protected:
    // ChannelHandler interface
    void onRead(Channel& channel) override;
    void onWrite(Channel& channel) override;
    void onError(Channel& channel) override;
    void onClose(Channel& channel) override;

private:
    friend class RedisScriptRegistry;

    bool disconnectInternal();

    // Helper functions
    RedisError loadScript(const RedisScript &redisScript);

    bool isBackgroundReconnectEnabled() const { return (Fiber::isFiberIdInUse(mReconnectFiberId) || (mReconnectTimerId != INVALID_TIMER_ID)); }
    void enableBackgroundReconnect();
    void disableBackgroundReconnect();
    void backgroundReconnect();

    // Callback functions to work with the Hiredis Async API.
    static void redisAsync_EventAddRead(void *privdata);
    static void redisAsync_EventDeleteRead(void *privdata);
    static void redisAsync_EventAddWrite(void *privdata);
    static void redisAsync_EventDeleteWrite(void *privdata);
    static void redisAsync_EventCleanup(void *privdata);

    static void redisAsync_ConnectCallback(const redisAsyncContext *context, int32_t status);
    static void redisAsync_DisconnectCallback(const redisAsyncContext *context, int32_t status);

    static void redisAsync_CommandCallback(redisAsyncContext *context, void *reply, void *privdata);

private:
    static const int32_t CONNECTION_CHECK_INTERVAL_MS;
    static const uint16_t MAX_FAILED_RECONNECTS;

    InetAddress mHostAddress;
    eastl::string mPassword;
    eastl::string mNodeName;

    RedisCluster& mRedisCluster;
    redisAsyncContext *mAsyncContext;

    enum State
    {
        STATE_DISCONNECTED,
        STATE_CONNECTING,
        STATE_CONNECTED,
        STATE_DISCONNECTING
    };
    static const char8_t* StateToString(State state);

    State mState;
    ChannelHandle mSocketFd;

    Fiber::EventHandle mConnectEvent;
    Fiber::EventHandle mDisconnectEvent;

    Fiber::FiberId mReconnectFiberId;
    TimerId mReconnectTimerId;

    typedef eastl::hash_map<eastl::string, eastl::string, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> ValueByNameMap;
    typedef eastl::hash_map<eastl::string, ValueByNameMap, CaseInsensitiveStringHash, CaseInsensitiveStringEqualTo> ValueByNameMapToSectionNameMap;
    ValueByNameMapToSectionNameMap mServerInfoMap;

    bool mIsClusterEnabled;
    bool mReadOnly;

    struct ScriptInfo
    {
        eastl::string sha1;
        int32_t numKeys;
    };
    typedef eastl::hash_map<RedisScriptId, ScriptInfo> ScriptIdToInfoMap;
    ScriptIdToInfoMap mScriptInfoMap;
};

} // Blaze

#endif // BLAZE_REDIS_CONN_H
