/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class Redis

    Provides support to connect to a Redis server, issue simple get, set and del commands, and
    create a pool of connections.  Each connection also makes its redisContext available for
    direct use.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "framework/redis/redis.h"
#include "framework/redis/redisconn.h"
#include "framework/redis/redisscriptregistry.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/controller/controller.h"
#include "framework/redis/redismanager.h"

namespace Blaze
{

static void noOpFreeRedisReply(void*) {}

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

const int32_t RedisConn::CONNECTION_CHECK_INTERVAL_MS = 500;
const uint16_t RedisConn::MAX_FAILED_RECONNECTS = 6;

RedisConn::RedisConn(const InetAddress &addr, const char8_t* nodeName, const char8_t *password, RedisCluster& redisCluster)
  : mHostAddress(addr),
    mPassword(password),
    mNodeName(nodeName),
    mRedisCluster(redisCluster),
    mAsyncContext(nullptr),
    mState(STATE_DISCONNECTED),
    mSocketFd(INVALID_SOCKET),
    mReconnectFiberId(Fiber::INVALID_FIBER_ID),
    mReconnectTimerId(INVALID_TIMER_ID),
    mServerInfoMap(BlazeStlAllocator("RedisConn::mServerInfoMap")),
    mIsClusterEnabled(false),
    mReadOnly(false),
    mScriptInfoMap(BlazeStlAllocator("RedisConn::mScriptInfoMap"))
{
    enablePriorityChannel();
}

RedisConn::~RedisConn()
{
    EA_ASSERT_MSG(mState == STATE_DISCONNECTED && mReconnectFiberId == Fiber::INVALID_FIBER_ID, "RedisConn::disconnect() should be called before deleting this object");
}

RedisError RedisConn::connect()
{
    if (mState != STATE_DISCONNECTED)
    {
        return REDIS_CONN_ERROR(REDIS_ERR_ALREADY_CONNECTED, "RedisConn.connect: "
            "Connect called on RedisConn that is in state " << StateToString(mState));
    }

    if (!setHandler(this))
    {
        return REDIS_CONN_ERROR(REDIS_ERR_SYSTEM, "RedisConn.connect: setHandler() failed.");
    }

    {
        // Scoped for the blocking suppression
        Fiber::BlockingSuppressionAutoPtr noblock;

        // Call the Redis API to begin the connection process
        // Important, we need to pass in the actual IP address (not a hostname) in order for this call to *not* block.
        mAsyncContext = redisAsyncConnect(mHostAddress.getIpAsString(), mHostAddress.getPort(InetAddress::HOST));
    }

    if (mAsyncContext == nullptr)
    {
        return REDIS_CONN_ERROR(REDIS_ERR_SYSTEM, "RedisConn.connect: Call to redisAsyncConnect() failed");
    }

    if (mAsyncContext->err != REDIS_OK)
    {
        RedisError err = REDIS_CONN_ERROR(REDIS_ERR_SYSTEM, "RedisConn.connect: Call to redisAsyncConnect() failed: err("
            << mAsyncContext->err << "), errstr(" << mAsyncContext->errstr << ")");

        redisAsyncFree(mAsyncContext);

        return err;
    }

    // At this point we know we have a socket, so let register ourselves with the selector.
    mSocketFd = static_cast<ChannelHandle>(mAsyncContext->c.fd);
    registerChannel(Channel::OP_NONE);

    // Setup the async event callbacks.  These will get called when the Hiredis lib
    // needs to tell us to add/remove something to/from our Selector.
    mAsyncContext->ev.data = this;
    mAsyncContext->ev.addRead = redisAsync_EventAddRead;
    mAsyncContext->ev.delRead = redisAsync_EventDeleteRead;
    mAsyncContext->ev.addWrite = redisAsync_EventAddWrite;
    mAsyncContext->ev.delWrite = redisAsync_EventDeleteWrite;
    mAsyncContext->ev.cleanup = redisAsync_EventCleanup;

    // Override the freeObject() function pointer to point to a no-op function.
    // This is to avoid letting the Hiredis lib delete memory immediately following
    // a callback.  And instead, we can control the lifetime of redis objects.
    mAsyncContext->c.reader->fn->freeObject = noOpFreeRedisReply;

    redisAsyncSetConnectCallback(mAsyncContext, RedisConn::redisAsync_ConnectCallback);
    redisAsyncSetDisconnectCallback(mAsyncContext, RedisConn::redisAsync_DisconnectCallback);

    // We're about to make our first blocking call, so update the state.
    mState = STATE_CONNECTING;

    // We'll wake up when something happens on the socket
    BlazeRpcError waitErr = Fiber::getAndWait(mConnectEvent, "RedisConn::connect");
    if (waitErr != ERR_OK)
    {
        // If we're in here, then a framework level cancellation of the wait occurred, rather than a
        // controlled signaling of this fiber from somewhere in this class which knows about our
        // state machine.  So that means, we must still be in the STATE_CONNECTING state.
        EA_ASSERT(mState == STATE_CONNECTING);

        // Clean things up manually.
        redisAsyncFree(mAsyncContext);
        mAsyncContext = nullptr;
        unregisterChannel(true);
        mSocketFd = INVALID_SOCKET;

        mState = STATE_DISCONNECTED;

        return REDIS_CONN_ERROR(RedisErrorHelper::convertError(waitErr), "RedisConn.connect: Call to Fiber::getAndWait() failed: err("
            << ErrorHelp::getErrorName(waitErr) << ")");
    }

    // If our call to getAndWait() return ERR_OK, then our state should now be updated
    // based on whatever happened during the connection attempt.  The state will
    // either be STATE_CONNECTED or STATE_DISCONNECTED.  It should never be STATE_CONNECTING at this point.
    EA_ASSERT(mState != STATE_CONNECTING);

    RedisError err;
    if (mState == STATE_DISCONNECTED)
    {
        err = REDIS_CONN_ERROR(REDIS_ERR_CONNECT_FAILED, "RedisConn.connect: Could not connect to Redis server: err("
            << mAsyncContext->err << "), errstr(" << mAsyncContext->errstr << ")");

        // Clean things up manually.
        redisAsyncFree(mAsyncContext);
        mAsyncContext = nullptr;
        unregisterChannel(true);
        mSocketFd = INVALID_SOCKET;

        return err;
    }

    // At this point, all other scenarios have been handled, and here we must be connected and we're free to carry on.
    EA_ASSERT(mState == STATE_CONNECTED);

    err = refreshServerInfo();
    if (err != REDIS_ERR_OK)
    {
        disconnectInternal();
        return REDIS_CONN_ERROR(err, "RedisConn.connect: Failed to retrieve Redis server info.");
    }

    if (!mIsClusterEnabled)
    {
        // BITC_TODO Fill in Blaze version
        // Starting with Blaze version TBA, we require all redis instances to run in cluster mode

        disconnectInternal();
        return REDIS_CONN_ERROR(REDIS_ERR_SYSTEM, "RedisConn.connect: "
            "The Redis node is not configured as part of a Redis cluster, which is not supported by this version of Blaze server.");
    }

    err = verifyNodeName();
    if (err != REDIS_ERR_OK)
    {
        disconnectInternal();
        return REDIS_CONN_ERROR(err, "RedisConn.connect: Failed to verify Redis node name.");
    }

    if (!mPassword.empty())
    {
        // If a password was provided, use it to authenticate
        RedisCommand authCommand;
        RedisResponsePtr authResponse;

        authCommand.add("AUTH").add(mPassword);
        err = exec(authCommand, authResponse, nullptr);
        if (err != REDIS_ERR_OK)
        {
            disconnectInternal();
            return REDIS_CONN_ERROR(err, "RedisConn.connect: Authentication failed with Redis node.");
        }
    }

    err = RedisScriptRegistry::loadRegisteredScripts(this);
    if (err != REDIS_ERR_OK)
    {
        disconnectInternal();
        return REDIS_CONN_ERROR(err, "RedisConn.connect: Failed to load all of the registered Lua scripts.");
    }

    // Kick off the background reconnect fiber
    enableBackgroundReconnect();

    return err;
}

void RedisConn::enableBackgroundReconnect()
{
    if (isBackgroundReconnectEnabled())
        return;

    mReconnectTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + (CONNECTION_CHECK_INTERVAL_MS * 1000),
        this, &RedisConn::backgroundReconnect, "RedisConn::backgroundReconnect");
}

