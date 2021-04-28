/*************************************************************************************************/
/*!
    \file 
  

    \attention
    (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#include "framework/blaze.h"

#include "framework/controller/instanceinfocontroller.h"

#include "EASTL/string.h"
#include "framework/controller/controller.h"
#include "framework/controller/processcontroller.h"
#include "framework/connection/connectionmanager.h"
#include "framework/connection/outboundconnectionmanager.h"
#include "framework/connection/nameresolver.h"
#include "framework/grpc/inboundgrpcmanager.h"
#include "framework/controller/instanceidallocator.h"
#include "redirector/rpc/redirectorslave.h"
#include "framework/redirector/redirectorutil.h"
#include "framework/redis/redismanager.h"

#ifdef EA_PLATFORM_LINUX
#include <sys/resource.h>
#endif

#ifdef EA_PLATFORM_WINDOWS
// WindowsSDK uses macros to #define a ton of symbols, one of which (DELETE in winnt.h)
// interferes with Envoy protobuf code. DELETE shows up in the base.pb.h header generated from
// envoy/api/v2/core/base.proto. Since it's a generated header, we can't #undef DELETE at
// the top of that header to avoid the collision.
#undef DELETE
#endif
#include "envoy/api/v2/endpoint.pb.h"

namespace Blaze
{

const char8_t* InstanceInfoController::FIELD_REMOTE_INSTANCE_IP = "ip";
const char8_t* InstanceInfoController::FIELD_REMOTE_INSTANCE_PORT = "port";
const char8_t* InstanceInfoController::ENVOY_API_VERSION = "v2";

/*! ***************************************************************************/
/*!    \brief Constructor
*******************************************************************************/
InstanceInfoController::InstanceInfoController(Selector& selector)
    : mBootConfigTdf(gProcessController->getBootConfigTdf()),
      mRegistrationId(0),
      mInstanceEnvoyResourcesRedisKey(),
      mInstanceEnvoyVersionInfoRedisKey(),
      mRemoteInstanceInfoFieldMap(Blaze::RedisId(RedisManager::FRAMEWORK_NAMESPACE, "remoteInstanceInfoFieldMap"), false),
      mRefreshRedisInstanceIdFailCount(0),
      mRefreshRedisInstanceInfoFailCount(0),
      mSelector(selector),
      mClusterVersion(0),
      mForceSyncClusterVersion(false),
      mPublishServerInstanceInfoRequest(BLAZE_NEW Redirector::PublishServerInstanceInfoRequest),
      mRedirectorUpdateStage(REDIRECTOR_UPDATE_STAGE_WAITING),
      mRedisInstanceInfoStage(REDIS_INSTANCE_INFO_STAGE_WAITING),
      mRefreshRedisInstanceIdTimerId(INVALID_TIMER_ID),
      mRefreshRedisInstanceInfoTimerId(INVALID_TIMER_ID),
      mUpdateRedirectorTimerId(INVALID_TIMER_ID),
      mSyncRemoteInstanceConnectionsTimerId(INVALID_TIMER_ID),
      mIsEnvoyInstanceInfoInRedis(false)
{
}

/*! ***************************************************************************/
/*!    \brief Destructor
*******************************************************************************/
InstanceInfoController::~InstanceInfoController()
{
}

bool InstanceInfoController::initializeRedirectorConnection()
{
    const char8_t* redirectorSlaveAddr = mBootConfigTdf.getRedirectorSlaveAddr();
    if (redirectorSlaveAddr[0] != '\0')
    {        
        mRdirAddr.setHostnameAndPort(redirectorSlaveAddr);

        if (!gNameResolver->blockingResolve(mRdirAddr))
        {
            char8_t addrStr[256];
            BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].initializeRedirectorConnection: could not resolve "
                          << "redirector slave server address: "<< mRdirAddr.toString(addrStr, sizeof(addrStr)));
            return false;
        }

        gController->getOutboundConnectionManager().setRedirectorAddr(mRdirAddr);
    }
    else if (mBootConfigTdf.getUseRedirectorForRemoteServerInstances())
    {
        // server can startup without redirector slave addr in the configs
        // if the remote instances of peers in its cluster is defined
        // in the configs
        if (mBootConfigTdf.getRemoteServerInstances().empty())
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].initializeRedirectorConnection: Both redirectorSlaveAddr and remoteServerInstances are not defined in boot config -- we must have at least one defined.");
            return false;
        }
        BLAZE_INFO_LOG(Log::CONTROLLER, "[InstanceInfoController].initializeRedirectorConnection: Starting server without redirectorSlaveAddr, using remoteServerInstances only.");
    }
    
    return true;
}

void InstanceInfoController::onConnectAndActivateInstanceFailure()
{
    //Always reset cluster version on failure to force resyncing peer conn list on next update
    mForceSyncClusterVersion = true;
}

InstanceId InstanceInfoController::allocateInstanceIdFromRedis()
{
    InstanceId newInstanceId = InstanceIdAllocator::getOrRefreshInstanceId(gController->getInstanceName(), gController->getInstanceId(), mBootConfigTdf.getInstanceIdAllocatorConfig().getRedisTTL());
    if (newInstanceId == INVALID_INSTANCE_ID)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].allocateInstanceIdFromRedis: Failed to allocate instance id from redis.");
    }
    return newInstanceId;
}

void InstanceInfoController::onInstanceIdReady()
{
    // Using instanceName instead of instanceId as key since instanceId can potentially change between startups
    const char8_t* instanceName = gController->getInstanceName();
    mInstanceEnvoyResourcesRedisKey.sprintf("eadp.blaze.instances.{%s}.envoy.eds.%s", instanceName, ENVOY_API_VERSION);
    mInstanceEnvoyVersionInfoRedisKey.sprintf("eadp.blaze.instances.{%s}.envoy.version_info", instanceName);

    scheduleRefreshRedisInstanceId();
}

