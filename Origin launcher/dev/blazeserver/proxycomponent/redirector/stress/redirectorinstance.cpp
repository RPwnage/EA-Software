/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RedirectorInstance


*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "framework/connection/selector.h"
#include "framework/connection/nameresolver.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"

#include "redirector/tdf/redirectortypes_server.h"
#include "redirector/rpc/redirectormaster.h"
#include "redirector/rpc/redirectorslave.h"
#include "redirectormodule.h"
#include "framework/redirector/redirectorutil.h"

namespace Blaze
{
namespace Stress
{

static const int64_t DEFAULT_SLEEP_DELAY = 5*1000*1000LL;
static const int64_t ERROR_SLEEP_DELAY = DEFAULT_SLEEP_DELAY*2;
static const int64_t CLIENT_ASSIGN_LOG_DELAY = 5*1000*1000LL;
static const int64_t CLIENT_SECONDARY_INSTANCE_DELAY = 60*60*1000*1000LL; // 1 hour delay for secondary instances

/////////////////////// ServerInstance //////////////////////

ServerInstance::ServerInstance(ClusterInstance& cluster, Redirector::InstanceType type, InstanceId instanceId)
: mClusterInstance(cluster), mIsActive(type == Redirector::CONFIG_MASTER)
{
    mStartupTime = TimeValue::getTimeOfDay();
    eastl::string instanceName = Redirector::InstanceTypeToString(type);
    instanceName.make_lower();
    instanceName.append_sprintf("%u", (uint32_t) instanceId);
    mServerInfo.setInstanceName(instanceName.c_str());
    mServerInfo.setInstanceType(type);
    mServerInfo.setCurrentWorkingDirectory("~/cwd");
    mServerInfo.setInstanceId(instanceId);
    setInService(false);
    setLoad(0);
    addEndpointAndPort(true, (uint16_t)(10000+instanceId));
    if (type == Redirector::SLAVE)
    {
        addEndpointAndPort(false, (uint16_t)(20000+instanceId));
        mServerInfo.getClientTypes().push_back(CLIENT_TYPE_GAMEPLAY_USER);
        mServerInfo.getClientTypes().push_back(CLIENT_TYPE_LIMITED_GAMEPLAY_USER);
        mServerInfo.getClientTypes().push_back(CLIENT_TYPE_TOOLS);
    }
    else if (type == Redirector::AUX_MASTER)
    {
        mServerInfo.getClientTypes().push_back(CLIENT_TYPE_DEDICATED_SERVER);
    }
}

bool ServerInstance::isLegacy() const 
{
    return mClusterInstance.mSpec.mIsLegacy;
}

void ServerInstance::populateStaticData(Redirector::ServerInstanceInfoData& info) const
{
    Redirector::ServerInstanceStaticData& staticData = info.getStaticData();
    Redirector::StaticServerInstance& staticInstance = staticData.getInstance();
    Redirector::RedirectorUtil::copyServerInstanceToStaticServerInstance(mServerInfo, staticInstance);
    Redirector::ServerInfoData clusterInfo;
    mClusterInstance.populateClusterInfo(clusterInfo);
    Redirector::ServerInstanceStaticData::AttributeMapMap& attrMap = staticData.getAttributeMap();
    attrMap[Redirector::ATTRIBUTE_BUILD_LOCATION] = clusterInfo.getBuildLocation();
    attrMap[Redirector::ATTRIBUTE_BUILD_TARGET] = clusterInfo.getBuildTarget();
    attrMap[Redirector::ATTRIBUTE_BUILD_TIME] = clusterInfo.getBuildTime();
    attrMap[Redirector::ATTRIBUTE_CONFIG_VERSION] = clusterInfo.getConfigVersion();
    attrMap[Redirector::ATTRIBUTE_DEPOT_LOCATION] = clusterInfo.getDepotLocation();
    attrMap[Redirector::ATTRIBUTE_VERSION] = clusterInfo.getVersion();
    staticData.setDefaultDnsAddress(clusterInfo.getDefaultDnsAddress());
    staticData.setDefaultServiceId(clusterInfo.getDefaultServiceId());
    staticData.setDefaultServiceName(mClusterInstance.getName());
    ServiceNameInfo* svcNameInfo = staticData.getServiceNames().allocate_element();
    svcNameInfo->setPlatform(Blaze::pc);
    staticData.getServiceNames()[staticData.getDefaultServiceName()] = svcNameInfo;
}

bool ServerInstance::isMemberOfActiveCluster(const Redirector::ServerAddressInfoList& addrList) const
{
    return mClusterInstance.isMemberOfActiveCluster(*this, addrList);
}

ClusterInstance& ServerInstance::getCluster() 
{
    return mClusterInstance;
}

void ServerInstance::setInService(bool inService)
{
    mServerInfo.setInService(inService);
    mDynamicData.setInService(inService);
}

void ServerInstance::setLoad(int32_t load)
{
    mServerInfo.setLoad(load);
    mDynamicData.setLoad(load);
    mDynamicData.getLoadMap()[Redirector::ServerInstanceDynamicData::LOAD_CONNECTION_COUNT] = load;
    mDynamicData.getLoadMap()[Redirector::ServerInstanceDynamicData::LOAD_CPU_PCT] = load;
    mDynamicData.getLoadMap()[Redirector::ServerInstanceDynamicData::LOAD_UPTIME] = (TimeValue::getTimeOfDay() - mStartupTime).getSec();
}

bool ServerInstance::addClient()
{
    if (mClusterInstance.addClient())
    {
        setLoad(mServerInfo.getLoad() + 1);
        return true;
    }
    return false;
}

bool ServerInstance::dropClient()
{
    int32_t load = mServerInfo.getLoad();
    if (load > 0)
    {
        load--;
        setLoad(load - 1);
        return true;
    }
    return false;
}

void ServerInstance::addEndpointAndPort(bool isInternal, uint16_t port)
{
    Redirector::ServerInstance::ServerEndpointInfoList& eps = mServerInfo.getEndpoints();
    Redirector::ServerEndpointInfo& ep = *eps.pull_back();
    ep.setBindType(isInternal ? BIND_INTERNAL : BIND_EXTERNAL);
    ep.setChannel("tcp");
    ep.setEncoder("heat2");
    ep.setDecoder("heat2");
    ep.setProtocol("fire2");
    ep.setMaxConnections(100);
    ep.setCurrentConnections(0);
    Redirector::ServerEndpointInfo::ServerAddressInfoList& addrs = ep.getAddresses();
    Redirector::ServerAddressInfo& addr = *addrs.pull_back();
    Redirector::IpAddress ip;
    ip.setIp(IP_OFFSET + mServerInfo.getInstanceId());
    ip.setPort(port);
    addr.setType(isInternal ? Redirector::INTERNAL_IPPORT : Redirector::EXTERNAL_IPPORT);
    addr.getAddress().setIpAddress(&ip);
}

void ServerInstance::disconnect()
{
    Redirector::InstanceType type = getInfo().getInstanceType();
    if (isLegacy())
    {
        if (type != Redirector::CONFIG_MASTER)
        {
            setActive(false);
        }
    }
    else
    {
        setActive(false);
    }
    setInService(false);
    mClusterInstance.dropServer(*this);
    setLoad(0);
}

/////////////////////// RedirectorServerInstance //////////////////////

RedirectorServerInstance::RedirectorServerInstance(RedirectorModule& owner, StressConnection& connection, Login& login, ServerInstance& serverInstance)
:   StressInstance(&owner, &connection, &login, BlazeRpcLog::redirector),
    mState(STATE_INIT),
    mServerInstance(serverInstance),
    mMasterProxy(nullptr),
    mSlave(nullptr),
    mRegistrationId(0),
    mClusterVersion(0)
{
    if (mServerInstance.isLegacy())
        mMasterProxy = BLAZE_NEW Redirector::RedirectorMaster(connection);
    mSlave = BLAZE_NEW Redirector::RedirectorSlave(connection);
}

RedirectorServerInstance::~RedirectorServerInstance()
{
    if (mServerInstance.isLegacy())
        delete mMasterProxy;
    delete mSlave;
}

void RedirectorServerInstance::reset()
{
    mServerInstance.disconnect();
    mRegistrationId = 0;
    mClusterVersion = 0;
    mState = STATE_INIT;
}

BlazeRpcError RedirectorServerInstance::reconnect()
{
    StressConnection& conn = *getConnection();
    if (!conn.connected())
    {
        char8_t buf[128];
        BLAZE_TRACE_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].reconnect: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
            << ":" << mServerInstance.getCluster().getName() 
            << ") reconnecting to: " << conn.getAddress().toString(buf, sizeof(buf)));
        if(!conn.connect(true))
        {
            BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].reconnect: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
                << ":" << mServerInstance.getCluster().getName() 
                << ") failed reconnect to: " << conn.getAddress().toString(buf, sizeof(buf)));
            return ERR_SYSTEM;
        }
        BLAZE_INFO_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].reconnect: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
            << ":" << mServerInstance.getCluster().getName() 
            << ") reconnected to: " << conn.getAddress().toString(buf, sizeof(buf)));
    }
    return ERR_OK;
}

