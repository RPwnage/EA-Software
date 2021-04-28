/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RedisCluster

    Provides support to connect to and issue commands to a cluster of Redis servers.  Commands are
    sent to a Redis server based on a hash code.

*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/controller/controller.h"
#include "framework/connection/selector.h"
#include "framework/util/shared/stringbuilder.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/util/random.h"

#include "framework/redis/rediscluster.h"
#include "framework/redis/redisunittests.h"
#include "framework/redis/rediscollection.h"
#include "framework/redis/redisconn.h"
#include "framework/redis/rediscommand.h"
#include "framework/redis/redisresponse.h"
#include "framework/redis/redisscriptregistry.h"
#include "framework/redis/redislockmanager.h"
#include "framework/redis/rediscollectionmanager.h"
#include "framework/redis/redisuniqueidmanager.h"
#include "framework/redis/redismanager.h"
#include "EASTL/functional.h"
#include "EAIO/EAFileBase.h"

#ifdef EA_PLATFORM_WINDOWS
    #include <direct.h>
    #define GetCurrentDir _getcwd
#else
    #include <unistd.h>
    #define GetCurrentDir getcwd
 #endif

namespace Blaze
{

// we don't need a header for this function, it's only intended to be used by RedisCluster internally
extern unsigned short redis_crc16(const char *buf, int len);

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

/*** Public Methods ******************************************************************************/

RedisCluster::RedisCluster(const char8_t* name)
:   mActive(false),
    mName(name),
    mUpdateSlotsConfigurationTimerId(INVALID_TIMER_ID),
    mCollectionManager(*(BLAZE_NEW RedisCollectionManager)), // must be constructed here because of dependency on redisCluster
    mUniqueIdManager(*(BLAZE_NEW RedisUniqueIdManager)), // must be constructed here because of dependency on redisCluster
    mLockIdCounter(0),
    mMetricsCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::cluster_name, name)),
    mCmdCount(mMetricsCollection, "redis.cmdCount"),
    mErrorCount(mMetricsCollection, "redis.errorCount", Metrics::Tag::redis_error),
    mBytesSent(mMetricsCollection, "redis.bytesSent"),
    mBytesRecv(mMetricsCollection, "redis.bytesRecv"),
    mAllNodes(mMetricsCollection, "redis.allNodes", [this]() { return mNodeByNodeNameMap.size(); })
{
}

RedisCluster::~RedisCluster()
{
    EA_ASSERT_MSG(!mActive, "Redis cluster must be deactivate first");
    SAFE_DELETE_REF(mUniqueIdManager);
    SAFE_DELETE_REF(mCollectionManager);
}

RedisError RedisCluster::initFromConfig(const RedisConfig& redisConfig, const RedisClusterConfig& config)
{
    config.copyInto(mClusterConfig);
    return activate(redisConfig);
}

RedisError RedisCluster::activate(const RedisConfig& redisConfig)
{
    if (mActive)
        return REDIS_ERR_OK;

    RedisError err = REDIS_ERR_SYSTEM;
    if (mClusterConfig.getNodes().empty())
    {
        BLAZE_FATAL_LOG(Log::REDIS, "RedisCluster[" << mName << "].activate: Invalid cluster configuration: nodes list is empty");
        return REDIS_ERR_INVALID_CONFIG;
    }

    uint16_t retriesLeft = redisConfig.getMaxActivationRetries();
    do {
        const RedisActivateStatus activateStatus = activateInternal(err);
        if (activateStatus == REDIS_ACTIVATE_SUCCESSFUL)
            break;
        else if (activateStatus == REDIS_ACTIVATE_ABORT)
            return err;

        if (retriesLeft-- > 0)
        {
            err = RedisErrorHelper::convertError(Fiber::sleep(gRedisManager->getRedisConfig().getUpdateSlotsDelay(), "RedisCluster::updateSlotsConfiguration"));
            if (err != REDIS_ERR_OK)
            {
                BLAZE_FATAL_LOG(Log::REDIS, "RedisCluster[" << mName << "].activate: Failed to update slots configuration; error(" << RedisErrorHelper::getName(err) << ") in Fiber::sleep");
                return err;
            }
        }
    } while (retriesLeft > 0);

    if (!mActive)
    {
        BLAZE_FATAL_LOG(Log::REDIS, "RedisCluster[" << mName << "].activate: Failed to activate redis cluster after " << redisConfig.getMaxActivationRetries() + 1
                << " attempt(s)");
        err = REDIS_ERR_SYSTEM;
    }

    return err;
}