bool InstanceInfoController::startRemoteInstanceSynchronization()
{
    populateServerInstanceStaticData();

    syncRemoteInstanceConnections();
    updateRedirector();

    // if we're not using redirector to discover our peers, all of the server instances in the cluster will be specified
    // in remoteServerInstances in the boot config.
    if (!mBootConfigTdf.getRemoteServerInstances().empty())
    {
        mResolvedRemoteServerInstances.reserve(mBootConfigTdf.getRemoteServerInstances().size());
        for (BootConfig::StringRemoteServerInstancesList::const_iterator
            it = mBootConfigTdf.getRemoteServerInstances().begin(),
            itEnd = mBootConfigTdf.getRemoteServerInstances().end(); it != itEnd; ++it)
        {
            InetAddress addr(it->c_str());
            char8_t addrBuf[256];
            Blaze::NameResolver::LookupJobId jobId;
            BlazeRpcError err = gNameResolver->resolve(addr, jobId);
            if (err != ERR_OK)
            {
                BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].startRemoteInstanceSynchronization: " <<
                    "Failed to resolve remote server instance at " << addr.toString(addrBuf, sizeof(addrBuf)) << ", rc: " << ErrorHelp::getErrorName(err));
                return false;
            }
            mResolvedRemoteServerInstances.push_back(addr);
        }

        gController->connectAndActivateRemoteInstances(mResolvedRemoteServerInstances);
    }

    return true;
}

void InstanceInfoController::onControllerConfigured()
{
    // now that we have the configuration values, we need to publish our full info to the redirector
    populateServerInstanceStaticData();
    mRedirectorUpdateStage = REDIRECTOR_UPDATE_STAGE_PUBLISH_FULL_INFO;
    mRedisInstanceInfoStage = REDIS_INSTANCE_INFO_STAGE_PUBLISHING;

    refreshRedisInstanceInfo();

    BLAZE_INFO_LOG(Log::CONTROLLER, "[InstanceInfoController].onControllerConfigured: instance is ready to publish full instance info");
}

void InstanceInfoController::onControllerReconfigured()
{
    // now that we have the updated configuration values, we need to publish our full info to the redirector
    populateServerInstanceStaticData();
    mRedirectorUpdateStage = REDIRECTOR_UPDATE_STAGE_PUBLISH_FULL_INFO;
    mRedisInstanceInfoStage = REDIS_INSTANCE_INFO_STAGE_PUBLISHING;

    BLAZE_INFO_LOG(Log::CONTROLLER, "[InstanceInfoController].onControllerReconfigured: framework config has been reloaded; publishing the instance info again.");
}

void InstanceInfoController::onControllerServiceStateChange()
{
    mRedisInstanceInfoStage = REDIS_INSTANCE_INFO_STAGE_PUBLISHING;
}

void InstanceInfoController::updateRedirectorInternal(EA::TDF::TimeValue& nextUpdate)
{
    FiberAutoMutex autoMutex(mUpdateRedirectorFiberMutex);
    if (autoMutex.hasLock())
    {
        if (gController->isShuttingDown())
            return;

        if (mRdirAddr.isValid())
        {
            if (mRedirectorUpdateStage == REDIRECTOR_UPDATE_STAGE_WAITING)
            {
                if (mBootConfigTdf.getUseRedirectorForRemoteServerInstances())
                {
                    BLAZE_TRACE_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedirectorInternal: getting peer remote instance addresses from the redirector until the instance is ready to publish full instance info");
                    Redirector::PeerServerAddressInfo peerServerAddressInfo;
                    BlazeRpcError rc = getPeerAddressInfo(peerServerAddressInfo);
                    if (rc != ERR_OK)
                    {
                        BLAZE_WARN_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedirectorInternal: getPeerAddressInfo failed with error: " << ErrorHelp::getErrorName(rc));
                    }
                    else
                    {
                        handleRedirectorPeerServerAddressInfo(peerServerAddressInfo);
                    }
                }
                else
                {
                    BLAZE_TRACE_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedirectorInternal: waiting until the instance is ready to publish full instance info");
                }
            }
            else
            {
                BlazeRpcError rc = ERR_SYSTEM;
                Redirector::UpdateServerInstanceInfoResponse resp;

                if (mRedirectorUpdateStage == REDIRECTOR_UPDATE_STAGE_PUBLISH_FULL_INFO)
                {
                    BLAZE_INFO_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedirectorInternal: publishing the full instance info to the redirector.");
                    rc = publishFullInstanceInfoToRedirector(resp);

                    if (rc == ERR_OK)
                    {
                        mRedirectorUpdateStage = REDIRECTOR_UPDATE_STAGE_UPDATE_DYNAMIC_INFO;
                        BLAZE_INFO_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedirectorInternal: full instance info has been published to the redirector; transitioning to the update-only stage.");
                    }
                    else
                    {
                        BLAZE_WARN_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedirectorInternal: publishFullInstanceInfoToRedirector failed with error: " << ErrorHelp::getErrorName(rc));
                    }
                }
                else if (mRedirectorUpdateStage == REDIRECTOR_UPDATE_STAGE_UPDATE_DYNAMIC_INFO)
                {
                    BLAZE_TRACE_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedirectorInternal: sending the dynamic instance info update to the redirector.");
                    rc = updateDynamicInstanceInfoOnRedirector(resp);

                    if (rc == ERR_OK)
                    {
                        BLAZE_TRACE_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedirectorInternal: dynamic instance info has been updated on the redirector.");
                    }
                    else if (rc == REDIRECTOR_NO_MATCHING_INSTANCE)
                    {
                        // redirector has somehow lost us, republish ourselves
                        mRedirectorUpdateStage = REDIRECTOR_UPDATE_STAGE_PUBLISH_FULL_INFO;
                        BLAZE_INFO_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedirectorInternal: the redirector has lost this instance's info; publishing the instance info again.");
                    }
                    else if (rc != ERR_OK)
                    {
                        BLAZE_WARN_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedirectorInternal: updateDynamicInstanceInfoOnRedirector failed with error: " << ErrorHelp::getErrorName(rc));
                    }
                }
                else
                {
                    BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedirectorInternal: Invalid mRedirectorUpdateStage(" <<
                        mRedirectorUpdateStage << ")");
                }

                if (rc == ERR_OK)
                {
                    handleUpdateRedirectorResponse(resp);

                    nextUpdate = resp.getNextUpdate();
                }
            }
        }
        // if mRdirAddr is not valid and we're using the redirector to discover our peers (instead of fetchRemoteInstanceInfo),
        // all of the server instances in the cluster should be specified
        // in remoteServerInstances in the boot config.
        else if (mBootConfigTdf.getUseRedirectorForRemoteServerInstances())
        {
            gController->connectAndActivateRemoteInstances(mResolvedRemoteServerInstances);
        }
    }
}

