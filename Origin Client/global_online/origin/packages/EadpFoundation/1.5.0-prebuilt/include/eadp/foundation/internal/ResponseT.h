// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#if EADPSDK_USE_STD_STL
#include <tuple>
#else
#include <EASTL/tuple.h>
#endif

#include <eadp/foundation/Response.h>
#include <eadp/foundation/Hub.h>
#include <EAAssert/eaassert.h>

namespace eadp
{
namespace foundation
{

class INamedResponse : public IResponse
{
public:
    INamedResponse(String&& name)
        : m_name(EADPSDK_MOVE(name))
    {
    }
    
    String toString(Allocator& allocator) override
    {
        return m_name;
    }
protected:
    String m_name;
};

template<typename... Arguments> 
class ResponseT : public INamedResponse
{
public:
    using CallbackType = CallbackT<void(Arguments...)>;
    
    static inline void post(IHub* hub, const char* name, CallbackType callback, Arguments... args)
    {
        EA_ASSERT_MSG(name, "The name of response class cannot be null.");
        Allocator& allocator = hub->getAllocator();
        hub->addResponse(allocator.makeUnique<ResponseT<Arguments...>>(allocator.make<String>(name),
                                                                       EADPSDK_MOVE(callback),
                                                                       EADPSDK_FORWARD<Arguments>(args)...));
    }

    ResponseT(Allocator& allocator, const char* name, const CallbackType& callback, Arguments... args)
        : INamedResponse(allocator.make<String>(name))
        , m_callback(EADPSDK_MOVE(callback))
        , m_arguments(EADPSDK_FORWARD<Arguments>(args)...)
    {
    }

    // this version is optimized for post()
    ResponseT(String&& name, CallbackType&& callback, Arguments&&... args)
        : INamedResponse(EADPSDK_MOVE(name))
        , m_callback(EADPSDK_MOVE(callback))
        , m_arguments(EADPSDK_FORWARD<Arguments>(args)...)
    {
    }
    
    Callback& getCallback() override
    {
        return m_callback;
    }
    
    void invoke(IHub* hub) override
    {
        invokeCallback(EADPSDK_INDEX_SEQUENCE_FOR<Arguments...>());
    }
private:
#if EADPSDK_USE_STD_STL
    using ArgumentTuple = std::tuple<std::remove_reference_t<Arguments>...>;
#else
    using ArgumentTuple = eastl::tuple<eastl::remove_reference_t<Arguments>...>;
#endif

    // unpacking the variadic argument from tuple to call a callback
    // https://stackoverflow.com/questions/687490/how-do-i-expand-a-tuple-into-variadic-template-functions-arguments/19060157#19060157
    // https://stackoverflow.com/questions/27941661/generating-one-class-member-per-variadic-template-argument
#if EADPSDK_USE_STD_STL
    template<size_t... Indexes>
    void invokeCallback(std::index_sequence<Indexes...>)
    {
        m_callback(std::move(std::get<Indexes>(m_arguments))...);
    }
#else
    template<size_t... Indexes>
    void invokeCallback(eastl::index_sequence<Indexes...>)
    {
        m_callback(eastl::move(eastl::get<Indexes>(m_arguments))...);
    }
#endif
    
    CallbackType m_callback;
    ArgumentTuple m_arguments;
};

using ErrorResponseT = ResponseT<const ErrorPtr&>;

template<typename ResponseDataType>
using UniquePtrResponseT = ResponseT<UniquePtr<ResponseDataType>, const ErrorPtr&>;

// adapter for back-compatiblity, should NOT be used in latest code
class ErrorResponse : public ErrorResponseT
{
public:
    ErrorResponse(ErrorResponseT::CallbackType callback, const ErrorPtr& error, String name)
        : ErrorResponseT(EADPSDK_MOVE(name), EADPSDK_MOVE(callback), error)
    {
    }
};

template<typename ResponseDataType>
class SimpleResponseT : public ResponseT<ResponseDataType, const ErrorPtr&>
{
public:
    using ParentType = ResponseT<ResponseDataType, const ErrorPtr&>;
    SimpleResponseT(typename ParentType::CallbackType callback, ResponseDataType data, const ErrorPtr& error, String name)
    : ParentType(EADPSDK_MOVE(name), EADPSDK_MOVE(callback), EADPSDK_MOVE(data), error)
    {
    }
};

// end of adapter
}
}
