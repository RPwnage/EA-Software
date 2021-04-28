// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#include <eadp/foundation/Callback.h>
#include <eadp/foundation/Hub.h>
#include <eadp/foundation/Error.h>
#include <eadp/foundation/LoggingService.h>
#include <eadp/foundation/Response.h>
#include <eadp/foundation/internal/Utils.h>
#include "Serialization.h"

namespace eadp
{
namespace foundation
{
namespace Internal
{

// doesn't fit ResponseT as invoke() is special
template<typename RequestType, typename ResponseType>
class RpcResponse : public IResponse
{
public:
    using CallbackType = RpcCallback<RequestType, ResponseType>;

    RpcResponse(CallbackType callback,
                RawBuffer responseData,
                ErrorPtr error)
        : m_callback(callback), m_data(EADPSDK_MOVE(responseData)), m_error(EADPSDK_MOVE(error))
    {
    }

    RpcResponse(const RpcResponse&) = delete;
    RpcResponse(RpcResponse&&) = default;
    RpcResponse& operator= (const RpcResponse&) = delete;
    RpcResponse& operator= (RpcResponse&&) = default;

    Callback& getCallback() override
    {
        return m_callback;
    }

    void invoke(IHub* hub) override
    {
        if (m_error || m_data.empty())
        {
            // error or empty response
            m_callback(UniquePtr<ResponseType>(nullptr), m_error);
            return;
        }

        UniquePtr<ResponseType> response = hub->getAllocator().makeUnique<ResponseType>(hub->getAllocator());
        Decoder decoder(hub->getAllocator());
        ErrorPtr decodeError = decoder.decode(m_data, response.get());
        if (decodeError)
        {
            // error
            response.reset();
            EADPSDK_LOGV(hub, "RpcResponse", "Failed to decode response data because of %s.", decodeError->toString().c_str());
        }

        m_callback(EADPSDK_MOVE(response), decodeError);
    }

    String toString(Allocator& allocator) override
    {
        ResponseType dummy(allocator);
        String description = allocator.make<String>();
        Internal::Utils::appendSprintf(description, "RpcResponse(%s)", dummy.getTypeName().c_str());
        return description;
    }

protected:
    CallbackType m_callback;
    RawBuffer m_data;
    ErrorPtr m_error;
};

}
}
}

