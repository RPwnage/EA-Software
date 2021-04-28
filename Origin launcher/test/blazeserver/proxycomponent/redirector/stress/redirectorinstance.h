/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_STRESS_REDIRECTORINSTANCE_H
#define BLAZE_STRESS_REDIRECTORINSTANCE_H

/*** Include files *******************************************************************************/

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#include "blazerpcerrors.h"
#include "EASTL/intrusive_list.h"
#include "EASTL/vector.h"
#include "framework/util/random.h"
#include "stressmodule.h"
#include "stressinstance.h"
#include "loginmanager.h"
#include "redirector/tdf/redirectortypes_server.h"

namespace Blaze
{

namespace Redirector
{
    class RedirectorSlave;
    class RedirectorMaster;
}

namespace Stress
{

class ClusterInstance;
class RedirectorModule;
class RedirectorServerInstance;
class RedirectorClientInstance;

class ServerInstance
{
    NON_COPYABLE(ServerInstance);
public:
    ServerInstance(ClusterInstance& cluster, Redirector::InstanceType type, InstanceId instanceId);
    bool isLegacy() const;
    void populateStaticData(Redirector::ServerInstanceInfoData& info) const;
    bool isMemberOfActiveCluster(const Redirector::ServerAddressInfoList& addrList) const;
    const Redirector::ServerInstance& getInfo() const { return mServerInfo; }
    void copyDynamicDataInto(Redirector::ServerInstanceDynamicData& dynamicData) { mDynamicData.copyInto(dynamicData); }
    ClusterInstance& getCluster();
    bool isActive() const { return mIsActive; }
    void setActive(bool active) { mIsActive = active; }
    void setInService(bool inService);
    void setLoad(int32_t load);
    bool addClient();
    void disconnect();
    static const uint32_t IP_OFFSET = 200000;
   
private:
    void addEndpointAndPort(bool isInternal, uint16_t port);
    bool dropClient();
        
private:
    friend class ClusterInstance;
    ClusterInstance& mClusterInstance;
    Redirector::ServerInstance mServerInfo;
    Redirector::ServerInstanceDynamicData mDynamicData;
    
    bool mIsActive;

    TimeValue mStartupTime;
};

class RedirectorServerInstance : public StressInstance, public AsyncHandler
{
    NON_COPYABLE(RedirectorServerInstance);

public:
    explicit RedirectorServerInstance(RedirectorModule& owner, StressConnection& connection, Login& login, ServerInstance& serverInstance);
    ~RedirectorServerInstance() override;

protected:
    void onLogin(BlazeRpcError result) override{}
    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override{}
    void onDisconnected() override;
    
private:
    const char8_t* getName() const override { return mName.c_str(); }
    bool isConnected() const { return getConnection()->connected(); }
    void reset();
    BlazeRpcError reconnect();
    BlazeRpcError execute() override;
    BlazeRpcError executeMasterLegacy();
    BlazeRpcError executeNonMasterLegacy();
    BlazeRpcError executeServerInstance();
    BlazeRpcError executeExpireClients();

private:
    enum State
    {
        STATE_INIT,
        STATE_PUBLISH,
        STATE_ACTIVE,
        STATE_SHUTDOWN
    };
    State mState;
    eastl::string mName;
    ServerInstance& mServerInstance;
    Blaze::Redirector::RedirectorMaster* mMasterProxy;
    Blaze::Redirector::RedirectorSlave* mSlave;
    uint64_t mRegistrationId;
    uint64_t mClusterVersion;
};

class RedirectorClientInstance : public StressInstance, public AsyncHandler
{
    NON_COPYABLE(RedirectorClientInstance);

public:
    explicit RedirectorClientInstance(RedirectorModule& owner, StressConnection& connection, Login& login, ClusterInstance& clusterInstance);
    ~RedirectorClientInstance() override;

protected:
    void onLogin(BlazeRpcError result) override{}
    void onAsync(uint16_t component, uint16_t type, RawBuffer* payload) override{}
    void onDisconnected() override;

private:
    const char8_t* getName() const override { return mName.c_str(); }
    BlazeRpcError execute() override;
    BlazeRpcError executeClient();
    BlazeRpcError reconnect();
    void setIndex(uint32_t index) { mIndex = index; }
    bool isPrimary() const { return mIndex == 0; }
    bool isConnected() const { return getConnection()->connected(); }
    
private:
    friend class ClusterInstance;
    eastl::string mName;
    ClusterInstance& mClusterInstance;
    Blaze::Redirector::RedirectorSlave* mSlave;
    TimeValue mLastAssignLog;
    uint64_t mSquelchedCount;
    uint32_t mIndex;
};


} // Stress
} // Blaze

#endif // BLAZE_STRESS_REDIRECTORINSTANCE_H

