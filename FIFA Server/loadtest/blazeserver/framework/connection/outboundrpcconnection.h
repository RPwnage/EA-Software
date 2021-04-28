/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_OUTBOUNDRPCCONNECTION_H
#define BLAZE_OUTBOUNDRPCCONNECTION_H

/*** Include files *******************************************************************************/

#include "EASTL/hash_map.h"

#include "framework/connection/connection.h"
#include "framework/component/message.h"
#include "framework/component/rpcproxysender.h"
#include "framework/system/fibersemaphore.h"

#include "EATDF/time.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

namespace Blaze
{

class ConnectionOwner;
class SocketChannel;
class RawBuffer;
class Endpoint;
class TdfEncoder;
class TdfDecoder;
struct CommandMetrics;

class OutboundRpcConnection : public Connection, public RpcProxySender
{
    NON_COPYABLE(OutboundRpcConnection);
public:

    OutboundRpcConnection(ConnectionOwner& owner, ConnectionId ident,
        SocketChannel& channel, Blaze::Endpoint& endpoint);
    ~OutboundRpcConnection() override;

    // RpcProxySender interface
    BlazeRpcError sendRequestNonblocking(ComponentId component, CommandId command, const EA::TDF::Tdf* request, MsgNum& msgNum,
        int32_t logCategory = CONNECTION_LOGGER_CATEGORY, const RpcCallOptions &options = RpcCallOptions(), RawBuffer* responseRaw = nullptr) override;

    BlazeRpcError DEFINE_ASYNC_RET(sendRawRequest(ComponentId component, CommandId command, RawBufferPtr& request,
        RawBufferPtr& response, InstanceId& movedTo, int32_t logCategory = CONNECTION_LOGGER_CATEGORY, const RpcCallOptions &options = RpcCallOptions()) override);

    BlazeRpcError DEFINE_ASYNC_RET(waitForRequest(MsgNum msg, EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf, InstanceId& movedTo,
        int32_t logCategory = CONNECTION_LOGGER_CATEGORY, const RpcCallOptions &options = RpcCallOptions(), RawBuffer* responseRaw = nullptr) override);

    Protocol::Type getProtocolType() const override { return getProtocolBase().getType(); }

    void setInstanceId(InstanceId id) { mInstanceId = id; }
    InstanceId getInstanceId() const { return mInstanceId; }

    void close() override;

    bool hasOutstandingCommands() const override { return !mSentRequests.empty(); }

    void setContextOverride(uint64_t context) { mContextOverride = context; }

protected:
    BlazeRpcError sendRequestInternal(RpcProtocol::Frame& outFrame, RawBufferPtr& requestBuf, const RpcCallOptions &options);
    BlazeRpcError DEFINE_ASYNC_RET(waitForRequestInternal(MsgNum msg, ComponentId& component, CommandId& command, RawBufferPtr& responseBuf, InstanceId& movedTo, const RpcCallOptions &options));

    void processMessage(RawBufferPtr& message) override;

    RpcProtocol &getProtocol() { return (RpcProtocol &)getProtocolBase(); }
    uint64_t getContext() const;
    bool isKeepAliveRequired() const override { return true; }
private:
    struct OutgoingRequest
    {
        OutgoingRequest() : complete(false), component(Component::INVALID_COMPONENT_ID), 
            command(Component::INVALID_COMMAND_ID), respErrorCode(ERR_OK) {}

        bool complete;
        bool ignoreReply;
        ComponentId component;
        CommandId command;        
        Fiber::EventHandle eventHandle;
        RawBufferPtr responseBuf;
        BlazeRpcError respErrorCode;
        InstanceId movedTo;
    };

    typedef eastl::hash_map<uint32_t, OutgoingRequest> RequestsBySeqno;

    InstanceId mInstanceId;

    void timeoutSendRequest(SemaphoreId semId);

    // Map of requests currently being processed for this connection
    RequestsBySeqno mSentRequests;

    uint64_t mContextOverride;
};

} // Blaze

#endif // BLAZE_CONNECTION_H