RedisCluster::RedisActivateStatus RedisCluster::activateInternal(RedisError& err)
{
    for (RedisNodeConfigList::const_iterator nodeItr = mClusterConfig.getNodes().begin(); nodeItr != mClusterConfig.getNodes().end(); ++nodeItr)
    {
        InetAddress address(nodeItr->c_str());
        RedisError upsertErr = upsertNode(address, "");
        if (upsertErr != REDIS_ERR_OK)
        {
            BLAZE_WARN_LOG(Log::REDIS, "RedisCluster[" << mName << "].activate: Could not connect to configured node; error: " << RedisErrorHelper::getName(upsertErr) <<
                "; lastErrorDescription: " << gRedisManager->getLastError().getLastErrorDescription());
        }
    }

    err = updateSlotsConfiguration(true /*isActivating*/);
    if (err != REDIS_ERR_OK)
    {
        BLAZE_FATAL_LOG(Log::REDIS, "RedisCluster[" << mName << "].activate: Failed to update slots configuration; error: " << RedisErrorHelper::getName(err) <<
            "; lastErrorDescription: " << gRedisManager->getLastError().getLastErrorDescription());
        return REDIS_ACTIVATE_ABORT;
    }

    bool slotsCovered = checkSlotCoverage();
    if (slotsCovered)
    {
        mActive = true;
        if (!mUniqueIdManager.registerManager())
        {
            mActive = false;
            BLAZE_WARN_LOG(Log::REDIS, "RedisCluster[" << mName << "].activate: Failed to register UniqueIdManager; lastErrorDescription: "
                << gRedisManager->getLastError().getLastErrorDescription());
        }
        else
        {
            return REDIS_ACTIVATE_SUCCESSFUL;
        }
    }

    return REDIS_ACTIVATE_RETRY;
}

void RedisCluster::deactivate()
{
    if (!mActive)
        return;

    mActive = false;
    mUniqueIdManager.deregisterManager();
    mNodeNamesBySlotRangeMap.clear();
    mNodeNameByAddressMap.clear();

    mUpdateSlotsCoordinator.beginWorkOrWait(); // Make sure only ONE call from deactivate() and updateSlotsConfiguration() will be running, since both of them will delete node from mNodeByAddressMap

    if (mUpdateSlotsConfigurationTimerId != INVALID_TIMER_ID)
        gSelector->cancelTimer(mUpdateSlotsConfigurationTimerId);

    if (!mNodeByNodeNameMap.empty())
    {
        NodeByNodeNameMap copyNodeMap;
        mNodeByNodeNameMap.swap(copyNodeMap);
        for (NodeByNodeNameMap::iterator it = copyNodeMap.begin(), end = copyNodeMap.end(); it != end; ++it)
        {
            Node& node = it->second;

            node.commandConn->disconnect();
            delete node.commandConn;
        }
    }

    mUpdateSlotsCoordinator.doneWork(ERR_OK);
}

void RedisCluster::getStatus(RedisClusterStatus& status) const
{
    status.setAllNodes((uint32_t)mAllNodes.get());
    status.setCommands(mCmdCount.getTotal());
    status.setBytesSent(mBytesSent.getTotal());
    status.setBytesRecv(mBytesRecv.getTotal());

    RedisClusterStatus::ErrorCountsMap& errorMap = status.getErrorCounts();
    mErrorCount.iterate([&errorMap](const Metrics::TagPairList& tags, const Metrics::Counter& value) {
        const char8_t* errorName = tags[0].second.c_str();
        errorMap[errorName] = value.getTotal();
    });
}

const RedisCluster::Node *RedisCluster::selectNode(const RedisCommand &command) const
{
    uint16_t hashSlot = HASH_SLOT_MAX+1;
    if (command.getKeyCount() > 0)
    {
        hashSlot = command.getHashSlot();
    }

    return selectNodeInternal(hashSlot);
}

const RedisCluster::Node *RedisCluster::selectNodeInternal(uint16_t hashSlot) const
{
    const Node* node = nullptr;
    if (hashSlot <= HASH_SLOT_MAX)
        node = selectMasterNode(hashSlot);

    if (node != nullptr)
        return node;

    // Even if we're targeting a particular hash slot, we pick a random node if we can't
    // find the appropriate master. The node we pick will return a -MOVED error that'll direct
    // us to the correct node.
    return selectRandomNode();
}

const RedisCluster::Node *RedisCluster::selectMasterNode(uint16_t hashSlot) const
{
    SlotRange slotRange(hashSlot, hashSlot);
    NodeNamesBySlotRangeMap::const_iterator it = mNodeNamesBySlotRangeMap.lower_bound(slotRange);

    if (it != mNodeNamesBySlotRangeMap.end())
    {
        if (it->first.isInRange(hashSlot))
        {
            for (const auto& nodeName : it->second)
            {
                NodeByNodeNameMap::const_iterator nodeItr = mNodeByNodeNameMap.find(nodeName.c_str());
                if (nodeItr != mNodeByNodeNameMap.cend() && nodeItr->second.isMaster())
                    return &(nodeItr->second);
            }

            BLAZE_WARN_LOG(Log::REDIS, "RedisCluster[" << mName << "].selectMasterNode: No masters found for hashslot(" << hashSlot << ") from slot range("
                << it->first.lowerBound << ", " << it->first.upperBound << ").");
        }
        else
        {
            BLAZE_WARN_LOG(Log::REDIS, "RedisCluster[" << mName << "].selectMasterNode: Failed to find hashslot(" << hashSlot << ") from slot range("
                << it->first.lowerBound << ", " << it->first.upperBound << ").");
        }
    }
    return nullptr;
}