void RedirectorServerInstance::onDisconnected()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].onDisconnected: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
        << ":" << mServerInstance.getCluster().getName() << ") disconnected!");
    reset();
}

BlazeRpcError RedirectorServerInstance::execute()
{
    BlazeRpcError rc = ERR_OK;
    TimeValue sleepDelay = mServerInstance.getCluster().getServerUpdateDelay();
    TimeValue startTime = TimeValue::getTimeOfDay();
    
    if (!isConnected() && getOwner()->getReconnect())
    {
        rc = reconnect();
    }
    
    if (rc == ERR_OK)
    {
        Redirector::InstanceType type = mServerInstance.getInfo().getInstanceType();
        if (mServerInstance.isLegacy())
        {
            if (type == Redirector::CONFIG_MASTER)
            {
                rc = executeMasterLegacy();
            }
            else
            {
                rc = executeNonMasterLegacy();
            }
        }
        else
        {
            rc = executeServerInstance();
        }

        if (type == Redirector::CONFIG_MASTER)
        {
            rc = executeExpireClients();
        }
    }

    // sleep longer if we encounter errors, 
    // because there is no point in spamming the logs
    if (rc != ERR_OK)
        sleepDelay = ERROR_SLEEP_DELAY;
        
    TimeValue endTime = TimeValue::getTimeOfDay();
    TimeValue targetTime = startTime + sleepDelay;
    if (endTime < targetTime)
    {
        rc = gSelector->sleep(targetTime - endTime);
    }
    else if (endTime > targetTime)
    {
        // this means that we are too busy and the timer did not wake up the fiber on time
        BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].execute: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
            << ":" << mServerInstance.getCluster().getName() 
            << ") Execution unexpectedly delayed by: " 
            << (endTime - targetTime).getMicroSeconds() << "us.");
    }
    
    return rc;
}