void InstanceInfoController::handleUpdateRedirectorResponse(const Redirector::UpdateServerInstanceInfoResponse& resp)
{
    // save off the registrationId from the response to use in the next update
    mRegistrationId = resp.getRegistrationId();

    const Redirector::PeerServerAddressInfo& peerServerAddressInfo = resp.getPeerServerAddressInfo();
    handleRedirectorPeerServerAddressInfo(peerServerAddressInfo);
}

void InstanceInfoController::handleRedirectorPeerServerAddressInfo(const Redirector::PeerServerAddressInfo& peerServerAddressInfo)
{
    // only update cluster version if getting a valid value from redirector. A zero cluster version or empty address list means no changes in cluster
    // version since our last update.
    if (peerServerAddressInfo.getVersion() != 0)
        mClusterVersion = peerServerAddressInfo.getVersion();

    // Pass the latest redirector update into the same function that handles remote instance info from Redis;
    // only use the redirector info to sanity check the Redis info (logging only).
    RemoteInstanceInfo remoteInstanceInfo;
    remoteInstanceInfo.mAddrList.reserve(peerServerAddressInfo.getAddressList().size());
    for (const Redirector::ServerAddressInfoPtr serverAddress : peerServerAddressInfo.getAddressList())
    {
        const Redirector::IpAddress* srcIpAddress = serverAddress->getAddress().getIpAddress();
        remoteInstanceInfo.mAddrList.push_back(RemoteInstanceAddress{srcIpAddress->getIp(), srcIpAddress->getPort()});
    }
    gController->handleRemoteInstanceInfo(remoteInstanceInfo, true);
}

void InstanceInfoController::updateRedirector()
{
    EA::TDF::TimeValue nextUpdate;
    updateRedirectorInternal(nextUpdate);

    scheduleUpdateRedirector(nextUpdate);
}

void InstanceInfoController::scheduleUpdateRedirector(EA::TDF::TimeValue& nextUpdate)
{
    if (mUpdateRedirectorTimerId != INVALID_TIMER_ID)
    {
        mSelector.cancelTimer(mUpdateRedirectorTimerId);
        mUpdateRedirectorTimerId = INVALID_TIMER_ID;
    }

    if (!gController->isShuttingDown())
    {
        // On startup, the nextUpdate time will not be returned from the redirector
        if (nextUpdate == 0)
            nextUpdate = UPDATE_REDIRECTOR_BOOTSTRAP_INTERVAL * 1000;

        mUpdateRedirectorTimerId = mSelector.scheduleFiberTimerCall(TimeValue::getTimeOfDay() + nextUpdate,
            this, &InstanceInfoController::updateRedirector, "InstanceInfoController::updateRedirector");
    }
}

void InstanceInfoController::syncRemoteInstanceConnections()
{
    BLAZE_TRACE_LOG(Log::CONTROLLER, "[InstanceInfoController].syncRemoteInstanceConnections: getting peer remote instance addresses from Redis");

    RemoteInstanceInfo remoteInstanceInfo;
    BlazeRpcError rc = fetchRemoteInstanceInfo(remoteInstanceInfo);
    if (rc != ERR_OK)
    {
        BLAZE_WARN_LOG(Log::CONTROLLER, "[InstanceInfoController].syncRemoteInstanceConnections: fetchRemoteInstanceInfo failed with error: " << ErrorHelp::getErrorName(rc));
    }
    else
    {
        gController->handleRemoteInstanceInfo(remoteInstanceInfo, false);
    }

    scheduleSyncRemoteInstanceConnections();
}

void InstanceInfoController::scheduleSyncRemoteInstanceConnections()
{
    if (mSyncRemoteInstanceConnectionsTimerId != INVALID_TIMER_ID)
    {
        mSelector.cancelTimer(mSyncRemoteInstanceConnectionsTimerId);
        mSyncRemoteInstanceConnectionsTimerId = INVALID_TIMER_ID;
    }

    if (!gController->isShuttingDown())
    {
        uint32_t timerIntervalMs = 0;
        if (gController->isMasterController())
        {
            // master controller starts up before any other instances, so sync more often (even after it is in-service)
            timerIntervalMs = SYNC_REMOTE_INSTANCE_CONNECTIONS_MASTER_CONTROLLER_INTERVAL;
        }
        else if (!gController->isInService())
        {
            timerIntervalMs = SYNC_REMOTE_INSTANCE_CONNECTIONS_BOOTSTRAP_INTERVAL;
        }
        else
        {
            timerIntervalMs = SYNC_REMOTE_INSTANCE_CONNECTIONS_ACTIVE_INTERVAL;
        }

        EA::TDF::TimeValue nextSync = timerIntervalMs * 1000;

        mSyncRemoteInstanceConnectionsTimerId = mSelector.scheduleFiberTimerCall(TimeValue::getTimeOfDay() + nextSync,
            this, &InstanceInfoController::syncRemoteInstanceConnections, "InstanceInfoController::syncRemoteInstanceConnections");
    }
}

void InstanceInfoController::shutdownController()
{
    if (mRefreshRedisInstanceIdTimerId != INVALID_TIMER_ID)
    {
        mSelector.cancelTimer(mRefreshRedisInstanceIdTimerId);
        mRefreshRedisInstanceIdTimerId = INVALID_TIMER_ID;
    }

    if (mRefreshRedisInstanceInfoTimerId != INVALID_TIMER_ID)
    {
        mSelector.cancelTimer(mRefreshRedisInstanceInfoTimerId);
        mRefreshRedisInstanceInfoTimerId = INVALID_TIMER_ID;
    }

    if (mUpdateRedirectorTimerId != INVALID_TIMER_ID)
    {
        mSelector.cancelTimer(mUpdateRedirectorTimerId);
        mUpdateRedirectorTimerId = INVALID_TIMER_ID;
    }

    if (mSyncRemoteInstanceConnectionsTimerId != INVALID_TIMER_ID)
    {
        mSelector.cancelTimer(mSyncRemoteInstanceConnectionsTimerId);
        mSyncRemoteInstanceConnectionsTimerId = INVALID_TIMER_ID;
    }
}

void InstanceInfoController::onControllerDrain()
{
    EA::TDF::TimeValue unused;
    updateRedirectorInternal(unused);

    // Following causes the endpoint information to be blanked out from Redis because InstanceInfoController::populateEnvoyEndpoint
    // removes the endpoint for the draining instances.
    mRedisInstanceInfoStage = REDIS_INSTANCE_INFO_STAGE_PUBLISHING;
    refreshRedisInstanceInfoInternal();
}