bool RedisCluster::SlotRange::isInRange(uint16_t hashSlot) const
{
    if (lowerBound <= hashSlot && hashSlot <= upperBound)
        return true;
    return false;
}

// static
uint16_t RedisCluster::getHashSlot(const RedisCommand &command)
{
    return (command.getKeyCount() > 0 ? command.getHashSlot() : HASH_SLOT_MAX + 1);
}

const RedisCluster::Node *RedisCluster::selectNode(const char8_t *key, int32_t keyLen) const
{
    uint16_t hashSlot = HASH_SLOT_MAX+1;
    if (key != nullptr)
    {
        hashSlot = computeHashSlot(key, keyLen);
    }

    return selectNodeInternal(hashSlot);
}

const RedisCluster::Node *RedisCluster::selectRandomNode(bool preferMaster /*= true*/) const
{
    if (mNodeByNodeNameMap.empty())
        return nullptr;

    const Node* node = nullptr;
    int startIndex = gRandom->getRandomNumber(mNodeByNodeNameMap.size());
    int randomIndex = startIndex;

    do 
    {
        node = &mNodeByNodeNameMap.at(randomIndex).second;
        if (node->isConnected() && (!preferMaster || node->isMaster()))
            return node;
        randomIndex = (randomIndex + 1) % mNodeByNodeNameMap.size();
    } while (randomIndex != startIndex);

    return node;
}

RedisError RedisCluster::resolveAddress(InetAddress & address)
{
    NameResolver::LookupJobId mResolverJobId;
    // Try to resolve the endpoint first
    BlazeRpcError sleepErr = gNameResolver->resolve(address, mResolverJobId);
    if (sleepErr != ERR_OK)
    {
        return REDIS_ERROR(REDIS_ERR_DNS_LOOKUP_FAILED, "RedisCluster[" << mName << "].resolveAddress: Failed to resolve address(" << address << "), failed with err(" << sleepErr << ").");
    }
    return REDIS_ERR_OK;
}

RedisError RedisCluster::upsertNode(InetAddress &address, const char8_t* nodeName)
{
    RedisError err = REDIS_ERR_OK;

    if (!address.isResolved())
    {
        err = resolveAddress(address);
        if (err != REDIS_ERR_OK)
            return err;
    }

    if (nodeName != nullptr && nodeName[0] != '\0')
    {
        NodeByNodeNameMap::iterator nodeItr = mNodeByNodeNameMap.find(nodeName);
        if (nodeItr != mNodeByNodeNameMap.end())
        {
            const InetAddress& currentAddr = nodeItr->second.commandConn->getHostAddress();
            if (currentAddr != address)
            {
                char8_t curBuf[InetAddress::MAX_HOSTNAME_LEN];
                char8_t newBuf[InetAddress::MAX_HOSTNAME_LEN];
                BLAZE_ERR_LOG(Log::REDIS, "RedisCluster[" << mName << "].upsertNode: Attempt to re-upsert redis node '" << nodeName << "' at a different address (current address: " <<
                    currentAddr.toString(curBuf, InetAddress::MAX_HOSTNAME_LEN, InetAddress::AddressFormat::HOST_PORT) << "; new address: " <<
                    address.toString(newBuf, InetAddress::MAX_HOSTNAME_LEN, InetAddress::AddressFormat::HOST_PORT) << ")");
                return REDIS_ERR_SYSTEM;
            }
            else
            {
                BLAZE_WARN_LOG(Log::REDIS, "RedisCluster[" << mName << "].upsertNode: Attempt to re-upsert redis node '" << nodeName << "' at " << address);
                return err;
            }
        }
    }
    else if (mNodeNameByAddressMap.find(address) != mNodeNameByAddressMap.end())
    {
        // We've already upserted this node
        return err;
    }

    RedisConn* commandConn = BLAZE_NEW RedisConn(address, nodeName, mClusterConfig.getPassword(), *this);
    err = commandConn->connect();
    if (err == REDIS_ERR_OK)
    {
        Node& newNode = mNodeByNodeNameMap[commandConn->getNodeName()];
        newNode.commandConn = commandConn;

        mNodeNameByAddressMap[address] = commandConn->getNodeName();

        BLAZE_INFO_LOG(Log::REDIS, "RedisCluster[" << mName << "].upsertNode: Upserted redis " << (newNode.isMaster() ? "master" : "replica") << " node '" << commandConn->getNodeName() << "' at " << address);
        return REDIS_ERR_OK;
    }
    else
    {
        delete commandConn;
        BLAZE_WARN_LOG(Log::REDIS, "RedisCluster[" << mName << "].upsertNode: Failed to upsert redis node '" << nodeName << "' at " << address << "; error:" << gRedisManager->getLastError()
            << "; lastErrorDescription: " << gRedisManager->getLastError().getLastErrorDescription());
    }

    return err;
}

