/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_INSTANCEINFOCONTROLLER_H
#define BLAZE_INSTANCEINFOCONTROLLER_H

/*** Include files *******************************************************************************/

#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/tdf/controllertypes_server.h"
#include "framework/redis/rediskeyfieldmap.h"
#include "framework/system/fibermutex.h"

#include "redirector/tdf/redirectortypes_server.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace envoy
{
    namespace api
    {
    namespace v2
    {
        namespace core
        {
            class SocketAddress;
        }
        namespace endpoint
        {
            class LbEndpoint;
        }

        class ClusterLoadAssignment;
    }
    }
}

namespace Blaze
{

class InetAddress;
class Selector;

struct RemoteInstanceAddress
{
    uint32_t mIp;   // ipv4 (in host order)
    uint16_t mPort; // socket port (in host order)
};
typedef eastl::vector<RemoteInstanceAddress> RemoteInstanceAddressList;

struct RemoteInstanceInfo
{
    RemoteInstanceAddressList mAddrList;
};

class InstanceInfoController
{
public:

    InstanceInfoController(Selector &selector);
    ~InstanceInfoController();

    bool initializeRedirectorConnection();
    void onConnectAndActivateInstanceFailure();
    InstanceId allocateInstanceIdFromRedis();
    void onInstanceIdReady();
    bool startRemoteInstanceSynchronization();

    void onControllerConfigured();
    void onControllerReconfigured();
    void onControllerServiceStateChange();

    void shutdownController();
    void onControllerDrain();

    const ServerInstanceInfo::ServerEndpointInfoList& getEndpointInfoList() { return mPublishServerInstanceInfoRequest->getInfo().getStaticData().getInstance().getEndpoints(); }

    /*! \brief Gets the id that identifies this instance with the redirector */
    uint64_t getRegistrationId() const { return mRegistrationId; }

private:
    int32_t getCurrentLoad() const;

    enum RedirectorUpdateStage
    {
        REDIRECTOR_UPDATE_STAGE_WAITING = 0,
        REDIRECTOR_UPDATE_STAGE_PUBLISH_FULL_INFO,
        REDIRECTOR_UPDATE_STAGE_UPDATE_DYNAMIC_INFO,
        REDIRECTOR_UPDATE_STAGE_MAX // SENTINEL, DO NOT USE
    };

    enum RedisInstanceInfoStage
    {
        REDIS_INSTANCE_INFO_STAGE_WAITING = 0,
        REDIS_INSTANCE_INFO_STAGE_PUBLISHING,
        REDIS_INSTANCE_INFO_STAGE_REFRESHING,
        REDIS_INSTANCE_INFO_STAGE_MAX // SENTINEL, DO NOT USE
    };

    static const char8_t* FIELD_REMOTE_INSTANCE_IP;
    static const char8_t* FIELD_REMOTE_INSTANCE_PORT;
    void updateRedirector();
    void scheduleUpdateRedirector(EA::TDF::TimeValue& nextUpdate);
    void updateRedirectorInternal(EA::TDF::TimeValue& nextUpdate);

    void syncRemoteInstanceConnections();
    void scheduleSyncRemoteInstanceConnections();

    void handleUpdateRedirectorResponse(const Redirector::UpdateServerInstanceInfoResponse& resp);
    void handleRedirectorPeerServerAddressInfo(const Redirector::PeerServerAddressInfo& peerServerAddressInfo);

    bool getRedisKeyExists(const char8_t* keyNamespace, const eastl::string& key);
    bool updateRedisKeyExpiry(const char8_t* keyNamespace, const eastl::string& key, EA::TDF::TimeValue relativeTimeout);

    bool isRedisEnvoyInfoMissing();
    bool isRedisRemoteInstanceInfoMissing();
    bool updateRedisEnvoyInfoExpiry(EA::TDF::TimeValue relativeTimeout);
    bool updateRedisRemoteInstanceInfoExpiry(EA::TDF::TimeValue relativeTimeout);

    void refreshRedisInstanceInfo();
    void scheduleRefreshRedisInstanceInfo();
    bool refreshRedisInstanceInfoInternal();

    bool publishInstanceInfoToRedis();