void InstanceInfoController::refreshRedisInstanceId()
{
    bool signalShutdown = false;
    const InstanceId existingInstanceId = gController->getInstanceId();
    InstanceId newInstanceId = InstanceIdAllocator::getOrRefreshInstanceId(gController->getInstanceName(), existingInstanceId, mBootConfigTdf.getInstanceIdAllocatorConfig().getRedisTTL());
    if (newInstanceId == INVALID_INSTANCE_ID)
    {
        if (mBootConfigTdf.getInstanceIdAllocatorConfig().getMaxRedisRefreshFailures() > 0 && ++mRefreshRedisInstanceIdFailCount > mBootConfigTdf.getInstanceIdAllocatorConfig().getMaxRedisRefreshFailures())
        {
            BLAZE_FATAL_LOG(Log::CONTROLLER, "[InstanceInfoController].refreshRedisInstanceId: Failed to refresh allocated instance id (" << existingInstanceId << ") " << mRefreshRedisInstanceIdFailCount <<
                " times (limit is " << mBootConfigTdf.getInstanceIdAllocatorConfig().getMaxRedisRefreshFailures() << " failures). Shutting down ...");
            signalShutdown = true;
        }
    }
    else if (existingInstanceId == newInstanceId)
    {
        mRefreshRedisInstanceIdFailCount = 0;
    }
    else
    {
        BLAZE_FATAL_LOG(Log::CONTROLLER, "[InstanceInfoController].refreshRedisInstanceId: Allocated instance id (" << newInstanceId << ") does not match existing instance id (" << existingInstanceId <<
            "). This should never happen, and may mean that other instances in the Blaze cluster are incorrectly referencing this instance by its old id. To force those instances to update their mappings, this instance will be shut down.");
        signalShutdown = true;
    }

    if (signalShutdown)
        gProcessController->shutdown(ProcessController::EXIT_INSTANCEID_REFRESH);
    else
        scheduleRefreshRedisInstanceId();
}

void InstanceInfoController::scheduleRefreshRedisInstanceId()
{
    if (mRefreshRedisInstanceIdTimerId != INVALID_TIMER_ID)
    {
        mSelector.cancelTimer(mRefreshRedisInstanceIdTimerId);
        mRefreshRedisInstanceIdTimerId = INVALID_TIMER_ID;
    }

    if (!gController->isShuttingDown())
    {
        mRefreshRedisInstanceIdTimerId = mSelector.scheduleFiberTimerCall(TimeValue::getTimeOfDay() + mBootConfigTdf.getInstanceIdAllocatorConfig().getRedisRefreshInterval(),
            this, &InstanceInfoController::refreshRedisInstanceId, "InstanceInfoController::refreshRedisInstanceId");
    }
}

bool InstanceInfoController::updateRedisKeyExpiry(const char8_t* keyNamespace, const eastl::string& key, EA::TDF::TimeValue relativeTimeout)
{
    bool success = true;

    RedisCluster* cluster = gRedisManager->getRedisCluster(keyNamespace);
    if (cluster != nullptr)
    {
        RedisCommand cmd;
        cmd.add("EXPIRE");
        cmd.addKey(key);

        cmd.add(relativeTimeout.getSec());

        RedisResponsePtr resp;
        RedisError redisErr = cluster->exec(cmd, resp);
        if (redisErr != REDIS_ERR_OK)
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedisKeyExpiry: (" << redisErr << ") error updating expiry on key(" << key << ")");
            success = false;
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedisKeyExpiry: getRedisCluster returned nullptr for namespace: " << keyNamespace);
        success = false;
    }

    return success;
}

bool InstanceInfoController::getRedisKeyExists(const char8_t* keyNamespace, const eastl::string& key)
{
    RedisCluster* cluster = gRedisManager->getRedisCluster(keyNamespace);
    if (cluster != nullptr)
    {
        RedisCommand cmd;
        cmd.add("EXISTS");
        cmd.addKey(key);

        RedisResponsePtr resp;
        RedisError redisErr = cluster->exec(cmd, resp);
        if ((redisErr == REDIS_ERR_OK) && resp->isInteger())
        {
            return (resp->getInteger() == 1);
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].getRedisKeyExists: getRedisCluster returned nullptr for namespace: " << keyNamespace);
    }

    return false;
}

bool InstanceInfoController::updateRedisEnvoyInfoExpiry(EA::TDF::TimeValue relativeTimeout)
{
    bool success = true;
    if (mIsEnvoyInstanceInfoInRedis)
    {
        success &= updateRedisKeyExpiry(RedisManager::FRAMEWORK_NAMESPACE, mInstanceEnvoyResourcesRedisKey, relativeTimeout);
        success &= updateRedisKeyExpiry(RedisManager::FRAMEWORK_NAMESPACE, mInstanceEnvoyVersionInfoRedisKey, relativeTimeout);
    }
    return success;
}

bool InstanceInfoController::updateRedisRemoteInstanceInfoExpiry(EA::TDF::TimeValue relativeTimeout)
{
    bool success = true;

    RedisError redisErr = mRemoteInstanceInfoFieldMap.expire(gController->getInstanceName(), relativeTimeout);
    if (redisErr != REDIS_ERR_OK)
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].updateRedisRemoteInstanceInfoExpiry: (" << redisErr << ") error updating expiry on remote instance info");
        success = false;
    }

    return success;
}

bool InstanceInfoController::isRedisEnvoyInfoMissing()
{
    if (!mIsEnvoyInstanceInfoInRedis)
    {
        // If the info was not stored in Redis to begin with, then it can't be missing
        return false;
    }

    if (!getRedisKeyExists(RedisManager::FRAMEWORK_NAMESPACE, mInstanceEnvoyResourcesRedisKey) ||
        !getRedisKeyExists(RedisManager::FRAMEWORK_NAMESPACE, mInstanceEnvoyVersionInfoRedisKey))
    {
        return true;
    }

    return false;
}

bool InstanceInfoController::isRedisRemoteInstanceInfoMissing()
{
    int64_t result = 0;
    RedisError redisErr = mRemoteInstanceInfoFieldMap.exists(gController->getInstanceName(), &result);
    if (redisErr == REDIS_ERR_OK)
    {
        const bool infoMissing = (result != 1);
        return infoMissing;
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].isRedisRemoteInstanceInfoMissing: (" << redisErr << ") error checking remote instance info existence");
    }

    return true;
}