RedisError RedisCluster::exec(const RedisCommand &command, RedisResponsePtr &response)
{
    return exec(command, response, true);
}

RedisError RedisCluster::exec(const RedisCommand &command, RedisResponsePtr &response, bool requireActiveCluster)
{
    RedisError err = REDIS_ERR_OK;
    InetAddress redirectAddr;
    uint16_t maxAttempts = gRedisManager->getRedisConfig().getMaxCommandRetries() + 1;
    const Node *node = nullptr;

    do
    {
        err = selectNextNode(node, command, err, redirectAddr, maxAttempts);
        if (err == REDIS_ERR_OK)
        {
            err = execOnNode(command, response, node, (command.getKeyCount() > 0 ? &redirectAddr : nullptr), requireActiveCluster);
        }
        else
        {
            eastl::string details;
            details += "; Command ->";
            command.encode(details);
            BLAZE_ASSERT_LOG(Log::REDIS, "RedisCluster[" << mName << "].exec: Failed to obtain node. Current error: " << RedisErrorHelper::getName(err) << "; Last error description: "
                << gRedisManager->getLastError().getLastErrorDescription() << details);
        }
    } while (shouldRedirectCommand(err) || shouldRetryCommand(err));

    return err;
}

RedisError RedisCluster::execScript(const RedisScript &redisScript, const RedisCommand &keysAndArgs, RedisResponsePtr &response)
{
    if (!mActive)
        return REDIS_ERROR(REDIS_ERR_SYSTEM, "RedisCluster[" << mName << "].execScript: Called without first being activated.");

    RedisError err = REDIS_ERR_OK;
    InetAddress redirectAddr;
    uint16_t maxAttempts = gRedisManager->getRedisConfig().getMaxCommandRetries() + 1;
    const Node *node = nullptr;

    do
    {
        err = selectNextNode(node, keysAndArgs, err, redirectAddr, maxAttempts);
        if (err == REDIS_ERR_OK)
        {
            RedisCommand scriptCommand;
            err = node->commandConn->makeScriptCommand(redisScript, keysAndArgs, scriptCommand);
            if (err == REDIS_ERR_OK)
                err = execOnNode(scriptCommand, response, node, (redisScript.getNumberOfKeys() > 0 ? &redirectAddr : nullptr));
            else
                mErrorCount.increment(1, err);
        }
        else
        {
            eastl::string details;
            details.sprintf("; Script -> %s, Keys/Arguments -> %" PRIi32" ", redisScript.getScript(), redisScript.getNumberOfKeys());
            keysAndArgs.encode(details);
            BLAZE_ASSERT_LOG(Log::REDIS, "RedisCluster[" << mName << "].execScript: Failed to obtain node. Current error: " << RedisErrorHelper::getName(err) << "; Last error description: "
                << gRedisManager->getLastError().getLastErrorDescription() << details);
        }
    } while (shouldRedirectCommand(err) || shouldRetryCommand(err));

    return err;
}

RedisError RedisCluster::execAll(const RedisCommand &command, RedisResponsePtrList &response)
{
    RedisError retErr = REDIS_ERR_OK;

    if (command.getKeyCount() > 0)
    {
        eastl::string details;
        details += "Command -> ";
        command.encode(details);
        BLAZE_ERR_LOG(Log::REDIS, "RedisCluster[" << mName << "].execAll: execAll() is only supported on slot-agnostic commands. " << details.c_str());
        return REDIS_ERR_SYSTEM;
    }

    // Here we get a copy of the nodes, and iterate over the list looking up the nodes by node name before running the command on them.
    NodeByNodeNameMap copyOfNodes = mNodeByNodeNameMap;
    NodeByNodeNameMap::iterator curNodeName = copyOfNodes.begin();
    for (; curNodeName != copyOfNodes.end(); ++curNodeName)
    {
        NodeByNodeNameMap::iterator curNode = mNodeByNodeNameMap.find(curNodeName->first.c_str());
        if (curNode != mNodeByNodeNameMap.end() && curNode->second.isMaster())
        {
            RedisError err = execOnNode(command, response.push_back(), &curNode->second);
            if (err != REDIS_ERR_OK)
                retErr = err;
        }
    }

    return retErr;
}


