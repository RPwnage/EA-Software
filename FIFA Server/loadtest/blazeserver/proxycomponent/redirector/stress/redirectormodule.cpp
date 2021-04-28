/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class RedirectorModule


*/
/*************************************************************************************************/

/*** Include Files *******************************************************************************/

#include "framework/blaze.h"
#include "EASTL/algorithm.h"
#include "framework/connection/selector.h"
#include "framework/config/config_map.h"
#include "framework/config/config_sequence.h"
#include "framework/protocol/shared/heat2decoder.h"
#include "framework/util/shared/rawbuffer.h"

#include "redirector/rpc/redirectorslave.h"
#include "redirectormodule.h"

namespace Blaze
{
namespace Stress
{

static double US_PER_MINUTE = 60000000.0;

/////////////////////// ClusterSpec //////////////////////

ClusterSpec::ClusterSpec(RedirectorModule& module, const ConfigMap& clusterMap)
: mModule(module)
{
    mServicePrefix = clusterMap.getString("servicePrefix", "unknown-service-prefix-");
    mClusterCount = clusterMap.getUInt32("clusterCount", 0);
    mStressClientCount = clusterMap.getUInt32("stressClientCount", 0);
    mClientAssignRatePerMin = clusterMap.getUInt32("clientAssignRatePerMin", 0);
    if (mClientAssignRatePerMin < 1)
        mClientAssignRatePerMin = 1;
    mClientDropRatePerMin = clusterMap.getUInt32("clientDropRatePerMin", 0);
    if (mClientDropRatePerMin < 1)
        mClientDropRatePerMin = 1;
    mClientLimit = clusterMap.getUInt32("clientLimit", 0);
    mServerUpateRatePerMin = clusterMap.getUInt32("serverUpdateRatePerMin", 0);
    if (mServerUpateRatePerMin < 1)
        mServerUpateRatePerMin = 1;
    mIsLegacy = clusterMap.getBool("isLegacy", true);
    const ConfigSequence* serverSeq = clusterMap.getSequence("servers");
    if (serverSeq != nullptr)
    {
        while (serverSeq->hasNext())
        {
            const ConfigMap* server = serverSeq->nextMap();
            const char8_t* type = server->getString("type", nullptr);
            Redirector::InstanceType t;
            if (type != nullptr && Redirector::ParseInstanceType(type, t))
            {
                mServerCountsMap[t] = server->getUInt16("count", 0);
            }
        }
        for (uint32_t i = 0; i < mClusterCount; ++i)
        {
            ClusterInstance* info = BLAZE_NEW ClusterInstance(*this, i);
            mClusterInstances.push_back(info);
            mModule.mClusterInstances.push_back(info);
        }
    }
}

/////////////////////// ClusterInstance //////////////////////

ClusterInstance::ClusterInstance(ClusterSpec& spec, uint32_t index)
:   mSpec(spec), mIndex(index), mClientCount(0), mClientAssignRatePerMin(spec.mClientAssignRatePerMin), 
    mClientDropRatePerMin(spec.mClientDropRatePerMin), mStressInstanceCount(0), mTotalClientAddCount(0),
    mTotalClientDropCount(0), mClientDropSlaveIndex(0), mNextClientIndex(0)
{
    mServiceName = spec.mServicePrefix;
    mServiceName.append_sprintf("%u", index);
    
    InstanceId instanceId = 0;
    instanceId = createServers(Redirector::CONFIG_MASTER, instanceId);
    instanceId = createServers(Redirector::AUX_MASTER, instanceId);
    instanceId = createServers(Redirector::SLAVE, instanceId);
    instanceId = createServers(Redirector::AUX_SLAVE, instanceId);
}

InstanceId ClusterInstance::createServers(Redirector::InstanceType type, InstanceId startId)
{
    ClusterSpec::ServerCountsByTypeMap::const_iterator end = mSpec.mServerCountsMap.end();
    ClusterSpec::ServerCountsByTypeMap::const_iterator it = mSpec.mServerCountsMap.find(type);
    InstanceId instanceId = startId;
    if (it != end)
    {
        InstanceId endId = startId + it->second;
        for (; instanceId < endId; ++instanceId)
        {
            ServerInstance* info = BLAZE_NEW ServerInstance(*this, it->first, instanceId);
            mServerInstances.push_back(info);
            if (type == Redirector::CONFIG_MASTER)
            {
                mSpec.mModule.mMasterInstances.push_back(info);
            }
            else
            {
                mSpec.mModule.mServerInstances.push_back(info);
                if (type == Redirector::SLAVE)
                    mSlaveInstances.push_back(info);
            }
        }
    }
    return instanceId;
}

bool ClusterInstance::addStressInstance(RedirectorClientInstance& client)
{
    if (mStressInstanceCount < mSpec.mStressClientCount)
    {
        client.setIndex((uint32_t)mClientInstances.size());
        mClientInstances.push_back(&client);
        ++mStressInstanceCount;
        return true;
    }
    return false;
}

bool ClusterInstance::addClient()
{
    if (mClientCount < mSpec.mClientLimit)
    {
        ++mClientCount;
        ++mTotalClientAddCount;
        mLastClientAdded = TimeValue::getTimeOfDay();
        return true;
    }
    return false;
}

void ClusterInstance::dropClients(uint32_t dropCount)
{
    if (mClientCount < dropCount)
        dropCount = mClientCount;
    if (dropCount > 0)
    {
        mClientCount -= dropCount;
        mTotalClientDropCount += dropCount;
        if (!mSlaveInstances.empty())
        {
            size_t size = mSlaveInstances.size();
            size_t i = mClientDropSlaveIndex;
            if (i >= size)
                i = 0;
            do
            {
                ServerInstance* slave = mSlaveInstances[i];
                if (slave->dropClient())
                    --dropCount;
                i = (i + 1) % size;
            }
            while (dropCount > 0);
            mClientDropSlaveIndex = i;
        }
        mLastClientDropped = TimeValue::getTimeOfDay();
    }
}

void ClusterInstance::dropServer(const ServerInstance& server)
{
    uint32_t dropCount = server.getInfo().getLoad();
    if (mClientCount < dropCount)
        dropCount = mClientCount;
    if (dropCount > 0)
    {
        mClientCount -= dropCount;
        mTotalClientDropCount += dropCount;
    }
}

RedirectorClientInstance* ClusterInstance::getRoundRobinClientInstance()
{
    EA_ASSERT_MSG(mNextClientIndex < mClientInstances.size(), "Client index must be less than # of instances!");
    RedirectorClientInstance* result = mClientInstances[mNextClientIndex++];
    mNextClientIndex %= (uint32_t) mClientInstances.size();
    return result;
}

bool ClusterInstance::rampUpAddRate()
{
    if (mClientDropRatePerMin > mClientAssignRatePerMin)
    {
        eastl::swap(mClientAssignRatePerMin, mClientDropRatePerMin);
        return true;
    }
    return false;
}

bool ClusterInstance::rampUpDropRate()
{
    if (mClientAssignRatePerMin > mClientDropRatePerMin)
    {
        eastl::swap(mClientDropRatePerMin, mClientAssignRatePerMin);
        return true;
    }
    return false;
}

void ClusterInstance::populateClusterInfo(Redirector::ServerInfoData& data) const
{
    data.setName(mServiceName.c_str());
    data.getServiceNames().push_back(mServiceName.c_str());
    data.setPlatform("pc");
    data.setVersion("Blaze 3.19.01.0 (CL# unknown)");
    data.setConfigVersion("#CL 123456");
    data.setBuildTime("unknown");
    data.setBuildLocation("unknown");
    data.setBuildTarget("unknown");
    data.setDepotLocation("unknown");
    data.setDefaultDnsAddress(0x7F000001);
    data.setDefaultServiceId(0x45410805);

    for (ServerInstanceList::const_iterator i = mServerInstances.begin(), e = mServerInstances.end(); i != e; ++i)
    {
        ServerInstance& server = **i;
        if (server.isActive())
        {
            Redirector::ServerInstance* inst = nullptr;
            switch (server.getInfo().getInstanceType())
            {
            case Redirector::SLAVE:
                inst = data.getInstances().pull_back();
                break;
            case Redirector::AUX_MASTER:
                inst = data.getAuxMasters().pull_back();
                break;
            case Redirector::CONFIG_MASTER:
                inst = &data.getMasterInstance();
                break;
            case Redirector::AUX_SLAVE:
                inst = data.getAuxSlaves().pull_back();
                break;
            default:
                EA_ASSERT_MSG(false, "Unhandled type!");
            }
            server.getInfo().copyInto(*inst);
        }
    }
}

void ClusterInstance::populateGetServerRequest(Redirector::ServerInstanceRequest& req) const
{
    req.setName(mServiceName.c_str());
    req.setBlazeSDKVersion("Blaze 3.19.01.0 (CL# unknown)");
    req.setBlazeSDKBuildDate("Mar 14 2012 15:34:26");
    req.setClientName("STRESS_Client");
    req.setClientType(Blaze::CLIENT_TYPE_GAMEPLAY_USER);
    req.setClientPlatform(Blaze::pc);
    req.setClientSkuId("STRESS_Id_1");
    req.setClientVersion("STRESS_Version_1_1");
    req.setClientSkuId("STRESS_Id_1");
    req.setDirtySDKVersion("8.18.0.0");
    req.setEnvironment("sdev");
    req.setClientLocale(0x656E5553);
    req.setPlatform("Windows");
    req.setConnectionProfile("standardInsecure_v3");
}

ServerInstance* ClusterInstance::getSlaveInstanceByRdirResponse(const Redirector::ServerInstanceInfo& info) const
{
    const Redirector::IpAddress* addr = info.getAddress().getIpAddress();
    if (addr != nullptr)
    {
        uint32_t instanceId = addr->getIp() - ServerInstance::IP_OFFSET;
        if (instanceId < mServerInstances.size())
        {
            ServerInstance* inst = mServerInstances[instanceId];
            if (inst->getInfo().getInstanceType() == Redirector::SLAVE)
                return inst;
            EA_ASSERT_MSG(false, "Assigned instance must always be a core slave!");
        }
    }
    return nullptr;
}

ServerInstance* ClusterInstance::getMasterInstanceByRdirResponse(const Redirector::ServerAddressMapResponse& info) const
{
    if (info.getAddressMap().size() == 1)
    {
        Redirector::InstanceNameAddressMap::const_iterator itr = info.getAddressMap().begin();
        const Redirector::ServerAddressInfo& addrInfo = itr->second->getServerAddressInfo();
        uint32_t ip = addrInfo.getAddress().getIpAddress()->getIp();
        uint32_t instanceId = ip - ServerInstance::IP_OFFSET;
        if (instanceId < mServerInstances.size())
        {
            ServerInstance* inst = mServerInstances[instanceId];
            if (inst->getInfo().getInstanceType() == Redirector::CONFIG_MASTER)
                return inst;
            EA_ASSERT_MSG(false, "Assigned instance must always be a config master!");
        }
    }
    return nullptr;
}

bool ClusterInstance::isMemberOfActiveCluster(const ServerInstance& svr, const Redirector::ServerAddressInfoList& addrList) const
{
    uint32_t instanceId = svr.getInfo().getInstanceId();
    for (Redirector::ServerAddressInfoList::const_iterator i = addrList.begin(), e = addrList.end(); i != e; ++i)
    {
        const Redirector::ServerAddressInfo& addrInfo = **i;
        if (addrInfo.getType() == Redirector::INTERNAL_IPPORT)
        {
            const Redirector::ServerAddress& addr = addrInfo.getAddress();
            const Redirector::IpAddress* ipAddr = addr.getIpAddress();
            if (ipAddr != nullptr)
            {
                uint32_t ip = ipAddr->getIp();
                uint32_t svrInstanceId = ip - ServerInstance::IP_OFFSET;
                if (svrInstanceId != instanceId)
                {
                    if (svrInstanceId < mServerInstances.size())
                    {
                        ServerInstance* instance = mServerInstances[svrInstanceId];
                        if (!instance->isActive())
                            return false;
                    }
                    else
                    {
                        EA_ASSERT_MSG(false, "Instance id must always match one of the cluster instances!");
                    }
                }
            }
        }
    }
    return true;
}

TimeValue ClusterInstance::getServerUpdateDelay() const
{
    return (int64_t)(US_PER_MINUTE/mSpec.mServerUpateRatePerMin);
}

TimeValue ClusterInstance::getNextClientAddDelay() const
{
    return (int64_t)(US_PER_MINUTE/mClientAssignRatePerMin);
}

bool ClusterInstance::isClientAddDelayExpired() const
{
    // this tells us if it's OK to time out at least ONE of our items
    return (mClientAssignRatePerMin/US_PER_MINUTE*(TimeValue::getTimeOfDay() - mLastClientAdded).getMicroSeconds()) >= 1.0;
}

uint32_t ClusterInstance::getExpiredClientsCount() const
{
    // this gives us the number of clients that have expired since the last drop of our items
    return (uint32_t)(mClientDropRatePerMin/US_PER_MINUTE*(TimeValue::getTimeOfDay() - mLastClientDropped).getMicroSeconds());
}

/////////////////////// RedirectorModule //////////////////////

// static
StressModule* RedirectorModule::create()
{
    return BLAZE_NEW RedirectorModule();
}

RedirectorModule::RedirectorModule()
:   mConnectedServerInstanceCount(0),
    mConnectedMasterInstanceCount(0),
    mNextClusterIndex(0)
{
}

RedirectorModule::~RedirectorModule()
{
}

bool RedirectorModule::initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil)
{
    if (!StressModule::initialize(config, bootUtil))
    {
        BLAZE_ERR_LOG(BlazeRpcLog::redirector, "[RedirectorModule].initialize: Failed to initialize base module!");
        return false;
    }
    
    const ConfigSequence* clusters = config.getSequence("clusters");
    if(clusters == nullptr)
    {
        BLAZE_ERR_LOG(BlazeRpcLog::redirector, "[RedirectorModule].initialize: Error");
        return false;
    }
    
    while (clusters->hasNext())
    {
        const ConfigMap* clusterMap = clusters->nextMap();
        ClusterSpec* spec = BLAZE_NEW ClusterSpec(*this, *clusterMap);
        mClusterSpecs.push_back(spec);
    }

    BLAZE_INFO_LOG(BlazeRpcLog::redirector, "[RedirectorModule].initialize: Config file parsed.");
    
    return true;
}

StressInstance* RedirectorModule::createInstance(StressConnection* connection, Login* login)
{
    StressInstance* stressInstance = nullptr;
    if (mConnectedMasterInstanceCount < mMasterInstances.size())
    {
        // first start all the config masters because legacy config masters need to be balanced *evenly* across all rdir slave conns
        ServerInstance* instance = mMasterInstances[mConnectedMasterInstanceCount++];
        RedirectorServerInstance* server = BLAZE_NEW RedirectorServerInstance(*this, *connection, *login, *instance);
        stressInstance = server;
    }
    else if (mConnectedServerInstanceCount < mServerInstances.size())
    {
        // next start the remaining servers
        ServerInstance* instance = mServerInstances[mConnectedServerInstanceCount++];
        RedirectorServerInstance* server = BLAZE_NEW RedirectorServerInstance(*this, *connection, *login, *instance);
        stressInstance = server;
    }
    else if (mNextClusterIndex < mClusterInstances.size())
    {
        ClusterInstance* instance = mClusterInstances[mNextClusterIndex];
        RedirectorClientInstance* client = BLAZE_NEW RedirectorClientInstance(*this, *connection, *login, *instance);
        instance->addStressInstance(*client);
        if (instance->isStressLimitReached())
        {
            // until cluster fills with the right # of
            // stress simulators, we reuse cluster index
            ++mNextClusterIndex;
        }
        stressInstance = client;
    }
    else
    {
        EA_ASSERT_MSG(false, "Creating redirector stress instance that exceeds # configured servers and clients");
    }
    return stressInstance;
}


} // Stress
} // Blaze

