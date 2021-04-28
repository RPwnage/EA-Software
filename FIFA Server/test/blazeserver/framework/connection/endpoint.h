/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_ENDPOINT_H
#define BLAZE_ENDPOINT_H

/*** Include files *******************************************************************************/

#include "EATDF/time.h"
#include "framework/connection/iendpoint.h"
#include "framework/connection/inetaddress.h"
#include "framework/connection/inetfilter.h"
#include "framework/connection/ratelimiter.h"
#include "framework/connection/sslcontext.h"
#include "framework/tdf/frameworkconfigtypes_server.h"
#include "framework/connection/rpcauthorizer.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/
#define MAX_ENDPOINT_NAME_LENGTH    40

namespace Blaze
{
class Component;
class InetAddress;
class Channel;

class Endpoint : public IEndpoint
{
    NON_COPYABLE(Endpoint);

public:

    Endpoint(const char8_t* name, const char8_t* protocol, const char8_t* encoder,
            const char8_t* decoder, uint32_t maxFrameSize = 262144);
    virtual ~Endpoint();

    const char8_t* getName() const override { return mEndpointName; }
    uint16_t getPort(InetAddress::Order order = InetAddress::NET) const override;
    Channel* getChannel() const { return mChannel; }
    BindType getBindType() const override { return mConfig.getBind(); }
    const char8_t* getRateLimiterName() const { return mConfig.getRateLimiter(); }
    const char8_t* getChannelType() const { return mConfig.getChannel(); }
    const char8_t* getProtocolName() const { return mConfig.getProtocol(); }
    Protocol::Type getProtocolType() const { return mProtocolType; }
    const char8_t* getEncoderName() const { return mConfig.getEncoder(); }
    Encoder::Type getEncoderType() const { return mEncoderType; }
    const char8_t* getDecoderName() const { return mConfig.getDecoder(); }
    Decoder::Type getDecoderType() const { return mDecoderType; }
    bool shouldSerializeRequests() const { return mConfig.getSerializeRequests(); }
    EA::TDF::TimeValue getInactivityTimeout() const { return mConfig.getInactivityTimeout(); }
    EA::TDF::TimeValue getAssumeResponsivePeriod() const { return mConfig.getAssumeResponsivePeriod(); }
    bool getIgnoreInactivityTimeoutEnabled() const { return mConfig.getEnableInactivityTimeoutBypass(); }
    EA::TDF::TimeValue getMaximumPingSuspensionPeriod() const { return mConfig.getMaximumInactivityTimeoutSuspensionPeriod(); }
    bool getTcpKeepAlive() const { return mConfig.getTcpKeepAlive(); }
    EA::TDF::TimeValue getFiberOverrunWarnLogInterval() const { return mConfig.getFiberOverrunWarnLogInterval(); }

    const InetAddress* getServerAddress() const { return mServerAddress; }

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

    uint32_t getMaxFrameSize() const { return mConfig.getMaxFrameSize(); }
    EA::TDF::TimeValue getCommandTimeout() const { return mConfig.getCommandTimeout(); }
    EA::TDF::TimeValue getSslHandShakeTimeout() const { return mConfig.getSslHandShakeTimeout(); }
    uint32_t getMaxConnections() const { return mConfig.getMaxConnections(); }
    uint32_t getMaxAcceptsPerIdle() const { return mConfig.getMaxAcceptsPerIdle(); } 
    EndpointConfig::ConnectionType getConnectionType() const { return mConfig.getConnectionType(); }

    uint64_t getCountTotalRejects(){return mCountTotalRejects.getTotal();}
    void incrementCountTotalRejects(){mCountTotalRejects.increment();}

    uint64_t getCountTotalRejectsMaxRateLimit(){return mCountTotalRejectsMaxRateLimit.getTotal();}
    void incrementCountTotalRejectsMaxRateLimit(){mCountTotalRejectsMaxRateLimit.increment();}

    void incrementHighWatermarks() { mHighWatermarks.increment(); }
    void decrementHighWatermarks() { mHighWatermarks.decrement(); }
    uint64_t getHighWatermarks() { return mHighWatermarks.get(); }

    void incrementTotalHighWatermarks() { mTotalHighWatermarks.increment(); }
    uint64_t getTotalHighWatermarks() { return mTotalHighWatermarks.getTotal(); }

    void incrementTotalExceededDataCloses() { mTotalExceededDataCloses.increment(); }
    uint64_t getTotalExceededDataCloses() { return mTotalExceededDataCloses.getTotal(); }

    void incrementTotalResumedConnections() { mTotalResumedConnections.increment(); }
    uint64_t getTotalResumedConnections() { return mTotalResumedConnections.getTotal(); }

    void incrementTotalResumedConnectionsFailed() { mTotalResumedConnectionsFailed.increment(); }
    uint64_t getTotalResumedConnectionsFailed() { return mTotalResumedConnectionsFailed.getTotal(); }

    void incrementTotalRpcAuthorizationFailureCount() { mRpcAuthorizer.incrementTotalRpcAuthorizationFailureCount(); }
    uint64_t getTotalRpcAuthorizationFailureCount() const { return mRpcAuthorizer.getTotalRpcAuthorizationFailureCount(); }

    void incrementTotalExceededRpcFiberLimitDropped() { mTotalExceededRpcFiberLimitDropped.increment(); }
    uint64_t getTotalExceededRpcFiberLimitDropped() { return mTotalExceededRpcFiberLimitDropped.getTotal(); }
    void incrementTotalExceededRpcFiberLimitPermitted() { mTotalExceededRpcFiberLimitPermitted.increment(); }
    uint64_t getTotalExceededRpcFiberLimitPermitted() { return mTotalExceededRpcFiberLimitPermitted.getTotal(); }

