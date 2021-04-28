/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_RPCPROXYSENDER_H
#define BLAZE_RPCPROXYSENDER_H

/*** Include files *******************************************************************************/

#include "framework/logger.h"
#include "framework/component/message.h"
#include "framework/protocol/protocol.h"

#include "EATDF/time.h"
#include "EATDF/tdfobjectid.h"

/*** Defines/Macros/Constants/Typedefs ***********************************************************/

#ifndef CONNECTION_LOGGER_CATEGORY
    #ifdef LOGGER_CATEGORY
        #define CONNECTION_LOGGER_CATEGORY LOGGER_CATEGORY
    #else
        #define CONNECTION_LOGGER_CATEGORY Log::CONNECTION
    #endif
#endif

namespace Blaze
{

struct RoutingOptions
{
    enum Type
    {
        INVALID,
        INSTANCE_ID,
        SLIVER_IDENTITY
    };

    inline RoutingOptions() : type(INVALID) {}
    inline RoutingOptions(const RoutingOptions& opts) : type(opts.type), id(opts.id) {}
    inline explicit RoutingOptions(const InstanceId& instanceId) : type(INSTANCE_ID) { id.instanceId = instanceId; }
    inline explicit RoutingOptions(const SliverIdentity& sliverIdentity) : type(SLIVER_IDENTITY) { id.sliverIdentity = sliverIdentity; }
    inline explicit RoutingOptions(SliverNamespace sliverNamespace, SliverId sliverId) : type(SLIVER_IDENTITY) { id.sliverIdentity = MakeSliverIdentity(sliverNamespace, sliverId); }

    inline bool isValid() const { return (type != INVALID); }
    inline Type getType() const { return type; }

    inline InstanceId getAsInstanceId() const { return (type == INSTANCE_ID ? id.instanceId : INVALID_INSTANCE_ID); }
    inline SliverIdentity getAsSliverIdentity() const { return (type == SLIVER_IDENTITY ? id.sliverIdentity : INVALID_SLIVER_IDENTITY); }

    inline void setInstanceIdFromKey(uint64_t key) { setInstanceId(GetInstanceIdFromInstanceKey64(key)); }
    inline void setInstanceId(InstanceId instanceId)
    {
        type = INVALID;
        if (instanceId != INVALID_INSTANCE_ID)
        {
            type = INSTANCE_ID;
            id.instanceId = instanceId;
        }
    }

    inline void setSliverNamespace(SliverNamespace sliverNamespace) { setSliverIdentity(MakeSliverIdentity(sliverNamespace, INVALID_SLIVER_ID)); }
    inline void setSliverIdentityFromSliverId(SliverNamespace sliverNamespace, SliverId sliverId) { setSliverIdentity(MakeSliverIdentity(sliverNamespace, sliverId)); }
    inline void setSliverIdentityFromKey(SliverNamespace sliverNamespace, SliverKey key) { setSliverIdentity(MakeSliverIdentity(sliverNamespace, GetSliverIdFromSliverKey(key))); }
    inline void setSliverIdentity(SliverIdentity sliverIdentity)
    {
        type = INVALID;
        if (sliverIdentity != INVALID_SLIVER_IDENTITY)
        {
            type = SLIVER_IDENTITY;
            id.sliverIdentity = sliverIdentity;
        }
    }

    static inline RoutingOptions fromInstanceId(InstanceId instanceId)
    {
        RoutingOptions opts;
        opts.setInstanceId(instanceId);
        return opts;
    }

    static inline RoutingOptions fromSliverIdentity(SliverIdentity sliverIdentity)
    {
        RoutingOptions opts;
        opts.setSliverIdentity(sliverIdentity);
        return opts;
    }

private:
    Type type;

    union Id
    {
        InstanceId instanceId;
        SliverIdentity sliverIdentity;
    } id;
};

struct RpcCallOptions
{
    RpcCallOptions() :
      timeoutOverride(0),
      ignoreReply(false),
      followMovedTo(true)
      {}

    RoutingOptions routeTo;

    EA::TDF::TimeValue timeoutOverride;
    bool ignoreReply;
    bool followMovedTo;
};

typedef uint32_t MsgNum;
class InboundRpcConnection;
struct CommandMetrics;

class RpcProxySender
{
public:
    virtual ~RpcProxySender() { }

    virtual BlazeRpcError DEFINE_ASYNC_RET(sendRequest(EA::TDF::ComponentId component, CommandId command, const EA::TDF::Tdf* request,
        EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf, InstanceId& movedTo,
        int32_t logCategory = CONNECTION_LOGGER_CATEGORY, const RpcCallOptions &options = RpcCallOptions(), RawBuffer* responseRaw = nullptr))
    {
        MsgNum msgNum = 0; 
        BlazeRpcError err = sendRequestNonblocking(component, command, request, msgNum, logCategory, options, responseRaw);
        if (err == ERR_OK)
        {
            err = waitForRequest(msgNum, responseTdf, errorTdf, movedTo, logCategory, options, responseRaw);
        }

        return err;
    }

    virtual BlazeRpcError DEFINE_ASYNC_RET(sendRawRequest(EA::TDF::ComponentId component, CommandId command, RawBufferPtr& request,
        RawBufferPtr& response, InstanceId& movedTo, int32_t logCategory = CONNECTION_LOGGER_CATEGORY, const RpcCallOptions &options = RpcCallOptions())) { return ERR_SYSTEM; }

    virtual BlazeRpcError sendRequestNonblocking(EA::TDF::ComponentId component, CommandId command, const EA::TDF::Tdf* request,
        MsgNum& msgNum, int32_t logCategory = CONNECTION_LOGGER_CATEGORY, const RpcCallOptions &options = RpcCallOptions(), RawBuffer* responseRaw = nullptr) { return ERR_SYSTEM; }

    virtual BlazeRpcError DEFINE_ASYNC_RET(waitForRequest(MsgNum msgNum,  EA::TDF::Tdf *responseTdf, EA::TDF::Tdf *errorTdf, InstanceId& movedTo,
        int32_t logCategory = CONNECTION_LOGGER_CATEGORY, const RpcCallOptions &options = RpcCallOptions(), RawBuffer* responseRaw = nullptr)) { return ERR_SYSTEM; }


    virtual BlazeRpcError sendPassthroughRequest(const Message &msg, uint32_t& msgNum,
        CommandMetrics *commandMetricsPtr, int32_t logCategory) { return ERR_SYSTEM; }

    virtual Protocol::Type getProtocolType() const { return Protocol::INVALID; }

};

} // namespace Blaze

#endif // BLAZE_RPCPROXYSENDER_H

