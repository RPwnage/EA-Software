// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#include <eadp/foundation/internal/JobBase.h>
#include <eadp/foundation/Callback.h>
#include <eadp/foundation/Error.h>
#include <eadp/foundation/Hub.h>
#include <eadp/foundation/ClientContext.h>
#include <eadp/foundation/StreamContext.h>
#include <eadp/foundation/internal/RpcResponse.h>
#include <eadp/foundation/internal/RpcResponseDelegator.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

class IRpcNetworkHandler;
class RpcStream;

/*!
 * @brief Job to send RPC request and delegate response to response delegator
 */
class EADPSDK_API RpcJobBase : public JobBase
{
public:
    virtual ~RpcJobBase() = default;
    void execute() override;
    void cleanup() override;

    // API to manage job timeout - FUTURE feature
    //void setTimeout(int16_t timeoutSeconds);
    //int16_t getTimeout();

    const SharedPtr<RpcStream>& getStream() { return m_stream; }


protected:
    // should not construct RpcBase directly, only used by its subclass
    explicit RpcJobBase(IHub* hub);

    RpcJobBase(const RpcJobBase&) = delete;
    RpcJobBase(RpcJobBase&&) = delete;

    RpcJobBase& operator =(const RpcJobBase&) = delete;
    RpcJobBase& operator =(RpcJobBase&&) = delete;

    // used by derived class to initialize the first RPC job and do precondition check
    ErrorPtr initialize(const SharedPtr<IRpcNetworkHandler>& network,
                        const ClientContext& clientContext,
                        const StreamContextPtr& streamContext,
                        const char8_t* relativeUrl,
                        bool isClientStreaming,
                        bool isServerStreaming,
                        UniquePtr<ProtobufMessage>&& request,
                        IRpcResponseDelegator* responseDelegator);

    // used by derived class to initialize the following RPC job and do precondition check
    ErrorPtr initialize(const StreamContextPtr& streamContext,
                        UniquePtr<ProtobufMessage>&& request);

    void sendRequest();
    void updateNetwork();
    void onError(ErrorPtr cause, const String& reason);

    IHub* getHub() const { return m_hub; }

    IHub* m_hub;
    SharedPtr<RpcStream> m_stream;
    UniquePtr<ProtobufMessage> m_request;
};

/*!
 * @brief Job for gRPC
 */
template <typename RequestType, typename ResponseType, bool IsClientStreamingBool, bool IsServerStreamingBool>
class RpcJob : public RpcJobBase, public IRpcResponseDelegator
{
public:
    using CallbackType = typename RpcResponse<RequestType, ResponseType>::CallbackType;

    explicit RpcJob(IHub* hub)
        : RpcJobBase(hub),
        m_callback(nullptr, Callback::PersistenceWeakPtr(), 0) // create a dummy empty callback object temporarily
    {
    }

    RpcJob(const RpcJob&) = delete;
    RpcJob(RpcJob&&) = delete;

    RpcJob& operator =(const RpcJob&) = delete;
    RpcJob& operator =(RpcJob&&) = delete;

    ErrorPtr initialize(const SharedPtr<IRpcNetworkHandler>& network,
                        const char8_t* relativeUrl,
                        UniquePtr<ProtobufMessage> request,
                        CallbackType callback,
                        const ClientContext& clientContext,
                        const StreamContextPtr& streamContext = nullptr)
    {
        m_callback = callback;
        ErrorPtr error = RpcJobBase::initialize(network,
                                                clientContext,
                                                streamContext,
                                                relativeUrl,
                                                IsClientStreamingBool,
                                                IsServerStreamingBool,
                                                EADPSDK_MOVE(request),
                                                this);
        if (error)
        {
            // trigger response callback to report initialization error
            respond(RawBuffer(), error);
        }
        return error;
    }

    Callback& getCallback() override
    {
        return m_callback;
    }

    void respond(RawBuffer data, const ErrorPtr& error) override
    {
        // enqueue response
        getHub()->addResponse(getHub()->getAllocator().template makeUnique<RpcResponse<RequestType, ResponseType>>(m_callback, EADPSDK_MOVE(data), error));
    }

protected:
    CallbackType m_callback;
};

/*!
 * @brief Job for unary gRPC
 */
template <typename RequestType, typename ResponseType>
using UnaryRpcJob = RpcJob<RequestType, ResponseType, false, false>;

/*!
 * @brief Job for client streaming gRPC
 */
template <typename RequestType, typename ResponseType>
using ClientStreamingRpcJob = RpcJob<RequestType, ResponseType, true, false>;

/*!
 * @brief Job for server streaming gRPC
 */
template <typename RequestType, typename ResponseType>
using ServerStreamingRpcJob = RpcJob<RequestType, ResponseType, false, true>;

/*!
 * @brief Job for bi-directional streaming gRPC
 */
template <typename RequestType, typename ResponseType>
using BidirectionalStreamingRpcJob = RpcJob<RequestType, ResponseType, true, true>;

/* @brief The StreamNextRequestJob is used to submit the following request for a streaming rpc.
 *
 * This class handles the scenarios in which the user may submit the next request
 * before stream Id is established.
 */
class EADPSDK_API StreamNextRequestJob : public RpcJobBase
{
public:
    explicit StreamNextRequestJob(IHub* hub) : RpcJobBase(hub) {}

    ErrorPtr initialize(const StreamContextPtr& streamContext,
                        UniquePtr<ProtobufMessage> request)
    {
        return RpcJobBase::initialize(streamContext, EADPSDK_MOVE(request));
    }

    void execute() override;
    void cleanup() override;

    EA_NON_COPYABLE(StreamNextRequestJob);
};

/* @brief The StreamFinishJob is for indicating the end of requests sending.
 *
 * This job should be added after all the request jobs, like StreamingRpcJob/StreamNextRequestJob.
 * Missing this finish signal sometimes will lead to a problem that server cannot process the requests and
 * respond correctly.
 * For convenience, you can use StreamContext.finish() to let SDK to create the job on your behalf.
 *
 * It will not close the stream and your RpcJob callback will still be triggered for response and error.
 *
 * This class handles the scenarios in which the user may submit the close request
 * before stream id is established. Note that this class is NOT derived from RpcJob.
 */
class EADPSDK_API StreamFinishJob : public JobBase
{
public:
    StreamFinishJob(IHub* hub, const SharedPtr<RpcStream>& stream);

    void execute() override

    EA_NON_COPYABLE(StreamFinishJob);

private:
    IHub* getHub() const { return m_hub; }

    IHub* m_hub;
    SharedPtr<RpcStream> m_stream;
};

}
}
}