void RedisConn::disableBackgroundReconnect()
{
    if (!isBackgroundReconnectEnabled())
        return;

    Fiber::cancel(mReconnectFiberId, ERR_CANCELED);

    // The RedisConn::backgroundReconnect() method will *always* reschedule itself.  It makes no assumption or determination regarding
    // how many times it needs to run.  Therefore, it's important that the last thing this method does is to cancels the timer that would have been
    // scheduled just before RedisConn::backgroundReconnect() return'ed.
    gSelector->cancelTimer(mReconnectTimerId);
    mReconnectTimerId = INVALID_TIMER_ID;
}

bool RedisConn::disconnect()
{
    // First, kill the reconnect fiber.
    disableBackgroundReconnect();

    // Then, call the real disconnect which actually performs the socket disconnect.
    return disconnectInternal();
}

bool RedisConn::disconnectInternal()
{
    switch (mState)
    {
    case STATE_DISCONNECTED:
        // Nothing to do if we're not connected.
        return true;

    case STATE_DISCONNECTING:
        // Already disconnecting.  Return false and let whoever is disconnecting finish doing his thing.
        return false;

    case STATE_CONNECTING:
        // This can happen if the selector reported an error to us while attempting a connection to the redis server.
        // Note, we set the state to STATE_DISCONNECTED here, and pass ERR_OK to signal() intentionally.  ERR_OK here
        // means "we are still in control of the connection flow", and the state is used to indicate where we're at.
        mState = STATE_DISCONNECTED;
        if (mConnectEvent.isValid())
            Fiber::signal(mConnectEvent, ERR_OK);
        return true;

    case STATE_CONNECTED:
        // If we're connected, we must have a valid mAsyncContext.
        EA_ASSERT(mAsyncContext != nullptr);

        // Set the state to STATE_DISCONNECTING, this will eventually get set to STATE_DISCONNECTED in redisAsync_DisconnectCallback().
        mState = STATE_DISCONNECTING;

        // Disconnection Flow...
        // The call to redisAsyncDisconnect() is non-blocking.  It will take one of two paths
        // 1. If all responses from redis have been received and read off of the socket, then it will immediately call
        //    all of the pending callbacks for those responses, and completely close the socket and destroy mAsyncContext.
        // 2. On the other hand, if there are commands that have been sent to redis, for which no responses have yet been received
        //    then it will put the mAsyncContext into a 'disconnecting' state.  In this case, mAsyncContext is still valid
        //    after redisAsyncContext() returns. The Selector is expected to continue to call onRead()/onWrite() to fully drain any oustanding
        //    data on the socket in/out buffers.
        //
        // In both scenarios, the last thing that will get called is the redisAsync_DisconnectCallback().  That is where we set the
        // state to STATE_DISCONNECTED.
        redisAsyncDisconnect(mAsyncContext);

        // The call to redisAsyncDisconnect() may disconnect immediately, or there may be some more
        // processing that needs to be done.  If it disconnects immediately, mState will be set to
        // STATE_DISCONNECTED.  Otherwise, we wait for the disconnection to complete.
        if (mState == STATE_DISCONNECTING)
        {
            BlazeRpcError err = ERR_SYSTEM;
            if (Fiber::isCurrentlyBlockable())
                err = Fiber::getAndWait(mDisconnectEvent, "RedisConn::disconnectInternal()");
            if (err != ERR_OK)
            {
                // We got here because the framework decided to cancel the wait before Hiredis had time to
                // call the redisAsync_DisconnectCallback() event.  And so, we can't really do anything here except ungracefully
                // kill everything, and carry on.
                redisAsyncFree(mAsyncContext);
                mState = STATE_DISCONNECTED;

                BLAZE_ERR_LOG(Log::REDIS, "RedisConn.disconnect: Failed to disconnect gracefully from Redis with "
                    "err(" << ErrorHelp::getErrorName(err) << "), aborted connection(" << mHostAddress << "), context: " << Fiber::getCurrentContext());
            }
        }

        EA_ASSERT(mState == STATE_DISCONNECTED);

        return true;
    }

    return true;
}

