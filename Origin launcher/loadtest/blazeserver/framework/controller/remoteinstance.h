/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_REMOTE_SERVER_INSTANCE_H
#define BLAZE_REMOTE_SERVER_INSTANCE_H

/*** Include files *******************************************************************************/
#include "EASTL/vector_map.h"
#include "framework/connection/connection.h" // ConnecitonListener

namespace Blaze
{

class Controller;
class ServerInstanceInfo;
class OutboundRpcConnection;
class RemoteServerInstanceHandler;

/*! ************************************************************************/
/*! \brief 
*****************************************************************************/
class RemoteServerInstance
    :public eastl::intrusive_hash_node,
    private ConnectionListener
{
    NON_COPYABLE(RemoteServerInstance)
public:
    RemoteServerInstance(const InetAddress& addr);
    ~RemoteServerInstance() override;   
    BlazeRpcError DEFINE_ASYNC_RET(connect());
    BlazeRpcError DEFINE_ASYNC_RET(initialize());
    BlazeRpcError DEFINE_ASYNC_RET(activate());
    BlazeRpcError DEFINE_ASYNC_RET(checkMutualConnectivity());
    void setDraining(bool draining);
    void setLoad(uint32_t totalLoad, uint32_t systemLoad);
    bool shutdown();
    void setInstanceId(InstanceId instanceId);
    void setRegistrationId(uint64_t registrationId);
    void setInstanceName(const char8_t* name);
    void setInstanceTypeName(const char8_t* typeName);
    void setServiceImpact(ServerConfig::ServiceImpact impact);
    void setInstanceTypeIndex(uint8_t typeIndex);
    void setListener(RemoteServerInstanceHandler& listener);
    void clearListener();

    bool hasComponent(ComponentId componentId) const { return mComponentIds.count(componentId) != 0; }

    bool isConnected() const { return mConnection != nullptr; }
    bool isDraining() const { return mIsDraining; }
    bool isShutdown() const { return mIsShutdown; }
    uint32_t getTotalLoad() const { return mTotalLoad; }
    uint32_t getSystemLoad() const { return mSystemLoad; }
    uint32_t getUserLoad() const { return mTotalLoad - mSystemLoad; }

    InstanceId getInstanceId() const { return mInstanceId; }
    uint64_t getRegistrationId() const { return mRegistrationId; }
    const char8_t* getInstanceName() const { return mInstanceName; }
    const char8_t* getInstanceTypeName() const { return mInstanceTypeName; }
    const char8_t* toString(char8_t* buf, uint32_t bufSize) const;
    ServerConfig::InstanceType getInstanceType() const { return mInstanceType; }
    ServerConfig::ServiceImpact getServiceImpact() const { return mServiceImpact; }
    TimeValue getStartupTime() const { return mStartupTime; }
    uint8_t getInstanceTypeIndex() const { return mInstanceTypeIndex; }
    const InetAddress& getInternalAddress() const { return mInternalAddress; }
    const OutboundRpcConnection* getConnection() const { return mConnection; }
    bool getImmediatePartialLocalUserReplication() const { return mImmediatePartialLocalUserReplication; }
    bool getMutuallyConnected() const { return mIsMutuallyConnected; }
    bool isActive() const { return mIsActive; }
    Fiber::FiberGroupId getFiberGroupId() const { return mFiberGroupId; }
    // Number of milliseconds to wait before retring a connection attempt to another instance
    static const uint32_t CONNECT_RETRY_TIME = 5000;
    static const uint32_t MAX_LOAD = 100;
    bool getAddressForEndpointType(bool isInternalAddr, InetAddress& inetAddress, BindType bindType, const char8_t* channel, const char8_t* encoder, const char8_t* decoder) const;
    
private:
    void disconnect();
    void onConnectionDisconnected(Connection& connection) override;

    BlazeRpcError connectInternal();
    BlazeRpcError initializeInternal();
    BlazeRpcError activateInternal();
    BlazeRpcError checkMutualConnectivityInternal();
    void setImmediatePartialLocalUserReplication(bool immediate);

private:
    friend class Connection;
    friend struct eastl::use_intrusive_key<RemoteServerInstance, uint32_t>;
    typedef eastl::hash_set<ComponentId> ComponentIdSet;
    
    InstanceId mInstanceId;
    uint64_t mRegistrationId;
    InetAddress mInternalAddress;
    ComponentIdSet mComponentIds;
    OutboundRpcConnection* mConnection;
    ServerInstanceInfo::ServerEndpointInfoList mEndpointList;
    bool mIsActive;
    bool mIsDraining;
    bool mIsShutdown;
    bool mIsMutuallyConnected;
    bool mImmediatePartialLocalUserReplication;
    uint8_t mInstanceTypeIndex;
    uint32_t mTotalLoad;
    uint32_t mSystemLoad;
    ServerConfig::InstanceType mInstanceType;
    ServerConfig::ServiceImpact mServiceImpact;
    TimeValue mStartupTime;
    RemoteServerInstanceHandler* mHandler;
    char8_t mInstanceName[64];
    char8_t mInstanceTypeName[64];
    Fiber::FiberGroupId mFiberGroupId;
};

class RemoteServerInstanceHandler
{
    friend class RemoteServerInstance;
public:
    virtual ~RemoteServerInstanceHandler() {}
    
private:
    virtual void onRemoteInstanceStateChanged(RemoteServerInstance& instance) = 0;
};

} // Blaze

namespace eastl
{
    template <>
    struct use_intrusive_key<Blaze::RemoteServerInstance, InstanceId>
    {
        const InstanceId operator()(const Blaze::RemoteServerInstance& ri) const
        { 
            return ri.getInstanceId();
        }
    };
}

#endif // BLAZE_REMOTE_SERVER_INSTANCE_H