RedisError RedisCluster::getDumpOutputLocations(RedisDumpFileLocationsMap &response)
{
    RedisCommand getDirCmd, getFilenameCmd;
    getDirCmd.add("CONFIG");
    getDirCmd.add("GET");
    getDirCmd.add("dir");

    getFilenameCmd.add("CONFIG");
    getFilenameCmd.add("GET");
    getFilenameCmd.add("dbfilename");
    
    RedisError retErr = REDIS_ERR_OK;
        
    // Here we get a copy of the nodes, and iterate over the list looking up the nodes by node name before running commands on them.
    NodeByNodeNameMap copyOfNodes = mNodeByNodeNameMap;
    NodeByNodeNameMap::iterator curNodeName = copyOfNodes.begin();
    for (; curNodeName != copyOfNodes.end(); ++curNodeName)
    {
        NodeByNodeNameMap::iterator curNode = mNodeByNodeNameMap.find(curNodeName->first.c_str());
        if (curNode != mNodeByNodeNameMap.end() && curNode->second.isMaster())
        {
            RedisResponsePtr redisDirResponse; 
            RedisResponsePtr redisFilenameResponse; 
            retErr = execOnNode(getDirCmd, redisDirResponse, &curNode->second);
            if (redisDirResponse->getArraySize() != 2)
            {
                retErr = REDIS_ERR_SYSTEM;
            }
            if (retErr != REDIS_ERR_OK)
            {
                return REDIS_ERROR(retErr, "RedisCluster[" << mName << "].getDumpOutputLocations: Unable to lookup CONFIG dir. Response dir array size " << redisDirResponse->getArraySize());
            }
    
            // Set the file directory: 
            retErr = execOnNode(getFilenameCmd, redisFilenameResponse, &curNode->second);
            if (redisFilenameResponse->getArraySize() != 2)
            {
                retErr = REDIS_ERR_SYSTEM;
            }
            if (retErr != REDIS_ERR_OK)
            {
                return REDIS_ERROR(retErr, "RedisCluster[" << mName << "].getDumpOutputLocations: Unable to lookup CONFIG dir. Response filename array size " << redisFilenameResponse->getArraySize());
            }
    
            // Append the filename:
            eastl::string fileLocation = redisDirResponse->getArrayElement(1).getString();
            fileLocation += EA_FILE_PATH_SEPARATOR_STRING_8;
            fileLocation += redisFilenameResponse->getArrayElement(1).getString();
            response[curNode->second.commandConn->getHostAddress().getHostname()] = fileLocation.c_str();
        }
    }

    return retErr;
}

// static
Logging::Level RedisCluster::getErrorLevel(RedisError err)
{
    switch (err.error)
    {
    case REDIS_ERR_OK:
    case REDIS_ERR_NOT_FOUND:
        return Logging::Level::TRACE;
    case REDIS_ERR_ASK:
    case REDIS_ERR_MOVED:
    case REDIS_ERR_TRYAGAIN:
    case REDIS_ERR_LOADING:
        return Logging::Level::INFO;
    case REDIS_ERR_NOT_CONNECTED:
    case REDIS_ERR_SEND_COMMAND_FAILED:
    case REDIS_ERR_CLUSTERDOWN:
    case REDIS_ERR_HIREDIS_IO:
    case REDIS_ERR_HIREDIS_EOF:
        return Logging::Level::ERR;
    default:
        return Logging::Level::FAIL;
    }
}

// static
bool RedisCluster::isRecoverableNodeFailure(RedisError commandError)
{
    return (commandError == REDIS_ERR_NOT_CONNECTED || commandError == REDIS_ERR_SEND_COMMAND_FAILED || commandError == REDIS_ERR_CLUSTERDOWN || commandError == REDIS_ERR_HIREDIS_IO || commandError == REDIS_ERR_HIREDIS_EOF);
}

// static
bool RedisCluster::shouldRedirectCommand(RedisError commandError)
{
    return (commandError == REDIS_ERR_ASK || commandError == REDIS_ERR_MOVED);
}

// static
bool RedisCluster::shouldRetryCommand(RedisError commandError)
{
    return (commandError == REDIS_ERR_TRYAGAIN || commandError == REDIS_ERR_LOADING || isRecoverableNodeFailure(commandError));
}

// static
bool RedisCluster::shouldUpdateSlotsConfiguration(RedisError commandError)
{
    return (commandError == REDIS_ERR_MOVED || isRecoverableNodeFailure(commandError));
}

RedisError RedisCluster::execOnNode(const RedisCommand &command, RedisResponsePtr &response, const Node *node, InetAddress* redirectAddr /*= nullptr*/, bool requireActiveCluster /*= true*/)
{
    if (requireActiveCluster && !mActive)
        return REDIS_ERROR(REDIS_ERR_SYSTEM, "RedisCluster[" << mName << "].execOnNode: Called without first being activated.");
    if (node == nullptr)
        return REDIS_ERROR(REDIS_ERR_SYSTEM, "RedisCluster[" << mName << "].execOnNode: Called on a nullptr node.");

    return node->commandConn->exec(command, response, redirectAddr);
}