void RedisConn::backgroundReconnect()
{
    mReconnectFiberId = Fiber::getCurrentFiberId();

    RedisError redisErr = connect();

    if (redisErr != REDIS_ERR_OK && redisErr != REDIS_ERR_ALREADY_CONNECTED)
    {
        // Call updateSlotsConfiguration to check whether the node has been removed.
        BLAZE_INFO_LOG(Log::REDIS, "RedisConn.backgroundReconnect: Failed to connect to redis node '" << mNodeName << "' at " << mHostAddress.getHostname() << " (" << mHostAddress << "); error " 
            << RedisErrorHelper::getName(redisErr) << ". Slot mappings will be updated (this node may have been removed).");

        mRedisCluster.scheduleUpdateSlotsConfiguration();
    }

    // This function will *always* reschedule itself.  It's up to the guy that started this 'background reconnect' process
    // to ensure that it is stopped.  See disableBackgroundReconnect().
    mReconnectTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + (CONNECTION_CHECK_INTERVAL_MS * 1000),
        this, &RedisConn::backgroundReconnect, "RedisConn::backgroundReconnect");

    mReconnectFiberId = Fiber::INVALID_FIBER_ID;
}

RedisError RedisConn::send(const RedisCommand &command, RedisResponsePtr &response)
{
    if (mState != STATE_CONNECTED)
        return REDIS_CONN_ERROR(REDIS_ERR_NOT_CONNECTED, "RedisConn.send: Connection state is " << StateToString(mState));

    response = BLAZE_NEW_MGID(MEM_GROUP_FRAMEWORK_REDIS, "Blaze::RedisResponse") RedisResponse();

    // Send the command.  Our redisAsync_CommandCallback() callback will be called when a response is received
    int32_t rc = redisAsyncCommandArgv(mAsyncContext, redisAsync_CommandCallback, response.get(), command.getParamCount(), command.getParams(), command.getParamLengths());

    if (rc != REDIS_OK)
    {
        // If the command could not be sent, delete the callback data.
        response = nullptr;

        return REDIS_CONN_ERROR(REDIS_ERR_SEND_COMMAND_FAILED, "RedisConn.send: Failed to send the command to Redis: "
            "rc(" << rc << "), err(" << mAsyncContext->err << "), errstr(" << mAsyncContext->errstr << ")");
    }

    response->AddRef();

    return REDIS_ERR_OK;
}

