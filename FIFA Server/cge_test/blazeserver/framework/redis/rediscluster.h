/*************************************************************************************************/
/*!
    \file

    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REDIS_CLUSTER_H
#define BLAZE_REDIS_CLUSTER_H

/*** Include files *******************************************************************************/

#include "blazerpcerrors.h"
#include "framework/system/fiber.h"
#include "framework/system/frameworkresource.h"
#include "framework/metrics/metrics.h"
#include "framework/redis/rediserrors.h"
#include "framework/redis/redisconn.h"
#include "framework/redis/redislockmanager.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/list.h"
#include "hiredis/hiredis.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class RedisCollectionManager;
class RedisUniqueIdManager;
class RedisConn;
class RedisCommand;
class RedisResponse;
class RedisClusterStatus;
typedef eastl::intrusive_ptr<RedisResponse> RedisResponsePtr;

class RedisCluster
{
    NON_COPYABLE(RedisCluster);

public:
    typedef Functor2<TimerId, RedisResponsePtr> TimerCb;

    RedisCluster(const char8_t* name);
    virtual ~RedisCluster();

    /*! ************************************************************************************************/
    /*!
        \brief Configures this RedisCluster.  Copies redisConfig into mRedisConfig and calls activate().

        \param[in] redisConfig - The blazeserver's RedisConfig
        \param[in] config - A RedisClusterConfig which provides a list of Redis servers, and their associated passwords. 
    ****************************************************************************************************/
    RedisError initFromConfig(const RedisConfig& redisConfig, const RedisClusterConfig& config);

    /*! ************************************************************************************************/
    /*!
        \brief Connects to each Redis server in the cluster.  If any of the Redis servers fail to
               connect, then an error is returned and all Redis servers are disconnected.
    
        \return REDIS_ERR_OK if all Redis servers connected successfully, otherwise an error.
    ****************************************************************************************************/
    RedisError activate(const RedisConfig& redisConfig);

    /*! ************************************************************************************************/
    /*!
        \brief Disconnects from all connected Redis servers.
    ****************************************************************************************************/
    void deactivate();

    void getStatus(RedisClusterStatus& status) const;
    const char8_t* getName() const { return mName.c_str(); }

    /*! ************************************************************************************************/
    /*!
        \brief Sends the command to the Redis server, and blocks until a response is received.

        \param[in] command - The command to send to the Redis server.
        \param[out] response - If no error occurs while sending the command or receiving the response,
                               the response contains a RedisResponse object populated with RESP data.
                               (See https://redis.io/topics/protocol for details about the possible RESP data types.)

        \return REDIS_ERR_OK is returned if a response was received and the response type is not a RESP error.

               Note that this method will never return a redirection error (i.e. REDIS_ERR_ASK/REDIS_ERR_MOVED) or
               an error that is recoverable (via failover or otherwise; e.g. REDIS_ERR_TRYAGAIN, REDIS_ERR_CLUSTERDOWN,
               REDIS_ERR_NOT_CONNECTED, REDIS_ERR_SEND_COMMAND_FAILED). These errors are handled automatically.

               REDIS_ERR_NO_MORE_RETRIES is returned if the command can't be
               redirected/retried on the appropriate node after a configurable number of attempts.

    ****************************************************************************************************/
    RedisError exec(const RedisCommand &command, RedisResponsePtr &response);

    /*! ************************************************************************************************/
    /*!
        \brief Executes the specified Lua script, and waits for a response from the server.

        \param[in] redisScript - A unique script that was registered during static initialization at program startup.
        \param[in] keysAndArgs - A RedisClusterCommand which contains the KEYS and parameters to pass to the Lua script.
                                 See http://redis.io/commands/eval for details.
        \param[in] response - If no error occurs while executing the script or receiving the response,
                              the response contains a RedisResponse object populated with RESP data.
                              (See https://redis.io/topics/protocol for details about the possible RESP data types.)

        \return Same as RedisCluster::exec
    ****************************************************************************************************/
    RedisError execScript(const RedisScript &redisScript, const RedisCommand &keysAndArgs, RedisResponsePtr &response);

    /*! ************************************************************************************************/
    /*!
        \brief Sends the command to all Redis master nodes, and blocks until all responses are received.
               All The Redis master nodes in the cluster will execute this command.

        \param[in] command - The command to send to the Redis master nodes.
                             This command must be slot-agnostic (i.e. it must not include any keys).
                             All Redis master nodes in the cluster will execute this command.

        \param[in] response - If no error occurs, the response contains a list of RedisResponsePtrs,
                              each containing a RedisResponse object populated with RESP data from a
                              Redis master node.
                              (See https://redis.io/topics/protocol for details about the possible RESP data types.)

        \return REDIS_ERR_OK if all responses were received and none of the responses contained a RESP error.

    ****************************************************************************************************/
    RedisError execAll(const RedisCommand &command, RedisResponsePtrList &response);

    RedisCollectionManager& getCollectionManager() { return mCollectionManager; }
    RedisUniqueIdManager& getUniqueIdManager() { return mUniqueIdManager; }

    static uint16_t computeHashSlot(const char8_t *key, int32_t keylen);

    // Maximum number of Redis Cluster hash slots, as defined by the Redis Cluster specification (http://redis.io/topics/cluster-spec)
    static const uint16_t HASH_SLOT_MAX = 16383;

    // Index of <ip:port> string returned by -MOVED and -ASK errors
    static const uint16_t REDIRECTION_ERROR_ADDRESS_INDEX = 2;