RedisError RedisCluster::selectNextNode(const Node*& nextNode, const RedisCommand &command, RedisError lastError, const InetAddress& nodeAddr, uint16_t& attemptsLeft)
{
    nextNode = nullptr;

    if (attemptsLeft-- <= 0)
    {
        mErrorCount.increment(1, REDIS_ERR_NO_MORE_RETRIES);
        return REDIS_ERR_NO_MORE_RETRIES;
    }

    RedisError sleepErr = REDIS_ERR_OK;

    if (shouldRetryCommand(lastError))
        sleepErr = RedisErrorHelper::convertError(Fiber::sleep(gRedisManager->getRedisConfig().getRetryCommandDelay(), "RedisCluster::selectNextNode(TRYAGAIN)"));

    if (shouldRedirectCommand(lastError))
    {
        NodeNameByAddressMap::const_iterator nameItr = mNodeNameByAddressMap.find(nodeAddr);
        if (nameItr == mNodeNameByAddressMap.end())
        {
            // If we don't have a node for this address, then either:
            // (1) -MOVED/-ASK has redirected us to a node at an address we don't know about yet
            //     (because it's a new node created after the redis cluster was scaled up).
            // or
            // (2) -MOVED/-ASK has redirected us to a node that's no longer valid
            //     (because the node was shut down, and the node that redirected us doesn't have the latest cluster configuration).
            //
            // In either case, we give the redis framework some time to catch up, then select a fresh node.
            sleepErr = RedisErrorHelper::convertError(Fiber::sleep(gRedisManager->getRedisConfig().getRetryCommandDelay(), "RedisCluster::selectNextNode(ASK/MOVED)"));
        }
        else
        {
            NodeByNodeNameMap::iterator nodeItr = mNodeByNodeNameMap.find(nameItr->second.c_str());
            if (nodeItr != mNodeByNodeNameMap.end())
            {
                RedisError redirectErr = REDIS_ERR_OK;
                nextNode = &nodeItr->second;

                if (lastError == REDIS_ERR_ASK)
                {
                    RedisResponsePtr dummyResponse;
                    RedisCommand askCommand;
                    askCommand.add("ASKING");
                    redirectErr = execOnNode(askCommand, dummyResponse, nextNode);
                }
                return redirectErr;
            }
            else
            {
                mErrorCount.increment(1, REDIS_ERR_SYSTEM);
                BLAZE_ERR_LOG(Log::REDIS, "RedisCluster[" << mName << "].selectNextNode: Failed to find node '" << nameItr->second.c_str() << "' associated with address " << nodeAddr);
                return REDIS_ERR_SYSTEM;
            }
        }
    }

    if (sleepErr == REDIS_ERR_OK)
    {
        nextNode = selectNode(command);
        if (nextNode == nullptr)
        {
            mErrorCount.increment(1, REDIS_ERR_SYSTEM);
            BLAZE_ERR_LOG(Log::REDIS, "RedisCluster[" << mName << "].selectNextNode: No nodes available");
            return REDIS_ERR_SYSTEM;
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::REDIS, "RedisCluster[" << mName << "].selectNextNode: Error " << RedisErrorHelper::getName(sleepErr) << " waiting for retryCommandDelay after " << RedisErrorHelper::getName(lastError));
        mErrorCount.increment(1, sleepErr);
    }

    return sleepErr;
}

RedisError RedisCluster::execScriptByMasterName(const char8_t* masterName, const RedisScript &redisScript, const RedisCommand &keysAndArgs, RedisResponsePtr &response)
{
    RedisError err = REDIS_ERR_SYSTEM;

    if (!mActive)
        return REDIS_ERROR(err, "RedisCluster[" << mName << "].execScriptByMasterName: Called without first being activated.");

    NodeByNodeNameMap::const_iterator nodeItr = mNodeByNodeNameMap.find(masterName);
    if (nodeItr == mNodeByNodeNameMap.cend())
        return err;

    RedisCommand scriptCommand;
    err = nodeItr->second.commandConn->makeScriptCommand(redisScript, keysAndArgs, scriptCommand);
    if (err == REDIS_ERR_OK)
        err = execOnNode(scriptCommand, response, &nodeItr->second);
    else
        mErrorCount.increment(1, err);

    return err;
}

uint16_t RedisCluster::computeHashSlot(const char8_t *key, int32_t keylen)
{
    int s, e; /* start-end indexes of { and } */

    for (s = 0; s < keylen; s++)
        if (key[s] == '{') 
            break;

    /* No '{' ? Hash the whole key. This is the base case. */
    if (s == keylen) 
        return redis_crc16(key,keylen) & HASH_SLOT_MAX;

    /* '{' found? Check if we have the corresponding '}'. */
    for (e = s+1; e < keylen; e++)
        if (key[e] == '}') 
            break;

    /* No '}' or nothing betweeen {} ? Hash the whole key. */
    if (e == keylen || e == s+1) 
        return redis_crc16(key,keylen) & HASH_SLOT_MAX;

    /* If we are here there is both a { and a } on its right. Hash
     * what is in the middle between { and }. */
    return redis_crc16(key+s+1,e-s-1) & HASH_SLOT_MAX;
}

const char8_t* RedisCluster::getRedisMasterNameForKey(const char8_t *key, int32_t keylen) const
{
    if (keylen <= 0)
        return nullptr;

    uint16_t hashSlot = computeHashSlot(key, keylen);
    const Node* masterNode = selectMasterNode(hashSlot);

    if (masterNode != nullptr)
        return masterNode->commandConn->getNodeName();

    return nullptr;
}