    void incrementSendMetrics(uint64_t bytesWritten)
    {
        mTotalSendBytes.increment(bytesWritten);
        mTotalSendCount.increment();
    }
    uint64_t getTotalSendBytes() const { return mTotalSendBytes.getTotal(); }
    uint64_t getTotalSendCount() const { return mTotalSendCount.getTotal(); }

    void incrementRecvMetrics(uint64_t bytesRead)
    {
        mTotalRecvBytes.increment(bytesRead);
        mTotalRecvCount.increment();
    }
    uint64_t getTotalRecvBytes() const { return mTotalRecvBytes.getTotal(); }
    uint64_t getTotalRecvCount() const { return mTotalRecvCount.getTotal(); }

    void incrementTotalQueuedOutputBytes(uint64_t bytesWritten) { mTotalQueuedOutputBytes.increment(bytesWritten); }
    void incrementTotalQueuedAsyncBytes(uint64_t bytesWritten) { mTotalQueuedAsyncBytes.increment(bytesWritten); }

    bool getAllowResumeConnection() const { return mConfig.getAllowResumeConnection(); }

    void addConnection() { mConnections.increment(); }
    void removeConnection() { mConnections.decrement(); }
    bool hasMaxConnections() const { return (mConnections.get() >= getMaxConnections()); }

    static void validateConfig(const EndpointConfig& config, const EndpointConfig* oldConfig, const char8_t* endpointGroup, const RateConfig& rateConfig, ConfigureValidationErrors& validationErrors);
    static void validateEndpointConfig(const EndpointConfig& config, const char8_t* endpointGroup, const RateConfig& rateConfig, ConfigureValidationErrors& validationErrors);
    
    template<typename EndpointConfigT>
    static void validateSslContext(const EndpointConfigT& config, const char8_t* endpointType, ConfigureValidationErrors& validationErrors);

    void reconfigure(const EndpointConfig::Endpoint& config, const RateConfig& newRateConfig, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists);

    bool assignPort(uint16_t port);

    static Endpoint* createFromConfig(const char8_t* name, const EndpointConfig::Endpoint& config, const RateConfig& rateConfig, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists);

    const char8_t* getCommonName() const;

    bool isSquelchingEnabled() const { return mSquelchingEnabled; }
    const EndpointConfig::OutputQueueConfig& getOutputQueueConfig() const { return mConfig.getQueuedOutputData(); }

    RateLimiter &getRateLimiter() { return mRateLimiter; }

    uint64_t getCurrentConnections() const { return mConnections.get(); }

    SslContextPtr getSslContext() const;

    bool isRpcAuthorized(ClientPlatformType platform, EA::TDF::ComponentId cmpId, CommandId cmdId) const { return mRpcAuthorizer.isRpcAuthorized(platform, cmpId, cmdId); }

    bool getInternalCommandsAllowed() const override;

    // Only enabling Envoy for gRPC endpoints at this time
    bool getEnvoyAccessEnabled() const override { return false; }

private:
    Channel* mChannel;
    Protocol::Type mProtocolType;
    Encoder::Type mEncoderType;
    Decoder::Type mDecoderType;
    InetAddress* mServerAddress;
    InetFilter* mTrustedNetworks;
    InetFilter* mInternalNicAccessNetworks;

    RateLimiter mRateLimiter;

    Metrics::MetricsCollection& mMetricsCollection;

    Metrics::Gauge mConnections; // Total number of active connections on this endpoint
    
    //The total number of connections rejected (at this endpoint) due to exceeding Rate Limit
    Metrics::Counter mCountTotalRejectsMaxRateLimit;
    
    //The total number of connections rejected (at this endpoint) due to exceeding maxConnection limit
    Metrics::Counter mCountTotalRejects;


    Metrics::Gauge mHighWatermarks;                // number of connections currently exceeding high watermark
    Metrics::Counter mTotalHighWatermarks;           // total number of times a high watermark has been exceeded
    Metrics::Counter mTotalExceededDataCloses;       // total number of connections we've disconnected due to having too much data queued
    Metrics::Counter mTotalResumedConnections;       // The total number of connections that have been resumed (at this endpoint)
    Metrics::Counter mTotalResumedConnectionsFailed; // The total number of connections that failed to be resumed (at this endpoint)
    
    Metrics::Counter mTotalExceededRpcFiberLimitDropped;
    Metrics::Counter mTotalExceededRpcFiberLimitPermitted;
    Metrics::Counter mTotalQueuedOutputBytes;
    Metrics::Counter mTotalQueuedAsyncBytes;
    Metrics::Counter mTotalRecvBytes;
    Metrics::Counter mTotalRecvCount;
    Metrics::Counter mTotalSendBytes;
    Metrics::Counter mTotalSendCount;

    char8_t* mEndpointName;
    EndpointConfig::Endpoint mConfig;
    bool mSquelchingEnabled;

    RpcAuthorizer mRpcAuthorizer;
private:

    Endpoint(const char8_t* endpointName);
    bool initialize(const EndpointConfig::Endpoint& config, const RateConfig& rateConfig, const EndpointConfig::CommandListByNameMap& rpcControlLists, const EndpointConfig::PlatformSpecificControlListsMap& platformSpecificControlLists);
    
    void configureQueuedOutputData(const EndpointConfig::OutputQueueConfig& config);

};

} // Blaze

#endif // BLAZE_ENDPOINT_H