void InstanceInfoController::refreshRedisInstanceInfo()
{
    bool signalShutdown = false;

    bool success = refreshRedisInstanceInfoInternal();
    if (!success)
    {
        if (gController->getFrameworkConfigTdf().getInstanceInfoPublishingConfig().getMaxRedisRefreshFailures() > 0 && ++mRefreshRedisInstanceInfoFailCount > gController->getFrameworkConfigTdf().getInstanceInfoPublishingConfig().getMaxRedisRefreshFailures())
        {
            BLAZE_FATAL_LOG(Log::CONTROLLER, "[InstanceInfoController].refreshRedisInstanceInfo: Failed to refresh instance info for (" << gController->getInstanceId() << ") " << mRefreshRedisInstanceInfoFailCount <<
                " times (limit is " << gController->getFrameworkConfigTdf().getInstanceInfoPublishingConfig().getMaxRedisRefreshFailures() << " failures). Shutting down ...");
            signalShutdown = true;
        }
    }
    else
    {
        mRefreshRedisInstanceInfoFailCount = 0;
    }

    if (signalShutdown)
        gProcessController->shutdown(ProcessController::EXIT_INSTANCEINFO_REFRESH);
    else
        scheduleRefreshRedisInstanceInfo();
}

void InstanceInfoController::scheduleRefreshRedisInstanceInfo()
{
    if (mRefreshRedisInstanceInfoTimerId != INVALID_TIMER_ID)
    {
        mSelector.cancelTimer(mRefreshRedisInstanceInfoTimerId);
        mRefreshRedisInstanceInfoTimerId = INVALID_TIMER_ID;
    }

    if (!gController->isShuttingDown())
    {
        EA::TDF::TimeValue nextUpdate = gController->getFrameworkConfigTdf().getInstanceInfoPublishingConfig().getRedisRefreshInterval();

        // Refresh more often on startup since the instance info will not be published until the instance is in-service
        if (!gController->isInService())
            nextUpdate = REDIS_INSTANCE_INFO_UPDATE_BOOTSTRAP_INTERVAL * 1000;;

        mRefreshRedisInstanceInfoTimerId = mSelector.scheduleFiberTimerCall(TimeValue::getTimeOfDay() + nextUpdate,
            this, &InstanceInfoController::refreshRedisInstanceInfo, "InstanceInfoController::refreshRedisInstanceInfo");
    }
}

bool InstanceInfoController::refreshRedisInstanceInfoInternal()
{
    bool success = false;

    FiberAutoMutex autoMutex(mRefreshRedisInstanceInfoFiberMutex);
    if (autoMutex.hasLock())
    {
        if (gController->isShuttingDown())
            return false;

        if (mRedisInstanceInfoStage == REDIS_INSTANCE_INFO_STAGE_WAITING)
        {
            // Have not published to Redis yet; bootstrap not complete
            BLAZE_TRACE_LOG(Log::CONTROLLER, "[InstanceInfoController].refreshRedisInstanceInfoInternal: Waiting to publish this instance's info to Redis.");
            success = true;
        }
        else if (mRedisInstanceInfoStage == REDIS_INSTANCE_INFO_STAGE_PUBLISHING)
        {
            BLAZE_TRACE_LOG(Log::CONTROLLER, "[InstanceInfoController].refreshRedisInstanceInfoInternal: Publishing this instance's info to Redis.");

            success = publishInstanceInfoToRedis();
            if (success)
                mRedisInstanceInfoStage = REDIS_INSTANCE_INFO_STAGE_REFRESHING;
        }
        else if (mRedisInstanceInfoStage == REDIS_INSTANCE_INFO_STAGE_REFRESHING)
        {
            // If somehow we have lost the key in Redis (due to a transient error or say sitting on a breakpoint), publish the instance info again
            if (isRedisEnvoyInfoMissing() || isRedisRemoteInstanceInfoMissing())
            {
                // Redis has somehow lost us, republish ourselves now
                mRedisInstanceInfoStage = REDIS_INSTANCE_INFO_STAGE_PUBLISHING;
                BLAZE_INFO_LOG(Log::CONTROLLER, "[InstanceInfoController].refreshRedisInstanceInfoInternal: Redis has lost this instance's info; publishing the instance info again.");

                success = publishInstanceInfoToRedis();
                if (success)
                    mRedisInstanceInfoStage = REDIS_INSTANCE_INFO_STAGE_REFRESHING;
            }
            else
            {
                // Refresh the timeout on the data in Redis
                const EA::TDF::TimeValue instanceInfoTtl = gController->getFrameworkConfigTdf().getInstanceInfoPublishingConfig().getRedisTTL();
                success = updateRedisEnvoyInfoExpiry(instanceInfoTtl);
                success &= updateRedisRemoteInstanceInfoExpiry(instanceInfoTtl);
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].refreshRedisInstanceInfoInternal: Invalid mRedisInstanceInfoStage(" << mRedisInstanceInfoStage << ")");
        }
    }

    return success;
}

void InstanceInfoController::populateSocketAddress(::envoy::api::v2::core::SocketAddress* socketAddress, const Blaze::IEndpoint* blazeEndpoint)
{
    const uint16_t port = blazeEndpoint->getPort(InetAddress::HOST);

    const bool isExternalAddress = (blazeEndpoint->getBindType() != BIND_INTERNAL);
    const InetAddress& inetAddress = isExternalAddress ? NameResolver::getExternalAddress() : NameResolver::getInternalAddress();

    socketAddress->set_protocol(::envoy::api::v2::core::SocketAddress_Protocol_TCP);
    socketAddress->set_address(inetAddress.getIpAsString());
    socketAddress->set_port_value(static_cast<::google::protobuf::uint32>(port));
}

