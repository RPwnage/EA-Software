// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/Service.h>

namespace eadp
{
namespace foundation
{

// forward declaration
namespace Internal
{
class RpcStream;
class IRpcNetworkHandler;
}
class StreamContext;
using StreamContextPtr = SharedPtr<StreamContext>;

/*!
 * @brief Stream context helps to manage the RPC stream.
 *
 * Stream context can be used to send multiple requests on single RPC stream as client streaming RPC or
 * bi-directional streaming RPC. For multiple requests, it is required to call finish() to end request sending.
 * Also it can be used to manage the stream, for example, calling cancel() can terminate stream elegantly.
 * If stream management is not required and only single request need to be sent, skipping stream context creation
 * and passing nullptr to RPC call stream context parameter.
 */
class EADPSDK_API StreamContext
{
public:
    /*!
     * @brief Create a new stream context
     *
     * @return StreamContextPtr The pointer to stream context instance; null if service is not valid.
     */
    static StreamContextPtr create(SharedPtr<IService> service);

    /*!
     * @brief Constructor
     */
    explicit StreamContext(SharedPtr<IHub> hub);

    /*!
     * @brief Default destructor
     */
    ~StreamContext() = default;

    /*!
     * @brief Check whether the stream context is still valid.
     *
     * This API is most used internally to validate stream context for job management.
     * Stream context will be in invalid state after creation until the first Rpc call initializes it.
     * After that, the stream context will be invalid if
     * - Stream context is not initialized successfully by Rpc call (raise assertion)
     * - Server closed the steam (raise response callback with error)
     * - Network connection dropped (raise response callback with error)
     * - Callback persistent object is released (no callback will be triggered)
     * If stream context becomes invalid, all pending & new jobs schedule will be dropped without execution.
     *
     * @return True if stream context is valid; otherwise false.
     */
    bool isValid();

    /*!
     * @brief Indicates the end of stream request sending
     *
     * Except the null stream context case, RPC caller should always call this API to indicate the end
     * after sending all the requests. Not doing that could possibly lead to that server timeouts the stream
     * without responding the requests.
     */
    void finish();

    /*!
     * @brief Cancel the stream
     *
     * Use this method to notify the service you are no longer interested in Responses from a streaming RPC.
     * Releasing the persistence object associated with the RPC callback will accomplish the same effect.
     * This method is provided to support termination of individual streaming RPCs when a persistence object
     * is shared across multiple RPC callbacks
     */
    void cancel();

    /*!
     * @brief Get the internal stream
     *
     * @note For EADP SDK internal use only.
     */
    SharedPtr<::eadp::foundation::Internal::RpcStream>& getStream();

private:
    SharedPtr<IHub> m_hub;
    SharedPtr<::eadp::foundation::Internal::RpcStream> m_stream;
};


}
}

