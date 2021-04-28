/*************************************************************************************************/
/*!
    \file   grpcendpoint.h

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
    \class GrpcEndpoint

*/
/*************************************************************************************************/
#ifndef BLAZE_GRPC_ENDPOINT_H
#define BLAZE_GRPC_ENDPOINT_H

#include "framework/blazedefines.h"
#include "framework/system/blazethread.h"
#include "framework/system/fiber.h"

#include "EATDF/tdf.h"
#include "EATDF/time.h"
#include "EATDF/tdfobjectid.h"

#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/connection/iendpoint.h"
#include "framework/connection/rpcauthorizer.h"
#include "framework/connection/ratelimiter.h"
#include "framework/component/componentmanager.h"
#include "framework/metrics/metrics.h"
#include "framework/util/alertinfo.h"

#include "framework/grpc/grpcutils.h"

#include <EASTL/vector.h>
#include <EASTL/unique_ptr.h>
#include <EASTL/functional.h>

#include <thread>
#include <atomic>

namespace grpc
{
class Service;
class ServerCompletionQueue;
}

namespace Blaze
{
class Selector;

namespace Grpc
{
class InboundGrpcRequestHandler;

class GrpcEndpoint : public BlazeThread, public IEndpoint
{
public:
    GrpcEndpoint(const char8_t* name, uint16_t port, Selector& selector);
    ~GrpcEndpoint();

    bool initialize(const EndpointConfig::GrpcEndpointConfig& config, const RateConfig& rateConfig, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists);
    void reconfigure(const EndpointConfig::GrpcEndpointConfig& config, const RateConfig& rateConfig, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists);
    void shutdown();
    void beginShutdown();
    bool isShuttingDown() const;
    bool registerServices(const Blaze::ComponentManager& compMgr);
    bool startGrpcServer();

    void processTag(TagInfo tag);

    // BlazeThread implementation
    void stop() override;
    void run() override;

    // Creates a new request handler on this endpoint and takes ownership
    InboundGrpcRequestHandler* createRequestHandler();
    // Destroys the request handler - Will queue the destruction.
    void destroyRequestHandler(InboundGrpcRequestHandler* handler);
    // Get an existing request handler from the sessionKey provided. nullptr if sessionKey not found.
    InboundGrpcRequestHandler* getRequestHandler(const char8_t* sessionKey);

    // An ever increasing counter providing initial session key for the request handler
    uint64_t getNextTempSessionKey() { return ++mTempSessionKeyCounter; }
    // Change the request handler key from it's current temporary session key to the permanent session key.
    void changeRequestHandlerSessionKey(const char8_t* newSessionKey, const char8_t* tempSessionKey);


    const char8_t* getName() const override { return mName.c_str(); }
    uint16_t getPort(InetAddress::Order order) const override
    { 
        if (order == InetAddress::Order::HOST)
            return mPort;
        return htons(mPort);
    }
    BindType getBindType() const override { return mConfig.getBind(); }
    const char8_t* getRateLimiterName() const { return mConfig.getRateLimiter(); }
    const char8_t* getChannelType() const { return mConfig.getChannel(); }
    const char8_t* getProtocolName() const { return "grpc"; }
    const char8_t* getEncoderName() const { return "protobuf"; }
    const char8_t* getDecoderName() const { return "protobuf"; }
    const char8_t* getCommonName() const;
    
    uint32_t getCurrentConnections() const;
    uint32_t getMaxConnections() const { return mConfig.getMaxConnections(); } 
    EA::TDF::TimeValue getCommandTimeout() const { return mConfig.getCommandTimeout(); }

    EndpointConfig::ConnectionType getConnectionType() const { return EndpointConfig::CONNECTION_TYPE_CLIENT; }
    Blaze::RateLimiter &getRateLimiter() { return mRateLimiter; }

    bool isRpcAuthorized(ClientPlatformType platform, EA::TDF::ComponentId cmpId, CommandId cmdId) const { return mRpcAuthorizer.isRpcAuthorized(platform, cmpId, cmdId); }
    
    Blaze::AlertInfo& getTrustedAddressAlertInfo() { return mTrustedAddressAlertInfo; }
    bool isTrustedAddress(uint32_t addr) const
    {
        if (mTrustedNetworks == nullptr)
            return true;
        
        return mTrustedNetworks->match(addr);
    }
    bool isTrustedAddress(const InetAddress& addr) const
    {
        return isTrustedAddress(addr.getIp());
    }

    bool hasInternalNicAccess(uint32_t addr) const
    {
        if (mInternalNicAccessNetworks == nullptr)
            return true;

        return mInternalNicAccessNetworks->match(addr);
    }
    bool hasInternalNicAccess(const InetAddress& addr) const
    {
        return hasInternalNicAccess(addr.getIp());
    }