void RedisCluster::updateMetricValues(const RedisCommand& command, const RedisResponsePtr response, RedisError error)
{
    mCmdCount.increment();
    mBytesSent.increment(command.getDataSize());
    if (response)
        mBytesRecv.increment(response->getResponseLength());
    if (error != REDIS_ERR_OK)
        mErrorCount.increment(1, error);
};

void RedisCluster::scheduleUpdateSlotsConfiguration()
{
    if (mActive && mUpdateSlotsConfigurationTimerId == INVALID_TIMER_ID && !mUpdateSlotsCoordinator.isWorking())
        mUpdateSlotsConfigurationTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay(), this, &RedisCluster::doUpdateSlotsConfiguration, "RedisCluster::doUpdateSlotsConfiguration");
}

void RedisCluster::doUpdateSlotsConfiguration()
{
    mUpdateSlotsConfigurationTimerId = INVALID_TIMER_ID;
    RedisError err = updateSlotsConfiguration(false /*isActivating*/);
    if (err != REDIS_ERR_OK)
    {
        BLAZE_ERR_LOG(Log::REDIS, "RedisCluster[" << mName << "].doUpdateSlotsConfiguration: Failed to update slots configuration; error: " << RedisErrorHelper::getName(err) <<
            "; lastErrorDescription: " << gRedisManager->getLastError().getLastErrorDescription());
        if (mActive)
            mUpdateSlotsConfigurationTimerId = gSelector->scheduleFiberTimerCall(TimeValue::getTimeOfDay() + gRedisManager->getRedisConfig().getUpdateSlotsDelay(), this, &RedisCluster::doUpdateSlotsConfiguration, "RedisCluster::doUpdateSlotsConfiguration");
    }
}

RedisError RedisCluster::updateSlotsConfiguration(bool isActivating)
{
    RedisError err = REDIS_ERR_OK;

    if (mUpdateSlotsCoordinator.beginWorkOrWait())
    {
        if (!mActive && !isActivating)
        {
            BLAZE_INFO_LOG(Log::REDIS, "RedisCluster[" << mName << "].updateSlotsConfiguration: Called on deactivated cluster. Slots configuration update will be skipped.");
            mUpdateSlotsCoordinator.doneWork(ERR_OK);
            return REDIS_ERR_OK;
        }

        typedef eastl::hash_set<eastl::string> NodeNameSet;
        typedef eastl::hash_map<eastl::string, eastl::pair<InetAddress,bool>> AddressAndReadOnlyByNodeNameMap;
        AddressAndReadOnlyByNodeNameMap newNodes;
        NodeNameSet deletedNodes;
        NodeNamesBySlotRangeMap nodesBySlotRange;

        const Node* queryNode = selectRandomNode(false /*preferMaster*/);
        RedisCommand clusterSlotsCmd;
        clusterSlotsCmd.add("CLUSTER").add("SLOTS");
        RedisResponsePtr clusterSlotsResp;
        err = execOnNode(clusterSlotsCmd, clusterSlotsResp, queryNode, nullptr /*redirAddr*/, false /*requireActiveCluster*/);
        if (err != REDIS_ERR_OK)
        {
            mUpdateSlotsCoordinator.doneWork(ERR_OK);
            return err;
        }

        if (!clusterSlotsResp->isArray())
        {
            mUpdateSlotsCoordinator.doneWork(ERR_OK);
            return REDIS_ERROR(REDIS_ERR_SYSTEM, "RedisCluster[" << mName << "].updateSlotsConfiguration: CLUSTER SLOTS command returned non-array response");
        }

        for (uint32_t slotRangeIndex = 0; slotRangeIndex < clusterSlotsResp->getArraySize(); ++slotRangeIndex)
        {
            const RedisResponse &slotRangeInfo = clusterSlotsResp->getArrayElement(slotRangeIndex);
            SlotRange slotRange;
            slotRange.lowerBound = (uint16_t)slotRangeInfo.getArrayElement(START_SLOT_RANGE).getInteger();
            slotRange.upperBound = (uint16_t)slotRangeInfo.getArrayElement(END_SLOT_RANGE).getInteger();

            eastl::vector<eastl::string>& nodeList = nodesBySlotRange[slotRange];

            for (uint32_t nodeInfoIndex = MASTER_NODE_INFO; nodeInfoIndex < slotRangeInfo.getArraySize(); ++nodeInfoIndex)
            {
                const RedisResponse &nodeInfo = slotRangeInfo.getArrayElement(nodeInfoIndex);
                const char8_t* nodeName = nodeInfo.getArrayElement(NODE_NAME).getString();
                newNodes[nodeName] = eastl::pair<InetAddress,bool>(InetAddress(nodeInfo.getArrayElement(NODE_IP).getString(), (uint16_t)nodeInfo.getArrayElement(NODE_PORT).getInteger()), nodeInfoIndex > MASTER_NODE_INFO);
                nodeList.push_back(nodeName);
            }
        }

        for (NodeByNodeNameMap::iterator it = mNodeByNodeNameMap.begin(); it != mNodeByNodeNameMap.end(); ++it)
        {
            AddressAndReadOnlyByNodeNameMap::iterator newIt = newNodes.find(it->first.c_str());
            if (newIt == newNodes.end())
            {
                deletedNodes.insert(it->first.c_str());
            }
            else
            {
                if (it->second.updateReadOnly(newIt->second.second))
                {
                    BLAZE_INFO_LOG(Log::REDIS, "RedisCluster[" << mName << "].updateSlotsConfiguration: Node '" << it->first.c_str() << "' is now a " << (newIt->second.second ? "replica" : "master"));
                }
                newNodes.erase(newIt);
            }
        }

        for (NodeNameSet::iterator delIt = deletedNodes.begin(); delIt != deletedNodes.end(); ++delIt)
        {
            NodeByNodeNameMap::iterator it = mNodeByNodeNameMap.find(delIt->c_str());
            if (it != mNodeByNodeNameMap.end())
            {
                RedisConn* commandConn = it->second.commandConn;
                mNodeByNodeNameMap.erase(it);

                BLAZE_INFO_LOG(Log::REDIS, "RedisCluster[" << mName << "].updateSlotsConfiguration: Disconnecting deleted node '" << delIt->c_str() << "' at " << commandConn->getHostAddress());
                commandConn->disconnect();
                delete commandConn;
            }
        }

        for (NodeNameByAddressMap::iterator addrIt = mNodeNameByAddressMap.begin(); addrIt != mNodeNameByAddressMap.end(); )
        {
            if (deletedNodes.find(addrIt->second.c_str()) != deletedNodes.end())
                addrIt = mNodeNameByAddressMap.erase(addrIt);
            else
                ++addrIt;
        }

        for (AddressAndReadOnlyByNodeNameMap::iterator newIt = newNodes.begin(); newIt != newNodes.end(); ++newIt)
        {
            err = upsertNode(newIt->second.first, newIt->first.c_str());
            if (err != REDIS_ERR_OK)
            {
                mUpdateSlotsCoordinator.doneWork(ERR_OK);
                return err;
            }
        }

        mNodeNamesBySlotRangeMap.swap(nodesBySlotRange);
        mUpdateSlotsCoordinator.doneWork(ERR_OK);
        BLAZE_INFO_LOG(Log::REDIS, "RedisCluster[" << mName << "].updateSlotsConfiguration: Successfully updated slots configuration");
    }

    return err;
}

