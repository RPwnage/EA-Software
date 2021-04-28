/*************************************************************************************************/
/*!
\file inboundrpccomponentglue.h


\attention
(c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

// The purpose of this file is to isolate the common glue code between rpc calls and component layer.
// This allows to reuse the functionality from both legacy(Fire*/http etc.) and grpc endpoints.

#ifndef BLAZE_INBOUNDRPCCOMPONENTGLUE_H
#define BLAZE_INBOUNDRPCCOMPONENTGLUE_H

#include "framework/tdf/userdefines.h"
#include "framework/protocol/rpcprotocol.h"
#include "framework/metrics/metrics.h"
#include "framework/slivers/sliver.h"

namespace Blaze
{
class PeerInfo;
class Component;
struct CommandInfo;
class Message;
class RateLimiter;

namespace Grpc
{
class InboundGrpcRequestHandler;
}

struct RpcContext
{
    const CommandInfo& mCmdInfo;
    Message& mMessage;

    Sliver::AccessRef mSliverAccess;
    InstanceId mMovedTo;

    using WaitContext = const char8_t*;
    eastl::hash_map<WaitContext, eastl::pair<EA::TDF::TimeValue, int32_t> > mWaitTimes;
protected:
    RpcContext(const CommandInfo& cmdInfo, Message& message);
    virtual ~RpcContext();

    NON_COPYABLE(RpcContext);
};

namespace Metrics
{
    namespace Tag
    {
        extern Metrics::TagInfo<RpcContext::WaitContext>* wait_context;
    }
}

// We need separate context types for InboundRpcConnection and Grpc. The reason being that the request/response type
// for the same rpc may be different in the two. For example, 
// 1. If the request is empty for an rpc, Grpc still uses google::protobuf::Empty which has no equivalent tdf.
// 2. Grpc response would be the regular rpc response and a status field, resulting in a new type.
// Due to these reasons, it is not possible a class like UnaryRpcJob to create request/response object using tdfptr creation
// functions. 

struct InboundRpcConnectionRpcContext : public RpcContext
{
    friend class InboundRpcConnection;
    
    EA::TDF::TdfPtr mRequest;
    EA::TDF::TdfPtr mResponse;
    EA::TDF::TdfPtr mErrorResponse;

    RawBufferPtr mResponseBuf;

    InboundRpcConnectionRpcContext(const CommandInfo& cmdInfo, Message& message);
    ~InboundRpcConnectionRpcContext();

    NON_COPYABLE(InboundRpcConnectionRpcContext);
};

struct GrpcRpcContext : public RpcContext
{
    friend class Grpc::InboundGrpcRequestHandler;
    
    const EA::TDF::Tdf* mRequest;
    EA::TDF::Tdf* mResponse;
    EA::TDF::Tdf* mErrorResponse;

    GrpcRpcContext(const CommandInfo& cmdInfo, Message& message);
    ~GrpcRpcContext();

    NON_COPYABLE(GrpcRpcContext);
};

extern FiberLocalPtr<RpcContext> gRpcContext;

class InboundRpcComponentGlue
{
public:
    static BlazeRpcError initRpcContext(RpcContext& rpcContext, PeerInfo& peerInfo);
    static BlazeRpcError processRequest(Component& component, RpcContext& rpcContext, const CommandInfo& cmdInfo, Message& message,
        const EA::TDF::Tdf* request, EA::TDF::Tdf* response, EA::TDF::Tdf* errorResponse);
    static void setLoggingContext(SlaveSessionId slaveSessionId, UserSessionId userSessionId, const char8_t* deviceId, const char8_t* personaName, const InetAddress& ipAddress, AccountId nucleusAccountId, ClientPlatformType platform);
    static void updatePeerServiceName(PeerInfo& peerInfo, const char8_t* requestedServiceName);
    static Component* getComponent(EA::TDF::ComponentId componentId, Blaze::BindType endpointBindType);
    static FixedString32 getCurrentUserSessionDeviceId(UserSessionId userSessionId);
    static bool hasReachedFiberLimit(EA::TDF::ComponentId componentId, bool& isFrameworkCommand, FixedString32& droppedOrPermitted);
    static Fiber::CreateParams getCommandFiberParams(bool isLocal, const CommandInfo& cmdInfo, EA::TDF::TimeValue commandTimeout, bool blocksCaller, Fiber::FiberGroupId groupId);
    static bool hasExceededCommandRate(RateLimiter& rateLimiter, RatesPerIp* rateCounter, const CommandInfo& cmdInfo, bool checkOverall);
};
}
#endif // BLAZE_INBOUNDRPCCOMPONENTGLUE_H