BlazeRpcError RedirectorServerInstance::executeMasterLegacy()
{
    BlazeRpcError rc = ERR_OK;
    Redirector::UpdateServerInfoRequest req;
    Redirector::UpdateServerInfoErrorResponse rsp;
    Redirector::ServerInfoData& data = req.getInfo();
    mServerInstance.getCluster().populateClusterInfo(data);
    Fiber::setCurrentContext("RedirectorServerInstance::updateServerInfo");
    rc = mMasterProxy->updateServerInfo(req, rsp);
    if (rc == ERR_OK)
    {
        // configMaster is already active when we update the server info
        if (mState == STATE_INIT)
        {
            mServerInstance.setInService(true);
            mState = STATE_ACTIVE;
        }
    }
    else
    {
        BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].executeMasterLegacy: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
            << ":" << mServerInstance.getCluster().getName() 
            << ") Failed to update server info: " << Blaze::ErrorHelp::getErrorName(rc));
    }
    return rc;
}

BlazeRpcError RedirectorServerInstance::executeNonMasterLegacy()
{
    BlazeRpcError rc = ERR_OK;
    
    if (mState == STATE_INIT)
    {
        Redirector::ServerAddressMapRequest req;
        Redirector::ServerAddressMapResponse resp;
        req.setInstanceType(Redirector::CONFIG_MASTER);
        req.setChannel("tcp");
        req.setProtocol("fire2");
        req.setAddressType(Redirector::INTERNAL_IPPORT);
        req.setName(mServerInstance.getCluster().getName());
        Fiber::setCurrentContext("RedirectorServerInstance::getServerAddressMap");
        rc = mSlave->getServerAddressMap(req, resp);
        if (rc == ERR_OK)
        {
            ServerInstance* master = mServerInstance.getCluster().getMasterInstanceByRdirResponse(resp);
            if (master != nullptr)
            {
                mServerInstance.setActive(true);
                mServerInstance.setInService(true);
                mState = STATE_ACTIVE;
            }
            else
            {
                if (!resp.getAddressMap().empty())
                {
                    BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].executeNonMasterLegacy: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
                        << ":" << mServerInstance.getCluster().getName() 
                        << ") Returned response does not contain config master");
                }
                rc = ERR_SYSTEM;
            }
        }
        else
        {
            BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].executeNonMasterLegacy: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
                << ":" << mServerInstance.getCluster().getName() 
                << ") Failed to get server address map: " << Blaze::ErrorHelp::getErrorName(rc));
        }
    }
    else
    {
        Fiber::setCurrentContext("RedirectorServerInstance::idle");
    }

    return rc;
}