RedisError RedisConn::exec(const RedisCommand &command, RedisResponsePtr &response, InetAddress* redirectAddress)
{
    RedisError err = send(command, response);
    if (err == REDIS_ERR_OK)
        err = response->waitForReply(); // Wait for the response.  The waitHandle will be signalled in redisAsync_CommandCallback().

    bool hasErrorDescription = (err != REDIS_ERR_OK);
    if (err == REDIS_ERR_OK)
    {
        if (response->isError())
        {
            if (blaze_strnicmp(response->getString(), "MOVED", sizeof("MOVED") - 1) == 0)
            {
                err = REDIS_ERR_MOVED;
            }
            else if (blaze_strnicmp(response->getString(), "ASK", sizeof("ASK") - 1) == 0)
            {
                err = REDIS_ERR_ASK;
            }
            else if (blaze_strnicmp(response->getString(), "TRYAGAIN", sizeof("TRYAGAIN") - 1) == 0)
            {
                err = REDIS_ERR_TRYAGAIN;
            }
            else if (blaze_strnicmp(response->getString(), "LOADING", sizeof("LOADING") - 1) == 0)
            {
                err = REDIS_ERR_LOADING;
            }
            else if (blaze_strnicmp(response->getString(), "CROSSSLOT", sizeof("CROSSSLOT") - 1) == 0)
            {
                err = REDIS_ERR_CROSSSLOT;
            }
            else if (blaze_strnicmp(response->getString(), "CLUSTERDOWN", sizeof("CLUSTERDOWN") - 1) == 0)
            {
                err = REDIS_ERR_CLUSTERDOWN;
            }
            else
            {
                err = REDIS_ERR_COMMAND_FAILED;
            }

            if (RedisCluster::shouldRedirectCommand(err))
            {
                if (redirectAddress == nullptr)
                    err = REDIS_CONN_ERROR(REDIS_ERR_SYSTEM, "-ASK/-MOVED was returned by a command that was expected to be slot-agnostic");
                else if (!RedisCluster::getAddressFromRedirectionError(response->getString(), response->getStringLen(), *redirectAddress))
                    err = REDIS_CONN_ERROR(REDIS_ERR_SYSTEM, "RedisConn.exec: Failed to parse address from -ASK/-MOVED response");
                hasErrorDescription = (err == REDIS_ERR_SYSTEM);
            }

            // Set last error description, so that callers of exec() can log it
            if (!hasErrorDescription)
                REDIS_CONN_ERROR(err, "RedisConn.exec: Command returned error reply: " << response->getString());
        }
    }

    mRedisCluster.updateMetricValues(command, response, err);
    if (RedisCluster::shouldUpdateSlotsConfiguration(err))
        mRedisCluster.scheduleUpdateSlotsConfiguration();

    Logging::Level errorLevel = RedisCluster::getErrorLevel(err);
    if (Logger::isEnabled(Log::REDIS, errorLevel, __FILE__, __LINE__))
    {
        eastl::string details;
        details += "Command -> ";
        command.encode(details);
        details.append_sprintf(", conn=%s:%u, node='%s'", mHostAddress.getHostname(), mHostAddress.getPort(InetAddress::Order::HOST), mNodeName.c_str());
        details += ", Rc = ";
        details.append_sprintf("%" PRIi32, (int32_t)err.error);
        details += ", Response = ";
        if (response != nullptr)
            response->encode(details);
        else
            details += "(null)";
        details += ", Err = ";
        details += RedisErrorHelper::getName(err);
        if (hasErrorDescription)
        {
            details += ", ErrDescription = ";
            details += gRedisManager->getLastError().getLastErrorDescription();
        }

        if (errorLevel == Logging::Level::FAIL)
        {
            BLAZE_ASSERT_LOG(Log::REDIS, "RedisConn.exec: " << details);
        }
        else
        {
            BLAZE_LOG(errorLevel, Log::REDIS, "RedisConn.exec: " << details);
        }
    }
    return err;
}