void InstanceInfoController::populateLbEndpoint(::envoy::api::v2::endpoint::LbEndpoint* lbEndpoint, const Blaze::IEndpoint* blazeEndpoint)
{
    ::envoy::api::v2::core::SocketAddress* socketAddress = lbEndpoint->mutable_endpoint()->mutable_address()->mutable_socket_address();
    populateSocketAddress(socketAddress, blazeEndpoint);

    ::envoy::api::v2::core::Metadata* endpointMetadata = lbEndpoint->mutable_metadata();
    ::google::protobuf::Struct& structObj = (*endpointMetadata->mutable_filter_metadata())["envoy.lb"];

    auto& metaMap = *(structObj.mutable_fields());
    metaMap["x-blaze-instanceid"].set_string_value(std::to_string(gController->getInstanceId()));
    metaMap["x-blaze-instance-type"].set_string_value(ServerConfig::InstanceTypeToString(gController->getInstanceType()));
}

void InstanceInfoController::populateEnvoyEndpoint(EnvoyEndpointMap& envoyEndpoints, const Blaze::IEndpoint* blazeEndpoint)
{
    if (!blazeEndpoint->getEnvoyAccessEnabled())
        return;

    const char8_t* resourceName = blazeEndpoint->getEnvoyResourceName();

    ::envoy::api::v2::ClusterLoadAssignment* loadAssignment = BLAZE_NEW ::envoy::api::v2::ClusterLoadAssignment;
    loadAssignment->set_cluster_name(resourceName);
    envoyEndpoints[resourceName] = loadAssignment;

    if (gController->isInService() && !gController->isDraining())
    {
        ::envoy::api::v2::endpoint::LocalityLbEndpoints* localityLbEndpoints = loadAssignment->mutable_endpoints()->Add();
        ::envoy::api::v2::endpoint::LbEndpoint* lbEndpoint = localityLbEndpoints->mutable_lb_endpoints()->Add();
        populateLbEndpoint(lbEndpoint, blazeEndpoint);
    }
}

void InstanceInfoController::populateInstanceInfo(EnvoyEndpointMap& envoyEndpoints, RemoteInstanceAddress& internalFire2Address)
{
    for (const Blaze::Endpoint* endpoint : gController->getConnectionManager().getEndpoints())
    {
        populateEnvoyEndpoint(envoyEndpoints, endpoint);

        if (endpoint->getProtocolType() == Blaze::Protocol::Type::FIRE2 &&
            endpoint->getBindType() == BIND_INTERNAL)
        {
            internalFire2Address.mIp = NameResolver::getInternalAddress().getIp(InetAddress::HOST);
            internalFire2Address.mPort = endpoint->getPort(InetAddress::HOST);
        }
    }

    for (const eastl::unique_ptr<Blaze::Grpc::GrpcEndpoint>& endpoint : gController->getInboundGrpcManager()->getGrpcEndpoints())
    {
        populateEnvoyEndpoint(envoyEndpoints, endpoint.get());
    }
}

bool InstanceInfoController::publishInstanceInfoToRedis()
{
    bool success = true;

    // If the controller is shutting down, bail out.
    // Instance info will expire naturally via the Redis TTL.
    if (gController->isShuttingDown())
        return success;

    EnvoyEndpointMap envoyEndpoints;
    RemoteInstanceAddress internalFire2Address{};
    populateInstanceInfo(envoyEndpoints, internalFire2Address);

    RedisCluster* cluster = gRedisManager->getRedisCluster(RedisManager::FRAMEWORK_NAMESPACE);
    if (cluster != nullptr)
    {
        const EA::TDF::TimeValue instanceInfoTtl = gController->getFrameworkConfigTdf().getInstanceInfoPublishingConfig().getRedisTTL();

        // Publish the instance resources used by the Blaze Control Plane / Envoy
        for (const auto& envoyEndpoint : envoyEndpoints)
        {
            const eastl::string& resourceName = envoyEndpoint.first;
            const ::envoy::api::v2::ClusterLoadAssignment* resourceInfo = envoyEndpoint.second;

            RedisCommand cmd;
            cmd.add("HSET");
            cmd.addKey(mInstanceEnvoyResourcesRedisKey);

            cmd.add(resourceName);
            cmd.add(resourceInfo->SerializeAsString()); // Protobuf

            RedisResponsePtr resp;
            RedisError redisErr = cluster->exec(cmd, resp);
            if (redisErr != REDIS_ERR_OK)
            {
                BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].publishInstanceInfoToRedis: (" << redisErr << ") error publishing key(" << mInstanceEnvoyResourcesRedisKey << ")");
                success = false;
            }
        }

        mIsEnvoyInstanceInfoInRedis = (success && !envoyEndpoints.empty());
        if (mIsEnvoyInstanceInfoInRedis)
        {
            // Set the EXPIRE for the instance resources
            success = updateRedisKeyExpiry(RedisManager::FRAMEWORK_NAMESPACE, mInstanceEnvoyResourcesRedisKey, instanceInfoTtl);

            // Publish the instance version info used by the Blaze Control Plane / Envoy
            {
                RedisCommand cmd;
                cmd.add("SET");
                cmd.addKey(mInstanceEnvoyVersionInfoRedisKey);

                char8_t dataVersionString[BUFFER_SIZE_64];
                TimeValue::getTimeOfDay().toString(dataVersionString, BUFFER_SIZE_64);
                eastl::string versionInfo(eastl::string::CtorSprintf(), "%s,%s", ENVOY_API_VERSION, dataVersionString);
                cmd.add(versionInfo);

                cmd.add("EX");
                cmd.add(instanceInfoTtl.getSec());

                RedisResponsePtr resp;
                RedisError redisErr = cluster->exec(cmd, resp);
                if (redisErr != REDIS_ERR_OK)
                {
                    BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].publishInstanceInfoToRedis: (" << redisErr << ") error publishing key(" << mInstanceEnvoyVersionInfoRedisKey << ")");
                    success = false;
                }
                else
                {
                    BLAZE_INFO_LOG(Log::CONTROLLER, "[InstanceInfoController].publishInstanceInfoToRedis: instance (" << gController->getInstanceName() << ") published version(" << versionInfo << ")");
                }
            }
        }

        // Publish the internal Fire2 endpoint info used for Blaze Remote Instance peer connections
        {
            RemoteInstanceInfoFieldMap::FieldValueMap fieldValueMap =
            {
                { FIELD_REMOTE_INSTANCE_IP, internalFire2Address.mIp },
                { FIELD_REMOTE_INSTANCE_PORT, internalFire2Address.mPort }
            };

            RedisError redisErr = mRemoteInstanceInfoFieldMap.upsert(gController->getInstanceName(), fieldValueMap);
            if (redisErr != REDIS_ERR_OK)
            {
                BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].publishInstanceInfoToRedis: (" << redisErr << ") error publishing remote instance info");
                success = false;
            }
            else
            {
                success &= updateRedisRemoteInstanceInfoExpiry(instanceInfoTtl);
            }
        }
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].publishInstanceInfoToRedis: getRedisCluster returned nullptr for namespace: " << RedisManager::FRAMEWORK_NAMESPACE);
        success = false;
    }

    // Free the memory allocated in populateInstanceInfo
    for (auto& envoyEndpoint : envoyEndpoints)
    {
        delete envoyEndpoint.second;
    }
    envoyEndpoints.clear();

    return success;
}