BlazeRpcError RedirectorServerInstance::executeExpireClients()
{
    BlazeRpcError rc = ERR_OK;
    if (mServerInstance.getCluster().getClientCount() > 0)
    {
        // decay the number of client connections in association with the rate required
        uint32_t expiredClients = mServerInstance.getCluster().getExpiredClientsCount();
        if (expiredClients > 0)
        {
            if (expiredClients > mServerInstance.getCluster().getClientCount())
                expiredClients = mServerInstance.getCluster().getClientCount();
            mServerInstance.getCluster().dropClients(expiredClients);
            BLAZE_INFO_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].executeExpireClients: ServerInstance(" << mServerInstance.getCluster().getName() 
                << ") Clients expired: " << expiredClients
                << ", active: " << mServerInstance.getCluster().getClientCount());
            if (mServerInstance.getCluster().getClientCount() == 0)
            {
                BLAZE_INFO_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].executeExpireClients: ServerInstance(" << mServerInstance.getCluster().getName() 
                    << ") Emptied cluster of clients.");
                if (mServerInstance.getCluster().rampUpAddRate())
                {
                    // switch expiry and drop rates
                    BLAZE_INFO_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].executeExpireClients: ServerInstance(" << mServerInstance.getCluster().getName() 
                        << ") Ramp up client assign rate.");
                }
            }
        }
    }
    return rc;
}

