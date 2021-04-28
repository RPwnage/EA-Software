// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>
#include <eadp/foundation/Hub.h>
#include <eadp/foundation/internal/HttpRequest.h>
#include <eadp/foundation/internal/Serialization.h>
#include <eadp/foundation/internal/JobBase.h>

namespace eadp
{
namespace foundation
{
namespace Internal
{

struct EADPSDK_API HttpResponseHeaderInfo
{
    static const int32_t kInvalidStatusCode = -1;      // status code default value before getting Http response
    static const int32_t kInvalidContentLength = -1;   // content length default value if HTTP header doesn't contain content-length

    explicit HttpResponseHeaderInfo(Allocator& allocator);

    String url; //!< When redirection happens, this url could be different from HttpRequest.url
    int32_t statusCode;
    int32_t contentLength;
    Timestamp modifiedDate;

    // Since HTTP header field name is case-insensitive,
    // we standardize the header key to lower case for simplifying the search
    MultiMap<String, String> headers;
};

struct EADPSDK_API HttpResponse
{
    using Callback = CallbackT<void(UniquePtr<HttpResponse> response)>;

    HttpResponse(const SharedPtr<const HttpRequest>& intialRequest, const SharedPtr<const HttpResponseHeaderInfo>& info);

    EA_NON_COPYABLE(HttpResponse);

    SharedPtr<const HttpRequest> request; // const pointer
    SharedPtr<const HttpResponseHeaderInfo> responseInfo; // const pointer

    RawBuffer data; // this is raw data buffer, so its size could be larger than the received data size
    uint64_t totalDataReceived; // this indicates the actually response data size
    bool isDone;
    ErrorPtr error;
};

}
}
}