/*** Protected Methods *****************************************************************************/

void RedisConn::onRead(Channel& channel)
{
    EA_ASSERT(&channel == this);
    redisAsyncHandleRead(mAsyncContext);
}

void RedisConn::onWrite(Channel& channel)
{
    EA_ASSERT(&channel == this);
    redisAsyncHandleWrite(mAsyncContext);
}

void RedisConn::onError(Channel& channel)
{
    EA_ASSERT(&channel == this);
    disconnectInternal();
}

void RedisConn::onClose(Channel& channel)
{
    EA_ASSERT(&channel == this);
    disconnectInternal();
}

/*** Private Methods *****************************************************************************/

void RedisConn::redisAsync_EventAddRead(void *privdata)
{
    RedisConn *myself = (RedisConn*)privdata;
    myself->addInterestOp(Channel::OP_READ);
}

void RedisConn::redisAsync_EventDeleteRead(void *privdata)
{
    RedisConn *myself = (RedisConn*)privdata;
    myself->removeInterestOp(Channel::OP_READ);
}

void RedisConn::redisAsync_EventAddWrite(void *privdata)
{
    RedisConn *myself = (RedisConn*)privdata;
    myself->addInterestOp(Channel::OP_WRITE);
}

void RedisConn::redisAsync_EventDeleteWrite(void *privdata)
{
    RedisConn *myself = (RedisConn*)privdata;
    myself->removeInterestOp(Channel::OP_WRITE);
}