BlazeRpcError RedirectorServerInstance::executeServerInstance()
{
    BlazeRpcError rc = ERR_OK;

    switch (mState)
    {
        case STATE_INIT:
        {
            Redirector::GetPeerServerAddressRequest req;
            Redirector::PeerServerAddressInfo resp;
            req.setAddressType(Redirector::INTERNAL_IPPORT);
            req.setChannel("tcp");
            req.setProtocol("fire2");
            req.setName(mServerInstance.getCluster().getName());
            req.setBindType(Blaze::BIND_INTERNAL);
            Fiber::setCurrentContext("RedirectorServerInstance::getPeerServerAddress");
            rc = mSlave->getPeerServerAddress(req, resp);
            if (rc == ERR_OK)
            {
                mState = STATE_PUBLISH;
            }
            else
            {
                BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].executeServerInstance: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
                    << ":" << mServerInstance.getCluster().getName() 
                    << ") Failed to get peer server address: " << Blaze::ErrorHelp::getErrorName(rc));
            }
        }
        break;
        case STATE_PUBLISH:
        {
            Redirector::PublishServerInstanceInfoRequest req;
            Redirector::UpdateServerInstanceInfoResponse resp;
            Redirector::PublishServerInstanceInfoErrorResponse err;
            mServerInstance.populateStaticData(req.getInfo());
            req.setAddressType(Redirector::INTERNAL_IPPORT);
            req.setChannel("tcp");
            req.setProtocol("fire2");
            req.setBindType(Blaze::BIND_INTERNAL);
            Redirector::ServerInstanceDynamicData& dynamicData = req.getInfo().getDynamicData();
            mServerInstance.copyDynamicDataInto(dynamicData);
            Fiber::setCurrentContext("RedirectorServerInstance::publishServerInstanceInfo");
            rc = mSlave->publishServerInstanceInfo(req, resp, err);
            if (rc == ERR_OK)
            {
                mServerInstance.setActive(true);
                mRegistrationId = resp.getRegistrationId();
                mClusterVersion = resp.getPeerServerAddressInfo().getVersion();
                mState = STATE_ACTIVE;
            }
            else
            {
                BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].executeServerInstance: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
                    << ":" << mServerInstance.getCluster().getName() 
                    << ") Failed to publish server instance: " 
                    << Blaze::ErrorHelp::getErrorName(rc) 
                    << ", conficting instance name: " << err.getConflictingInstanceName()
                    << ", conficting service name: " << err.getConflictingServiceName());
            }
        }
        break;
        case STATE_ACTIVE:
        {
            Redirector::UpdateServerInstanceInfoRequest req;
            Redirector::UpdateServerInstanceInfoResponse resp;
            req.setAddressType(Redirector::INTERNAL_IPPORT);
            req.setChannel("tcp");
            req.setProtocol("fire2");
            req.setBindType(Blaze::BIND_INTERNAL);
            req.setRegistrationId(mRegistrationId);
            req.setVersion(mClusterVersion);
            mServerInstance.copyDynamicDataInto(req.getDynamicData());
            Fiber::setCurrentContext("RedirectorServerInstance::updateServerInstanceInfo");
            rc = mSlave->updateServerInstanceInfo(req, resp);
            if (rc == ERR_OK)
            {
                mRegistrationId = resp.getRegistrationId();
                mClusterVersion = resp.getPeerServerAddressInfo().getVersion();
                if (!mServerInstance.getInfo().getInService())
                {
                    // iterate through all the servers in our cluster that match the published instances
                    // and if they are all active, then mark ourselves as inservice.
                    if (mServerInstance.isMemberOfActiveCluster(resp.getPeerServerAddressInfo().getAddressList()))
                    {
                        mServerInstance.setInService(true);
                    }
                }
            }
            else
            {
                if (rc == REDIRECTOR_NO_MATCHING_INSTANCE)
                {
                    BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].executeServerInstance: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
                        << ":" << mServerInstance.getCluster().getName() 
                        << ") Instance not found, publishing full info.")
                    mState = STATE_PUBLISH;
                }
                else
                {
                    BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].executeServerInstance: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
                        << ":" << mServerInstance.getCluster().getName() 
                        << ") Failed to update server instance info: " << Blaze::ErrorHelp::getErrorName(rc));
                }
            }
        }
        break;
        default:
            BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorServerInstance].executeServerInstance: ServerInstance(" << mServerInstance.getInfo().getInstanceName() 
                << ":" << mServerInstance.getCluster().getName() 
                << ") Unhandled state: " << mState);
        break;
    }

    return rc;
}

/////////////////////// RedirectorClientInstance //////////////////////

RedirectorClientInstance::RedirectorClientInstance(RedirectorModule& owner, StressConnection& connection, Login& login, ClusterInstance& clusterInstance)
:   StressInstance(&owner, &connection, &login, BlazeRpcLog::redirector), mClusterInstance(clusterInstance),
    mSlave(BLAZE_NEW Redirector::RedirectorSlave(connection)), mSquelchedCount(0)
{
}

RedirectorClientInstance::~RedirectorClientInstance()
{
    delete mSlave;
}

void RedirectorClientInstance::onDisconnected()
{
    BLAZE_TRACE_LOG(BlazeRpcLog::redirector, "[RedirectorClientInstance].onDisconnected: ClientInstance(" << mClusterInstance.getName() << ") disconnected!");
}

BlazeRpcError RedirectorClientInstance::execute()
{
    BlazeRpcError rc = ERR_OK;
    TimeValue sleepDelay = DEFAULT_SLEEP_DELAY;
    if (isPrimary())
    {
        if (!isConnected() && getOwner()->getReconnect())
        {
            BLAZE_INFO_LOG(BlazeRpcLog::redirector, "[RedirectorClientInstance].execute: ServerInstance(" << mClusterInstance.getName() 
                << ")  execute reconnect");
            rc = reconnect();
        }
        if (rc == ERR_OK)
        {
            if (!mClusterInstance.isClientLimitReached())
            {
                if (mClusterInstance.isClientAddDelayExpired())
                {
                    rc = executeClient();
                }
                // sleep longer if we encounter errors, 
                // because there is no point in spamming the logs
                if (rc == ERR_OK)
                    sleepDelay = mClusterInstance.getNextClientAddDelay();
                else
                    sleepDelay = ERROR_SLEEP_DELAY;
            }
            else if (mClusterInstance.getSpec().getClientLimit() > 0)
            {
                BLAZE_INFO_LOG(BlazeRpcLog::redirector, "[RedirectorClientInstance].execute: ClientInstance(" << mClusterInstance.getName() 
                    << ") Filled up cluster with clients.");
                if (mClusterInstance.rampUpDropRate())
                {
                    // switch expiry and drop rates
                    BLAZE_INFO_LOG(BlazeRpcLog::redirector, "[RedirectorClientInstance].execute: ClientInstance(" << mClusterInstance.getName() 
                        << ") Ramp up client drop rate.");
                }
            }
        }
    }
    else
    {
        sleepDelay = CLIENT_SECONDARY_INSTANCE_DELAY;
    }
    
    rc = gSelector->sleep(sleepDelay);
    
    return rc;
}