void InstanceInfoController::refreshCachedInstanceNamesLedger()
{
    eastl::string versionInfo;
    InstanceIdAllocator::getInstanceNamesLedgerVersionInfo(versionInfo);

    bool retrieveLedger = false;
    // Is the cached ledger uninitialized?
    if (mInstanceNamesLedger.empty() || mInstanceNamesLedgerVersionInfo.empty())
        retrieveLedger = true;
    // Has the ledger been updated in Redis?
    else if (mInstanceNamesLedgerVersionInfo != versionInfo)
        retrieveLedger = true;

    if (retrieveLedger)
    {
        mInstanceNamesLedger.clear();
        InstanceIdAllocator::getInstanceNamesLedger(mInstanceNamesLedger);

        // Was the ledger successfully retrieved?
        if (!mInstanceNamesLedger.empty())
            mInstanceNamesLedgerVersionInfo = versionInfo;
    }
}

BlazeRpcError InstanceInfoController::fetchRemoteInstanceInfo(RemoteInstanceInfo& remoteInstanceInfo)
{
    // Get the list of all of the instance names for the peer remote instance info
    // Note: getInstanceNamesLedger may return instance names no longer in use
    refreshCachedInstanceNamesLedger();

    if (mInstanceNamesLedger.empty())
        return ERR_OK;

    BlazeRpcError rc = ERR_OK;

    // Retrieve the instance info for each instance name
    remoteInstanceInfo.mAddrList.reserve(mInstanceNamesLedger.size());
    for (const eastl::string& instanceName : mInstanceNamesLedger)
    {
        RemoteInstanceInfoFieldMap::FieldValueMap fieldValueMap;
        RedisError redisErr = mRemoteInstanceInfoFieldMap.getFieldValueMap(instanceName, fieldValueMap);
        if (redisErr == REDIS_ERR_OK)
        {
            if (!fieldValueMap.empty())
            {
                const auto findIp = fieldValueMap.find(FIELD_REMOTE_INSTANCE_IP);
                const auto findPort = fieldValueMap.find(FIELD_REMOTE_INSTANCE_PORT);
                if (findIp != fieldValueMap.end() && findPort != fieldValueMap.end())
                {
                    remoteInstanceInfo.mAddrList.push_back(RemoteInstanceAddress{findIp->second, static_cast<uint16_t>(findPort->second)});
                }
                else
                {
                    BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].fetchRemoteInstanceInfo: Redis remoteInstanceInfoFieldMap has invalid instance info for key: " << instanceName);
                    rc = ERR_SYSTEM;
                }
            }
            else if (gController->isInService())
            {
                // If reply is an empty list, then the instanceName is either no longer in use (stale) or recently allocated but not yet publishing
                InstanceId instanceId = InstanceIdAllocator::checkAndCleanupInstanceNameOnLedger(instanceName);
                if (instanceId == INVALID_INSTANCE_ID)
                {
                    BLAZE_INFO_LOG(Log::CONTROLLER, "[InstanceInfoController].fetchRemoteInstanceInfo: Removed stale instance from Redis InstanceNamesLedger that was missing published instance info: " << instanceName);
                }
                else
                {
                    BLAZE_INFO_LOG(Log::CONTROLLER, "[InstanceInfoController].fetchRemoteInstanceInfo: Found active instance in Redis InstanceNamesLedger that is missing published instance info: " << instanceName);
                }
            }
        }
        else
        {
            BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].fetchRemoteInstanceInfo: failed to retrieve FieldValueMap, redisErr: " << redisErr);
            rc = ERR_SYSTEM;
        }
    }

    return rc;
}

BlazeRpcError InstanceInfoController::getPeerAddressInfo(Redirector::PeerServerAddressInfo& peerServerAddressInfo)
{
    Redirector::GetPeerServerAddressRequest request;
    request.setAddressType(Redirector::INTERNAL_IPPORT);
    request.setChannel("tcp");
    request.setProtocol(Fire2Protocol::getClassName());
    request.setName(gController->getDefaultServiceName());
    request.setBindType(Blaze::BIND_INTERNAL);
    request.setConnectionTypeString(EndpointConfig::ConnectionTypeToString(EndpointConfig::CONNECTION_TYPE_SERVER));
    
    RpcCallOptions rpcOptions;
    rpcOptions.timeoutOverride = REDIRECTOR_RPC_TIMEOUT * 1000;

    Redirector::RedirectorSlave* rdirProxy = gController->getOutboundConnectionManager().getRedirectorSlaveProxy();
    if (rdirProxy != nullptr)
    {
        BlazeRpcError rc = rdirProxy->getPeerServerAddress(request, peerServerAddressInfo, rpcOptions);
        if (OutboundConnectionManager::isFatalRedirectorError(rc))
            gController->getOutboundConnectionManager().disconnectRedirectorSlaveProxy();
        return rc;
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].getPeerAddressInfo: Attempt to get peer address info from rdir without initializing OutboundConnectionManager with rdir slave address.");
        return ERR_SYSTEM;
    }
}