    typedef eastl::hash_map<eastl::string, ::envoy::api::v2::ClusterLoadAssignment*> EnvoyEndpointMap;
    void populateSocketAddress(::envoy::api::v2::core::SocketAddress* socketAddress, const Blaze::IEndpoint* blazeEndpoint);
    void populateLbEndpoint(::envoy::api::v2::endpoint::LbEndpoint* lbEndpoint, const Blaze::IEndpoint* blazeEndpoint);
    void populateEnvoyEndpoint(EnvoyEndpointMap& envoyEndpoints, const Blaze::IEndpoint* blazeEndpoint);
    void populateInstanceInfo(EnvoyEndpointMap& envoyEndpoints, RemoteInstanceAddress& internalFire2Address);

    void refreshCachedInstanceNamesLedger();
    BlazeRpcError fetchRemoteInstanceInfo(RemoteInstanceInfo& remoteInstanceInfo);
    BlazeRpcError getPeerAddressInfo(Redirector::PeerServerAddressInfo& peerServerAddressInfo);

    BlazeRpcError publishFullInstanceInfoToRedirector(Redirector::UpdateServerInstanceInfoResponse& resp);
    BlazeRpcError updateDynamicInstanceInfoOnRedirector(Redirector::UpdateServerInstanceInfoResponse& resp);    
    void populateDynamicInstanceInfo(Redirector::ServerInstanceDynamicData& dynamicData);

    void populateServerInstanceStaticData();

    void refreshRedisInstanceId();
    void scheduleRefreshRedisInstanceId();

private:
    // Number of milliseconds to wait for response of redirector calls
    static const uint32_t REDIRECTOR_RPC_TIMEOUT = 15000;

    // Number of milliseconds between synchronizing the remote instance connections
    static const uint32_t SYNC_REMOTE_INSTANCE_CONNECTIONS_MASTER_CONTROLLER_INTERVAL = 1000;
    static const uint32_t SYNC_REMOTE_INSTANCE_CONNECTIONS_BOOTSTRAP_INTERVAL = 1000;
    static const uint32_t SYNC_REMOTE_INSTANCE_CONNECTIONS_ACTIVE_INTERVAL = 5000;
    // Number of milliseconds between Redis instance info updates on startup
    static const uint32_t REDIS_INSTANCE_INFO_UPDATE_BOOTSTRAP_INTERVAL = 1000;
    // Number of milliseconds between redirector updates on startup
    static const uint32_t UPDATE_REDIRECTOR_BOOTSTRAP_INTERVAL = 1000;

    typedef eastl::vector<InetAddress> ResolvedRemoteServerInstances;

    const BootConfig& mBootConfigTdf;

    uint64_t mRegistrationId;    

    static const char8_t* ENVOY_API_VERSION;
    eastl::string mInstanceEnvoyResourcesRedisKey;
    eastl::string mInstanceEnvoyVersionInfoRedisKey;

    eastl::string mInstanceNamesLedgerVersionInfo;
    eastl::vector<eastl::string> mInstanceNamesLedger;

    typedef RedisKeyFieldMap<eastl::string, eastl::string, uint32_t> RemoteInstanceInfoFieldMap;
    RemoteInstanceInfoFieldMap mRemoteInstanceInfoFieldMap;

    uint16_t mRefreshRedisInstanceIdFailCount;
    uint16_t mRefreshRedisInstanceInfoFailCount;

    Selector& mSelector;

    InetAddress mRdirAddr;
    uint64_t mClusterVersion;
    bool mForceSyncClusterVersion;
    Redirector::PublishServerInstanceInfoRequestPtr mPublishServerInstanceInfoRequest;
    RedirectorUpdateStage mRedirectorUpdateStage;
    RedisInstanceInfoStage mRedisInstanceInfoStage;
    ResolvedRemoteServerInstances mResolvedRemoteServerInstances;

    FiberMutex mUpdateRedirectorFiberMutex;
    FiberMutex mRefreshRedisInstanceInfoFiberMutex;

    TimerId mRefreshRedisInstanceIdTimerId;
    TimerId mRefreshRedisInstanceInfoTimerId;
    TimerId mUpdateRedirectorTimerId;
    TimerId mSyncRemoteInstanceConnectionsTimerId;

    bool mIsEnvoyInstanceInfoInRedis;
};

} // Blaze

#endif // BLAZE_INSTANCEINFOCONTROLLER_H