BlazeRpcError RedirectorClientInstance::executeClient()
{
    BlazeRpcError rc = ERR_OK;
    Redirector::ServerInstanceRequest request;
    Redirector::ServerInstanceInfo response;
    Redirector::ServerInstanceError error;
    Fiber::setCurrentContext("RedirectorClientInstance::addClient");
    mClusterInstance.populateGetServerRequest(request);
    // to ensure fairness, each RedirectorClientInstance adheres to strict 
    // round robin policy enforced via an algorithm in the ClusterInstance
    RedirectorClientInstance* client = mClusterInstance.getRoundRobinClientInstance();
    if (!client->isConnected() && getOwner()->getReconnect())
    {
        rc = client->reconnect();
    }
    if (rc == ERR_OK)
        rc = client->mSlave->getServerInstance(request, response, error);
    if (rc == ERR_OK)
    {
        ServerInstance* slave = mClusterInstance.getSlaveInstanceByRdirResponse(response);
        if (slave != nullptr)
        {
            if (slave->addClient())
            {
                TimeValue now = TimeValue::getTimeOfDay();
                if ((now - mLastAssignLog).getMicroSeconds() >= CLIENT_ASSIGN_LOG_DELAY)
                {
                    BLAZE_INFO_LOG(BlazeRpcLog::redirector, "[RedirectorClientInstance].executeClient: ClientInstance(" << mClusterInstance.getName() << ":" << client->mIndex << ") Client #" 
                        << slave->getCluster().getTotalClientAddCount() << "->" << slave->getInfo().getInstanceName()
                        << " w/load " << slave->getInfo().getLoad() << ", squelched " << mSquelchedCount << " others.");
                    mLastAssignLog = now;
                    mSquelchedCount = 0;
                }
                else
                    ++mSquelchedCount;
            }
            else
            {
                BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorClientInstance].executeClient: ClientInstance(" << mClusterInstance.getName() << ") failed to add client to slave instance.");
                rc = ERR_SYSTEM;
            }
        }
        else
        {
            BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorClientInstance].executeClient: ClientInstance(" << mClusterInstance.getName() << ") no valid slave instance found.");
            rc = ERR_SYSTEM;
        }
    }
    else if (rc != REDIRECTOR_NO_MATCHING_INSTANCE)
    {
        BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorClientInstance].executeClient: ClientInstance(" << mClusterInstance.getName() 
            << ") failed to get a server instance, rc: " << ErrorHelp::getErrorName(rc));
    }
    return rc;
}

BlazeRpcError RedirectorClientInstance::reconnect()
{
    StressConnection& conn = *getConnection();
    if (!conn.connected())
    {
        char8_t buf[128];
        BLAZE_TRACE_LOG(BlazeRpcLog::redirector, "[RedirectorClientInstance].reconnect: ClientInstance(" << mClusterInstance.getName() 
            << ") reconnecting to: " << conn.getAddress().toString(buf, sizeof(buf)));
        if(!conn.connect(true))
        {
            BLAZE_WARN_LOG(BlazeRpcLog::redirector, "[RedirectorClientInstance].reconnect: ClientInstance(" << mClusterInstance.getName() 
                << ") failed reconnect to: " << conn.getAddress().toString(buf, sizeof(buf)));
            return ERR_SYSTEM;
        }
        BLAZE_INFO_LOG(BlazeRpcLog::redirector, "[RedirectorClientInstance].reconnect: ClientInstance(" << mClusterInstance.getName() 
            << ") reconnected to: " << conn.getAddress().toString(buf, sizeof(buf)));
    }
    return ERR_OK;
}

} // Stress
} // Blaze

