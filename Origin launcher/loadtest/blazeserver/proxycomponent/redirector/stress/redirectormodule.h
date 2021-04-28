/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_REDIRECTORMODULE_H
#define BLAZE_STRESS_REDIRECTORMODULE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "blazerpcerrors.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/vector.h"
#include "EASTL/string.h"
#include "framework/util/random.h"
#include "stressmodule.h"
#include "stressinstance.h"
#include "loginmanager.h"
#include "redirectorinstance.h"

namespace Blaze
{

namespace Redirector
{
    class RedirectorSlave;
}

namespace Stress
{

class ServerInstance;
class ClusterInstance;
class RedirectorModule;

class ClusterSpec
{
    NON_COPYABLE(ClusterSpec);
public:
    ClusterSpec(RedirectorModule& module, const ConfigMap& clusterMap);
    ~ClusterSpec();
    uint32_t getClientLimit() const { return mClientLimit; }
   
private:
    friend class ServerInstance;
    friend class ClusterInstance;
    friend class RedirectorModule;
    typedef eastl::vector_map<Redirector::InstanceType, uint16_t> ServerCountsByTypeMap;
    typedef eastl::vector<ClusterInstance*> ClusterInstanceList;
    RedirectorModule& mModule;
    eastl::string mServicePrefix;
    uint32_t mClusterCount;
    uint32_t mStressClientCount;
    uint32_t mClientAssignRatePerMin;
    uint32_t mClientDropRatePerMin;
    uint32_t mServerUpateRatePerMin;
    uint32_t mClientLimit;
    bool mIsLegacy;
    ServerCountsByTypeMap mServerCountsMap;
    ClusterInstanceList mClusterInstances;
};

class ClusterInstance
{
    NON_COPYABLE(ClusterInstance);
public:
    ClusterInstance(ClusterSpec& spec, uint32_t index);
    void populateClusterInfo(Redirector::ServerInfoData& data) const;
    void populateGetServerRequest(Redirector::ServerInstanceRequest& req) const;
    ServerInstance* getSlaveInstanceByRdirResponse(const Redirector::ServerInstanceInfo& info) const;
    ServerInstance* getMasterInstanceByRdirResponse(const Redirector::ServerAddressMapResponse& info) const;
    bool isMemberOfActiveCluster(const ServerInstance& svr, const Redirector::ServerAddressInfoList& addrList) const;
    const char8_t* getName() const { return mServiceName.c_str(); }
    uint64_t getTotalClientAddCount() const { return mTotalClientAddCount; }
    uint64_t getTotalClientDropCount() const { return mTotalClientDropCount; }
    uint32_t getClientCount() const { return mClientCount; }
    bool isClientLimitReached() const { return mClientCount >= mSpec.mClientLimit; }
    bool isStressLimitReached() const { return mStressInstanceCount >= mSpec.mStressClientCount; }
    ClusterSpec& getSpec() const { return mSpec; }
    uint32_t getExpiredClientsCount() const;
    TimeValue getNextClientAddDelay() const;
    TimeValue getServerUpdateDelay() const;
    bool isClientAddDelayExpired() const;
    bool addStressInstance(RedirectorClientInstance& client);
    void dropClients(uint32_t dropCount);
    void dropServer(const ServerInstance& server);
    RedirectorClientInstance* getRoundRobinClientInstance();
    bool rampUpAddRate();
    bool rampUpDropRate();
    
private:
    InstanceId createServers(Redirector::InstanceType type, InstanceId startId);
    bool addClient();
    
private:
    friend class ServerInstance; 
    typedef eastl::vector<ServerInstance*> ServerInstanceList;
    typedef eastl::vector<RedirectorClientInstance*> ClientInstanceList;
    ClusterSpec& mSpec;
    uint32_t mIndex;
    uint32_t mClientCount;
    uint32_t mClientAssignRatePerMin;
    uint32_t mClientDropRatePerMin;
    uint32_t mStressInstanceCount;
    uint64_t mTotalClientAddCount;
    uint64_t mTotalClientDropCount;
    TimeValue mLastClientAdded;
    TimeValue mLastClientDropped;
    uint32_t mClientDropSlaveIndex;
    uint32_t mNextClientIndex;
    eastl::string mServiceName;
    ClientInstanceList mClientInstances;
    ServerInstanceList mServerInstances;
    ServerInstanceList mSlaveInstances;
};

class RedirectorModule : public StressModule
{
    NON_COPYABLE(RedirectorModule);
    
public:
    ~RedirectorModule() override;
    bool initialize(const ConfigMap& config, const ConfigBootUtil* bootUtil) override;
    StressInstance* createInstance(StressConnection* connection, Login* login) override;
    const char8_t* getModuleName() override {return "Redirector";}
    static StressModule* create();

private:
    RedirectorModule();
   
private:
    friend class ClusterSpec;
    friend class ClusterInstance;
    friend class ServerInstance;
    typedef eastl::vector<ClusterSpec*> ClusterSpecList;
    typedef eastl::vector<ClusterInstance*> ClusterInstanceList;
    typedef eastl::vector<ServerInstance*> ServerInstanceList;
    ClusterSpecList mClusterSpecs;
    ClusterInstanceList mClusterInstances;
    ServerInstanceList mServerInstances;
    ServerInstanceList mMasterInstances;
    uint32_t mConnectedServerInstanceCount;
    uint32_t mConnectedMasterInstanceCount;
    uint32_t mNextClusterIndex;
};

} // Stress
} // Blaze

#endif // BLAZE_STRESS_REDIRECTORMODULE_H

