/*************************************************************************************************/
/*!
    \file   outboundgrpcconnection.h

    \attention
        (c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

#ifndef BLAZE_OUTBOUNDGRPCCONNECTION_H
#define BLAZE_OUTBOUNDGRPCCONNECTION_H

/*** Include files *******************************************************************************/
#include "framework/connection/sslcontext.h"
#include "framework/grpc/grpcutils.h"
#include "framework/tdf/frameworkconfigdefinitions_server.h"
#include "framework/controller/controller.h"

#include <EASTL/hash_map.h>
#include <EASTL/queue.h>

namespace Blaze
{

namespace Grpc
{

extern const char8_t* SERVICE_MODULE_DELIMITER;

class OutboundRpcBase;
typedef eastl::intrusive_ptr<OutboundRpcBase> OutboundRpcBasePtr;

class OutboundGrpcChannel
{
public:
    OutboundGrpcChannel(const char8_t* serviceName, const GrpcServiceConnectionInfo& connInfo)
        : mServiceName(serviceName)
        , mConnInfo(connInfo)
    {
        if (mConnInfo.getSslCert()[0] != '\0' && mConnInfo.getSslKey()[0] != '\0')
        {
            SslContextPtr sslContext = gSslContextManager->get(mConnInfo.getSslContext());
            grpc::SslCredentialsOptions opts;
            if (mConnInfo.getPathType() == GrpcServiceConnectionInfo::VAULT)
            {
                opts.pem_cert_chain = mConnInfo.getSslCert();
                opts.pem_private_key = mConnInfo.getSslKey();
            }
            else // LOCAL_DISK
            {
                Blaze::Grpc::readFile(mConnInfo.getSslCert(), opts.pem_cert_chain);
                Blaze::Grpc::readFile(mConnInfo.getSslKey(), opts.pem_private_key);
            }
            Blaze::Grpc::readFile(sslContext->getCaFile(), opts.pem_root_certs);

            mCredentials = grpc::SslCredentials(opts);
        }
        else
            mCredentials = grpc::InsecureChannelCredentials();

        grpc::ChannelArguments channelArgs;
        channelArgs.SetInt(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, mConnInfo.getKeepAlive());
        channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIME_MS, static_cast<int32_t>(mConnInfo.getKeepAliveSendInterval().getMillis()));
        channelArgs.SetInt(GRPC_ARG_KEEPALIVE_TIMEOUT_MS, static_cast<int32_t>(mConnInfo.getKeepAliveTimeout().getMillis()));
        channelArgs.SetString(GRPC_ARG_LB_POLICY_NAME, mConnInfo.getLbPolicyName());
        channelArgs.SetString(GRPC_ARG_PRIMARY_USER_AGENT_STRING, gController->getGrpcUserAgent());

        mChannel = grpc::CreateCustomChannel(mConnInfo.getServiceAddress(), mCredentials, channelArgs);
    }

    inline const char8_t* getServiceName() const { return mServiceName.c_str(); }

    const std::shared_ptr<grpc::Channel>& getChannel() const { return mChannel; }

protected:
    eastl::string mServiceName;
    const GrpcServiceConnectionInfo& mConnInfo;

private:
    std::shared_ptr<grpc::Channel> mChannel;
    std::shared_ptr<grpc::ChannelCredentials> mCredentials;
};

template <typename Module>
class OutboundGrpcConnection;

class OutboundGrpcConnectionBase
{
public:
    OutboundGrpcConnectionBase(const char8_t* serviceName)
        : mServiceName(serviceName)
        , mOutboundGrpcMetrics(serviceName)
        , mShutdownEventHandle(Fiber::INVALID_EVENT_ID)
        , mNextRpcId(0)
        , mRpcShutdownPending(false)
        , mCleanupRequested(false)
    {
        eastl::vector<eastl::string> serviceNameParts;
        blaze_stltokenize(mServiceName, SERVICE_MODULE_DELIMITER, serviceNameParts, true);
        mServiceNameShortVersion = serviceNameParts.front();
    }
    