void RedisConn::redisAsync_EventCleanup(void *privdata)
{
    RedisConn *myself = (RedisConn*)privdata;
    myself->setInterestOps(Channel::OP_NONE);
}

void RedisConn::redisAsync_CommandCallback(redisAsyncContext *context, void *r, void *privdata)
{
    RedisResponse *response = (RedisResponse*)privdata;

    response->setRedisReplyInfo(context, (redisReply*)r, true);

    response->signalCompletion(context->err == REDIS_OK ? ERR_OK : ERR_SYSTEM);

    response->Release();
}

void RedisConn::redisAsync_ConnectCallback(const redisAsyncContext *context, int32_t status)
{
    RedisConn *myself = (RedisConn*)context->ev.data;

    if (status == REDIS_OK)
    {
        // We're connected.
        myself->mState = STATE_CONNECTED;
    }
    else
    {
        // We're not connected.  This fact will be picked up in the connect() method.
        myself->mState = STATE_DISCONNECTED;
    }

    // Signal the waiting connect() method, if it is still waiting.
    if (myself->mConnectEvent.isValid())
        Fiber::signal(myself->mConnectEvent, ERR_OK);
}

void RedisConn::redisAsync_DisconnectCallback(const redisAsyncContext *context, int32_t status)
{
    RedisConn *myself = (RedisConn*)context->ev.data;

    // At this point, redisAsync_DisconnectCallback() has been called - either immediately within redisAsyncDisconnect(), or from
    // within redisAsyncHandleRead() once the Hiredis lib is finished reading all available data from the socket.
    // In either case, by this time, Hiredis has free'd the memory pointed to by mAsyncContext.
    myself->mAsyncContext = nullptr;

    // This call will reference the socket fd in order to pull it from the select/epoll fd list.  So, it must remain valid
    // until after this call is made.
    myself->unregisterChannel(false);
    myself->mSocketFd = INVALID_SOCKET;

    // Not much can go wrong on a disconnect.  Just mark the state as disconnected.
    myself->mState = STATE_DISCONNECTED;

    // Signal the waiting disconnect() method, if it is still waiting.
    if (myself->mDisconnectEvent.isValid())
        Fiber::signal(myself->mDisconnectEvent, ERR_OK);
}

RedisError RedisConn::loadScript(const RedisScript &redisScript)
{
    RedisError err = REDIS_ERR_SYSTEM;

    if (redisScript.getId() != RedisScript::INVALID_SCRIPT_ID)
    {
        RedisCommand cmd;
        cmd.add("SCRIPT").add("LOAD").add(redisScript.getScript());

        RedisResponsePtr resp;
        err = exec(cmd, resp, nullptr);
        if (err == REDIS_ERR_OK)
        {
            ScriptInfo &scriptInfo = mScriptInfoMap[redisScript.getId()];
            scriptInfo.numKeys = redisScript.getNumberOfKeys();
            scriptInfo.sha1 = resp->getString();
        }
    }

    return err;
}