BlazeRpcError InstanceInfoController::publishFullInstanceInfoToRedirector(Redirector::UpdateServerInstanceInfoResponse& resp)
{
    Redirector::PublishServerInstanceInfoErrorResponse errResp;
    mPublishServerInstanceInfoRequest->setAddressType(Redirector::INTERNAL_IPPORT);
    mPublishServerInstanceInfoRequest->setChannel("tcp");
    mPublishServerInstanceInfoRequest->setProtocol(Fire2Protocol::getClassName());
    mPublishServerInstanceInfoRequest->setBindType(Blaze::BIND_INTERNAL);
    mPublishServerInstanceInfoRequest->setConnectionTypeString(EndpointConfig::ConnectionTypeToString(EndpointConfig::CONNECTION_TYPE_SERVER));

    Redirector::ServerInstanceDynamicData& dynamicData = mPublishServerInstanceInfoRequest->getInfo().getDynamicData();
    populateDynamicInstanceInfo(dynamicData);
    
    RpcCallOptions rpcOptions;
    rpcOptions.timeoutOverride = REDIRECTOR_RPC_TIMEOUT * 1000;

    Redirector::RedirectorSlave* rdirProxy = gController->getOutboundConnectionManager().getRedirectorSlaveProxy();
    if (rdirProxy != nullptr)
    {
        BlazeRpcError rc = rdirProxy->publishServerInstanceInfo(*mPublishServerInstanceInfoRequest, resp, errResp, rpcOptions);
        if (OutboundConnectionManager::isFatalRedirectorError(rc))
            gController->getOutboundConnectionManager().disconnectRedirectorSlaveProxy();
        return rc;
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].publishFullInstanceInfoToRedirector: Attempt to publish server instance info to rdir without initializing OutboundConnectionManager with rdir slave address.");
        return ERR_SYSTEM;
    }
}

void InstanceInfoController::populateDynamicInstanceInfo(Redirector::ServerInstanceDynamicData& dynamicData)
{
    Redirector::RedirectorUtil::populateServerInstanceDynamicData(dynamicData);
    dynamicData.setInService(gController->isInService());
    dynamicData.setLoad(getCurrentLoad());
    dynamicData.setLoadStatsVersion(Redirector::ServerInstanceDynamicData::STATS_VERSION);
    dynamicData.getLoadMap()[Redirector::ServerInstanceDynamicData::LOAD_CONNECTION_COUNT] = gController->getConnectionManager().getNumConnections() + (gController->getInboundGrpcManager() != nullptr ? gController->getInboundGrpcManager()->getNumConnections() : 0);
    dynamicData.getLoadMap()[Redirector::ServerInstanceDynamicData::LOAD_CPU_PCT] = gFiberManager->getCpuUsageForProcessPercent();
    dynamicData.getLoadMap()[Redirector::ServerInstanceDynamicData::LOAD_UPTIME] = (TimeValue::getTimeOfDay() - gController->getStartupTime()).getSec();

    // Calculate memory stats
    uint64_t systemMemSize;
    uint64_t systemMemFree;
    Allocator::getSystemMemorySize(systemMemSize, systemMemFree);
    uint64_t instanceCount = gController->getMachineInstanceCount();
    uint64_t processSize;
    uint64_t residentSize;
    Allocator::getProcessMemUsage(processSize, residentSize);

    uint64_t processMemTotal = systemMemSize;
#ifdef EA_PLATFORM_LINUX
    struct rlimit rlim;
    if (getrlimit(RLIMIT_AS, &rlim) == 0)
    {
        if (rlim.rlim_cur != RLIM_INFINITY)
            processMemTotal = rlim.rlim_cur;
    }
#endif

    dynamicData.getLoadMap()[Redirector::ServerInstanceDynamicData::LOAD_SYSTEM_INSTANCES] = instanceCount;
    dynamicData.getLoadMap()[Redirector::ServerInstanceDynamicData::LOAD_MEM_BOX_TOTAL] = systemMemSize;
    dynamicData.getLoadMap()[Redirector::ServerInstanceDynamicData::LOAD_MEM_BOX_FREE] = systemMemFree;
    dynamicData.getLoadMap()[Redirector::ServerInstanceDynamicData::LOAD_MEM_USED] = processSize;
    dynamicData.getLoadMap()[Redirector::ServerInstanceDynamicData::LOAD_MEM_USED_RSS] = residentSize;
    dynamicData.getLoadMap()[Redirector::ServerInstanceDynamicData::LOAD_MEM_TOTAL] = processMemTotal;
}

BlazeRpcError InstanceInfoController::updateDynamicInstanceInfoOnRedirector(Redirector::UpdateServerInstanceInfoResponse& resp)
{
    const char8_t* serverConnectionTypeString = EndpointConfig::ConnectionTypeToString(EndpointConfig::CONNECTION_TYPE_SERVER);

    Redirector::UpdateServerInstanceInfoRequest req;
    req.setAddressType(Redirector::INTERNAL_IPPORT);
    req.setChannel("tcp");
    req.setProtocol(Fire2Protocol::getClassName());
    req.setBindType(Blaze::BIND_INTERNAL);
    req.setRegistrationId(mRegistrationId);
    req.setVersion(mForceSyncClusterVersion ? 0 : mClusterVersion);
    req.setConnectionTypeString(serverConnectionTypeString);

    populateDynamicInstanceInfo(req.getDynamicData());
    
    RpcCallOptions rpcOptions;
    rpcOptions.timeoutOverride = REDIRECTOR_RPC_TIMEOUT * 1000;
    
    Redirector::RedirectorSlave* rdirProxy = gController->getOutboundConnectionManager().getRedirectorSlaveProxy();
    if (rdirProxy != nullptr)
    {
        BlazeRpcError rc = rdirProxy->updateServerInstanceInfo(req, resp, rpcOptions);
        if (rc == ERR_OK)
            mForceSyncClusterVersion = false;
        
        if (OutboundConnectionManager::isFatalRedirectorError(rc))
            gController->getOutboundConnectionManager().disconnectRedirectorSlaveProxy();
        return rc;
    }
    else
    {
        BLAZE_ERR_LOG(Log::CONTROLLER, "[InstanceInfoController].updateDynamicInstanceInfoOnRedirector: Attempt to update server instance info to rdir without initializing OutboundConnectionManager with rdir slave address.");
        return ERR_SYSTEM;
    }
}

int32_t InstanceInfoController::getCurrentLoad() const 
{  
    return gController->getConnectionManager().getNumConnections() + (gController->getInboundGrpcManager() != nullptr ? gController->getInboundGrpcManager()->getNumConnections() : 0);
}

void InstanceInfoController::populateServerInstanceStaticData()
{
    // Re-create the instance info TDF to ensure that no lingering values from a previous populate get left around in sub-maps, lists, etc.
    mPublishServerInstanceInfoRequest = BLAZE_NEW Redirector::PublishServerInstanceInfoRequest;
    Redirector::RedirectorUtil::populateServerInstanceStaticData(mPublishServerInstanceInfoRequest->getInfo().getStaticData());
}

} // Blaze