    RedisError getDumpOutputLocations(RedisDumpFileLocationsMap &response);
    static Logging::Level getErrorLevel(RedisError err);

private:
    friend class RedisLock;
    friend class RedisConn;
    friend class RedisResponse;
    friend class RedisErrorWrapper;
    friend class RedisCollection;

    struct SlotRange
    {
        SlotRange() : lowerBound(0), upperBound(0) {}
        SlotRange(uint16_t low, uint16_t upper) : lowerBound(low), upperBound(upper) {}
        uint16_t lowerBound;
        uint16_t upperBound;
        bool operator<(const SlotRange &rhs) const
        {
            return (upperBound < rhs.lowerBound);
        }
        bool isInRange(uint16_t hashSlot) const;
    };
/*
    struct SlotRangeCompare
    {
        bool operator() (const SlotRange& lhs, const SlotRange& rhs) const
        {
            return lhs < rhs;
        }
    };
    */
    struct Node
    {
        Node() : commandConn(nullptr) {}

        bool isMaster() const { return !commandConn->getReadOnly(); }
        bool isConnected() const { return commandConn != nullptr && commandConn->isConnected(); }
        bool updateReadOnly(bool readOnly) {
            if (commandConn != nullptr && commandConn->getReadOnly() != readOnly)
            {
                commandConn->setReadOnly(readOnly);
                return true;
            }
            return false;
        }

        RedisConn *commandConn;
    };

    typedef eastl::vector_map<SlotRange, eastl::vector<eastl::string>> NodeNamesBySlotRangeMap;
    typedef eastl::vector_map<eastl::string, Node> NodeByNodeNameMap;
    typedef eastl::vector_map<InetAddress, eastl::string> NodeNameByAddressMap;
    typedef eastl::vector<Node> NodeList;

    static uint16_t getHashSlot(const RedisCommand &command);

    enum RedisActivateStatus
    {
        REDIS_ACTIVATE_SUCCESSFUL,
        REDIS_ACTIVATE_ABORT,
        REDIS_ACTIVATE_RETRY,
    };
    RedisActivateStatus activateInternal(RedisError& err);

    const Node *selectNode(const RedisCommand &command) const;
    const Node *selectNode(const char8_t *key, int32_t keyLen) const;
    const Node *selectNodeInternal(uint16_t hashSlot) const;
    const Node *selectRandomNode(bool preferMaster = true) const;
    const Node *selectMasterNode(uint16_t hashSlot) const;

    RedisError selectNextNode(const Node*& nextNode, const RedisCommand &command, RedisError lastError, const InetAddress& nodeAddr, uint16_t& attemptsLeft);

    RedisError upsertNode(InetAddress &address, const char8_t* nodeName);

    RedisError resolveAddress(InetAddress & address);

    void updateMetricValues(const RedisCommand& command, const RedisResponsePtr response, RedisError error);
    RedisError exec(const RedisCommand &command, RedisResponsePtr &response, bool requireActiveCluster);
    RedisError execOnNode(const RedisCommand &command, RedisResponsePtr &response, const Node *node, InetAddress* redirectAddr = nullptr, bool requireActiveCluster = true);

    static bool shouldRetryCommand(RedisError commandError);
    static bool shouldRedirectCommand(RedisError commandError);
    static bool shouldUpdateSlotsConfiguration(RedisError commandError);
    static bool isRecoverableNodeFailure(RedisError commandError);

    static bool getAddressFromRedirectionError(const char8_t* errorString, size_t errorStringLen, InetAddress& address);
    void scheduleUpdateSlotsConfiguration();


    // DEPRECATED - These methods are only used by RediskeyFieldMap::getAllByKeys,
    // which is itself deprecated because it can't be executed reliably on redis in cluster mode
    const char8_t* getRedisMasterNameForKey(const char8_t *key, int32_t keylen) const;
    RedisError execScriptByMasterName(const char8_t* masterName, const RedisScript &redisScript, const RedisCommand &keysAndArgs, RedisResponsePtr &response);

private:

    typedef enum ClusterSlotsOuterArrayVal
    {
        START_SLOT_RANGE,
        END_SLOT_RANGE,
        MASTER_NODE_INFO,
        FIRST_REPLICA_NODE_INFO
    } ClusterSlotsOuterArrayVal;

    typedef enum ClusterSlotsInnerArrayVal
    {
        NODE_IP,
        NODE_PORT,
        NODE_NAME
    } ClusterSlotsInnerArrayVal;

    RedisError updateSlotsConfiguration(bool isActivating);
    void doUpdateSlotsConfiguration();
    bool checkSlotCoverage() const;
    bool mActive;
    eastl::string mName;

    RedisClusterConfig mClusterConfig;

    NodeNamesBySlotRangeMap mNodeNamesBySlotRangeMap;
    NodeByNodeNameMap mNodeByNodeNameMap;
    NodeNameByAddressMap mNodeNameByAddressMap;

    WorkCoordinator mUpdateSlotsCoordinator;
    TimerId mUpdateSlotsConfigurationTimerId;

    RedisCollectionManager& mCollectionManager;
    RedisUniqueIdManager& mUniqueIdManager;

    int32_t mLockIdCounter;

    Metrics::MetricsCollection& mMetricsCollection;
    Metrics::Counter mCmdCount;
    Metrics::TaggedCounter<RedisError> mErrorCount;
    Metrics::Counter mBytesSent;
    Metrics::Counter mBytesRecv;
    Metrics::PolledGauge mAllNodes;

#ifdef EA_PLATFORM_WINDOWS
    eastl::vector<HANDLE> mJobObjectHandles;
#endif
};


} // Blaze

#endif // BLAZE_REDIS_CLUSTER_H