RedisError RedisConn::makeScriptCommand(const RedisScript &redisScript, const RedisCommand &command, RedisCommand &scriptCommand)
{
    ScriptIdToInfoMap::iterator it = mScriptInfoMap.find(redisScript.getId());
    if (it != mScriptInfoMap.end())
    {
        ScriptInfo &scriptInfo = it->second;

        scriptCommand.add("EVALSHA").add(scriptInfo.sha1).add(scriptInfo.numKeys).add(command);

        return REDIS_ERR_OK;
    }

    return REDIS_CONN_ERROR(REDIS_ERR_SYSTEM, "RedisConn.makeScriptCommand: RedisScript not found in mScriptInfoMap -- script is either unregistered or has an invalid ScriptId");
}

RedisError RedisConn::verifyNodeName()
{
    RedisCommand nameCommand;
    nameCommand.add("CLUSTER").add("MYID");
    RedisResponsePtr nameResp;
    RedisError err = exec(nameCommand, nameResp, nullptr);
    if (err == REDIS_ERR_OK)
    {
        const char8_t* nodeName = nameResp->getString();
        if (mNodeName.empty())
        {
            mNodeName = nodeName;
        }
        else if (blaze_stricmp(nodeName, mNodeName.c_str()) != 0)
        {
            // In the cloud, a redis cluster can be configured to allow redis nodes to be
            // restarted with a different ip address. Blaze does not support this,
            // so we prevent this RedisConn from successfully connecting if a different
            // redis node is now running at this ip and port.
            err = REDIS_ERR_SYSTEM;
            BLAZE_ASSERT_LOG(Log::REDIS, "RedisConn.verifyNodeName: Unexpected node (" << nodeName << ") is running at address " << mHostAddress << " (expected node " << mNodeName.c_str() << ")");
        }
    }
    return err;
}

RedisError RedisConn::refreshServerInfo()
{
    RedisCommand infoCommand;
    infoCommand.add("INFO").add("all");

    RedisResponsePtr infoResp;
    RedisError err = exec(infoCommand, infoResp, nullptr);
    if (err == REDIS_ERR_OK)
    {
        char8_t *infoStr = blaze_strdup(infoResp->getString(), MEM_GROUP_FRAMEWORK_REDIS);
        char8_t *infoStrPtr = nullptr;

        const char8_t *sectionName = "";
        char8_t *name = blaze_strtok(infoStr, "\r\n", &infoStrPtr);
        while (name != nullptr)
        {
            if (name[0] == '#')
            {
                ++name;
                while (*name == ' ')
                    ++name;
                sectionName = name;
            }
            else
            {
                char8_t * value = nullptr ;
                name = blaze_strtok (name , ":" , &value);
                if (name != nullptr)
                    mServerInfoMap [sectionName][name] = value ;
            }

            name = blaze_strtok(nullptr, "\r\n", &infoStrPtr);
        }

        BLAZE_FREE(infoStr);

        const char8_t* isClusterEnabled = getServerInfo("cluster", "cluster_enabled", "");
        mIsClusterEnabled = (strcmp(isClusterEnabled, "1") == 0);

        const char8_t* replicationRole = getServerInfo("replication", "role", "");
        mReadOnly = (strcmp(replicationRole, "master") != 0);
    }

    return err;
}

const char8_t *RedisConn::getServerInfo(const char8_t *sectionName, const char8_t *valueName, const char8_t *defaultValue) const
{
    ValueByNameMapToSectionNameMap::const_iterator foundSection = mServerInfoMap.find(sectionName);
    if (foundSection != mServerInfoMap.end())
    {
        const ValueByNameMap &valueMap = foundSection->second;

        ValueByNameMap::const_iterator foundValue = valueMap.find(valueName);
        if (foundValue != valueMap.end())
            return foundValue->second.c_str();
    }

    return defaultValue;
}

// static
const char8_t* RedisConn::StateToString(State state)
{
    switch (state)
    {
    case STATE_DISCONNECTED: return "STATE_DISCONNECTED";
    case STATE_CONNECTING: return "STATE_CONNECTING";
    case STATE_CONNECTED: return "STATE_CONNECTED";
    case STATE_DISCONNECTING: return "STATE_DISCONNECTING";
    default: return "unknown";
    }
}

} // Blaze