bool RedisCluster::checkSlotCoverage() const
{
    typedef eastl::set<SlotRange> SlotRangeSet;
    SlotRangeSet slotRangeSet;
    for (const auto& itr : mNodeNamesBySlotRangeMap)
        slotRangeSet.insert(itr.first);

    uint16_t nextLowerBound = 0;
    for (const auto& range : slotRangeSet)
    {
        if (range.lowerBound > nextLowerBound)
        {
            BLAZE_WARN_LOG(Log::REDIS, "RedisCluster[" << mName << "].checkSlotCoverage: Incomplete slot coverage -- missing slots in range " << nextLowerBound << "-" << range.lowerBound-1);
            return false;
        }
        nextLowerBound = range.upperBound + 1;
    }

    SlotRangeSet::const_reverse_iterator last = slotRangeSet.crbegin();
    if (last == slotRangeSet.crend())
    {
        BLAZE_WARN_LOG(Log::REDIS, "RedisCluster[" << mName << "].checkSlotCoverage: Incomplete slot coverage -- missing slots in range 0-" << HASH_SLOT_MAX);
        return false;
    }
    if (last->upperBound < HASH_SLOT_MAX)
    {
        BLAZE_WARN_LOG(Log::REDIS, "RedisCluster[" << mName << "].checkSlotCoverage: Incomplete slot coverage -- missing slots in range " << last->upperBound << "-" << HASH_SLOT_MAX);
        return false;
    }
    return true;
}

// static
bool RedisCluster::getAddressFromRedirectionError(const char8_t* errorString, size_t errorStringLen, InetAddress& address)
{
    // Redirection error strings will have format:
    // "<-MOVED|-ASK> <hashslot> <ip:port>"
    const char8_t* ipAndPort = nullptr;
    size_t index = 0;
    for (size_t i = 0; i < errorStringLen; ++i)
    {
        if (errorString[i] == ' ')
            ++index;

        if (index == REDIRECTION_ERROR_ADDRESS_INDEX)
        {
            if (i + 1 < errorStringLen)
                ipAndPort = &(errorString[i + 1]);
            break;
        }
    }

    if (ipAndPort == nullptr)
        return false;

    address.setHostnameAndPort(ipAndPort);
    return true;
}

} // Blaze
