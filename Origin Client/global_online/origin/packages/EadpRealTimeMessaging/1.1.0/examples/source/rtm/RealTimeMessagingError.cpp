// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#include <eadp/realtimemessaging/RealTimeMessagingError.h>

using namespace eadp::foundation;
using namespace eadp::realtimemessaging;

const char8_t* RealTimeMessagingError::kDomain = "eadp::RealTimeMessagingError";

ErrorPtr RealTimeMessagingError::create(Allocator& allocator, Code code)
{
    return allocator.makeShared<Error>(allocator.make<String>(RealTimeMessagingError::kDomain),
                                       static_cast<uint32_t>(code),
                                       String(allocator.emptyString()),
                                       nullptr);
}

ErrorPtr RealTimeMessagingError::create(Allocator& allocator, String code, String reason)
{
    // Default server error code
    Code error_code = Code::SERVER_ERROR_UNRECOGNIZED;

    // Parse server error code
    if      (code == "VALIDATION_FAILED")         error_code = Code::SERVER_ERROR_VALIDATION_FAILED;
    else if (code == "AUTHORIZATION_ERROR")       error_code = Code::SERVER_ERROR_AUTHORIZATION_ERROR;
    else if (code == "NOT_AUTHENTICATED")         error_code = Code::SERVER_ERROR_NOT_AUTHENTICATED;
    else if (code == "INVALID_REQUEST")           error_code = Code::SERVER_ERROR_INVALID_REQUEST;
    else if (code == "USER_MUTED")                error_code = Code::SERVER_ERROR_USER_MUTED;
    else if (code == "UNDERAGE_BAN")              error_code = Code::SERVER_ERROR_UNDERAGE_BAN;
    else if (code == "USER_BANNED")               error_code = Code::SERVER_ERROR_USER_BANNED;
    else if (code == "SESSION_NOT_FOUND")         error_code = Code::SERVER_ERROR_SESSION_NOT_FOUND;
    else if (code == "SESSION_ALREADY_EXIST")     error_code = Code::SERVER_ERROR_SESSION_ALREADY_EXIST;
    else if (code == "RATE_LIMIT_EXCEEDED")       error_code = Code::SERVER_ERROR_RATE_LIMIT_EXCEEDED; 
    else if (code == "SESSION_LIMIT_REACHED")     error_code = Code::SERVER_ERROR_SESSION_LIMIT_REACHED;

    return allocator.makeShared<Error>(allocator.make<String>(RealTimeMessagingError::kDomain), 
                                       static_cast<uint32_t>(error_code),
                                       EADPSDK_MOVE(reason), 
                                       nullptr);
}

ErrorPtr RealTimeMessagingError::create(Allocator& allocator, Code code, String reason)
{
    return allocator.makeShared<Error>(allocator.make<String>(RealTimeMessagingError::kDomain),
                                       static_cast<uint32_t>(code),
                                       EADPSDK_MOVE(reason),
                                       nullptr);
}

ErrorPtr RealTimeMessagingError::create(Allocator& allocator, Code code, const char8_t* reason)
{
    return allocator.makeShared<Error>(allocator.make<String>(RealTimeMessagingError::kDomain),
                                       static_cast<uint32_t>(code),
                                       allocator.make<String>(reason),
                                       nullptr);
}

ErrorPtr RealTimeMessagingError::create(Allocator& allocator, Code code, String reason, ErrorPtr cause)
{
    return allocator.makeShared<Error>(allocator.make<String>(RealTimeMessagingError::kDomain),
                                       static_cast<uint32_t>(code),
                                       EADPSDK_MOVE(reason),
                                       EADPSDK_MOVE(cause));
}

ErrorPtr RealTimeMessagingError::create(Allocator& allocator, Code code, const char8_t* reason, ErrorPtr cause)
{
    return allocator.makeShared<Error>(allocator.make<String>(RealTimeMessagingError::kDomain),
                                       static_cast<uint32_t>(code),
                                       allocator.make<String>(reason),
                                       EADPSDK_MOVE(cause));
}