    // Metrics support
    void getStatusInfo(Blaze::GrpcEndpointStatus& status) const;
    void incrementTotalRpcAuthorizationFailureCount() { mRpcAuthorizer.incrementTotalRpcAuthorizationFailureCount(); }
    uint64_t getTotalRpcAuthorizationFailureCount() const { return mRpcAuthorizer.getTotalRpcAuthorizationFailureCount(); }

    void incrementTotalExceededRpcFiberLimitDropped() { mTotalExceededRpcFiberLimitDropped.increment(); }
    uint64_t getTotalExceededRpcFiberLimitDropped() { return mTotalExceededRpcFiberLimitDropped.getTotal(); }
    void incrementTotalExceededRpcFiberLimitPermitted() { mTotalExceededRpcFiberLimitPermitted.increment(); }
    uint64_t getTotalExceededRpcFiberLimitPermitted() { return mTotalExceededRpcFiberLimitPermitted.getTotal(); }
    uint64_t getCountTotalRejects() { return mCountTotalRejects.getTotal(); }
    void incrementCountTotalRejects() { mCountTotalRejects.increment(); }

    Metrics::MetricsCollection& getEndpointMetricsCollection() { return mEndpointMetricsCollection; }
    Metrics::MetricsCollection& getGrpcMetricsCollection() { return mGrpcMetricsCollection; }

    void updateRpcCountMetric(RpcType rpcType, bool increment);

    bool getInternalCommandsAllowed() const override;
    bool getEnvoyAccessEnabled() const override { return mConfig.getEnvoyAccessEnabled(); }

    bool isTrustedInternalSecureEnvoyEndpoint() const;
    bool isEnvoyXfccTrustedHash(const eastl::string& hash) const;

private:
    void startGrpcServerShutdown(int64_t timespanUS);
    
    void destroyRequestHandlersInternal();
    void expireRequestHandlers();

    Blaze::EndpointConfig::GrpcEndpointConfig mConfig;
    EA::TDF::TdfString mName;

    uint16_t mPort;

    eastl::unique_ptr<grpc::Server> mServer;
    eastl::unique_ptr<grpc::ServerCompletionQueue> mCQ;

    typedef eastl::vector<eastl::unique_ptr<grpc::Service> > GrpcServices;
    GrpcServices mServices;
    grpc::ServerBuilder mServerBuilder;

    typedef eastl::hash_map<FixedString64, eastl::unique_ptr<InboundGrpcRequestHandler>, eastl::string_hash<FixedString64>, eastl::equal_to<FixedString64> > SessionKeyToRequestHandlerMap;
    SessionKeyToRequestHandlerMap mSessionKeyToRequestHandlerMap;

    Blaze::Selector& mSelector;
    Metrics::MetricsCollection& mEndpointMetricsCollection;
    Metrics::MetricsCollection& mGrpcMetricsCollection;

    RpcAuthorizer mRpcAuthorizer;
    bool mShuttingDown;

    eastl::hash_map<grpc::Service*, const Blaze::ComponentData*> mServiceComponentDataMap;
    uint64_t mTempSessionKeyCounter;

    Fiber::EventHandle mAllRequestHandlersDoneEvent;
    eastl::list<InboundGrpcRequestHandler*> mPendingRemovalQueue;
    bool mRequestHandlersPendingRemoval;

    TimerId mExpireRequestHandlersTimerId;

    Blaze::RateLimiter mRateLimiter;
    eastl::unique_ptr<InetFilter> mTrustedNetworks;
    eastl::unique_ptr<InetFilter> mInternalNicAccessNetworks;
    Blaze::AlertInfo mTrustedAddressAlertInfo;

    // An ever increasing number to make sure our events are executed in-order. The fiber system priority is indeterministic if the events came within a micro-second.
    int64_t mFiberPriority; 

    Metrics::Counter mTotalExceededRpcFiberLimitDropped;
    Metrics::Counter mTotalExceededRpcFiberLimitPermitted;
    //The total number of connections rejected (at this endpoint) due to exceeding max limit. This is currently an approximation.
    Metrics::Counter mCountTotalRejects;
    
    Metrics::Gauge mNumRequestHandlers;
    
    Metrics::Gauge mNumUnaryRpcs;
    Metrics::Gauge mNumServerStreamingRpcs;
    Metrics::Gauge mNumClientStreamingRpcs;
    Metrics::Gauge mNumBidiStreamingRpcs;

    bool mEndpointDeletedSentinel;

    std::thread mShutdownThread;
    std::atomic_int64_t mPendingTagCount;

    typedef eastl::hash_set<eastl::string> StringSet;
    StringSet mEnvoyXfccTrustedHashes;
};

} //namespace Grpc
} // namespace Blaze



#endif // BLAZE_GRPC_ENDPOINT_H