    virtual ~OutboundGrpcConnectionBase()
    {
        BLAZE_ASSERT_COND_LOG(Log::GRPC, mOutboundRpcs.empty(), "No outbound RPC should remain at connection deletion.");
        BLAZE_ASSERT_COND_LOG(Log::GRPC, mPendingRemovalQueue.empty(), "No outbound RPC should be queued for deletion at connection deletion.");
    }

    void shutdown();
    void requestCleanup();

    inline const char8_t* getServiceName() const { return mServiceName.c_str(); }
    inline const char8_t* getServiceNameShortVersion() const { return mServiceNameShortVersion.c_str(); }

    void attachRpc(OutboundRpcBasePtr rpc);
    void detachRpc(uint64_t rpcId);

    void getStatusInfo(OutboundGrpcServiceStatusMap& infoMap) const;
    uint64_t getNextRpcId() { return mNextRpcId++; }

    void incRpcCount();
    void decRpcCount();
    void incRequestsSent();
    void incFailedRequests();
    void incResponsesReceived();
    void incFailedResponses();

    template <typename Module>
    OutboundGrpcConnection<Module>* getAsService() { return static_cast<OutboundGrpcConnection<Module>*>(this); }

protected:
    eastl::string mServiceName;
    eastl::string mServiceNameShortVersion;

    struct OutboundGrpcMetrics
    {
        OutboundGrpcMetrics(const char8_t* serviceName)
            : mMetricsCollection(Metrics::gFrameworkCollection->getCollection(Metrics::Tag::outboundgrpcservice, serviceName)),
            mNumRpcs(mMetricsCollection, "outboundgrpc.numrpcs"),
            mNumRequestsSent(mMetricsCollection, "outboundgrpc.numrequestssent"),
            mNumRequestsFailed(mMetricsCollection, "outboundgrpc.numrequestsfailed"),
            mNumResponsesReceived(mMetricsCollection, "outboundgrpc.numresponsesreceived"),
            mNumResponsesFailed(mMetricsCollection, "outboundgrpc.numresponsesfailed"),
            mFailureStreak(0)
        {}

        Metrics::MetricsCollection& mMetricsCollection;

        Metrics::Gauge mNumRpcs;
        Metrics::Counter mNumRequestsSent;
        Metrics::Counter mNumRequestsFailed;
        Metrics::Counter mNumResponsesReceived;
        Metrics::Counter mNumResponsesFailed;
        int32_t mFailureStreak;
    };

    OutboundGrpcMetrics mOutboundGrpcMetrics;

private:
    void detachRpcInternal();

private:
    // Maintain a reference to the intrusive pointer. This reference will only be removed when the RPC tells us it's done
    eastl::hash_map<uint64_t, OutboundRpcBasePtr> mOutboundRpcs;
    eastl::queue<uint64_t> mPendingRemovalQueue;

    Fiber::EventHandle mShutdownEventHandle;

    uint64_t mNextRpcId; // Key for the outbound RPC map. Simplifies cleanup of finished RPCs
    bool mRpcShutdownPending;
    bool mCleanupRequested;
};

template <typename Module>
class OutboundGrpcConnection : public OutboundGrpcConnectionBase
{
    typedef typename Module::Stub StubType;

public:
    OutboundGrpcConnection(const char8_t* serviceName, OutboundGrpcChannel& channel)
        : OutboundGrpcConnectionBase(serviceName)
    {
        mStub = Module::NewStub(channel.getChannel());
    }

    StubType* getStub() { return mStub.get(); }

    ~OutboundGrpcConnection()
    {
    }

private:
    std::unique_ptr<StubType> mStub;
};

} // namespace Grpc
} // namespace Blaze

#endif // BLAZE_OUTBOUNDGRPCCONNECTION_H
