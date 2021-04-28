// Copyright(C) 2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Error.h>

namespace eadp
{
namespace realtimemessaging
{
/*!
 * Error helping class for EADP RealTimeMessaging
 */
class RealTimeMessagingError
{
public:
    /*!
     * @brief The error domain for EADP SDK RealTimeMessaging error
     *
     * The domain should always be used in conjuction with RealTimeMessagingError::Code error code enum to indicate an error.
     *
     * @see RealTimeMessagingError::Code
     */
    static const char8_t* kDomain;

    /*!
     * @brief EADP SDK RealTimeMessaging error code
     *
     * The error code should always be used in conjunction with RealTimeMessagingError::kDomain.
     *
     * @note Please use enum literals, avoid using uint32_t value directly as it could change.
     */
    enum class Code : uint32_t
    {
        PROTO_SOCKET_ERROR                  = 30000,  //!< ProtoWebSocket Error
        SOCKET_REQUEST_TIMEOUT              = 30001,  //!< Socket Request timeout
        INVALID_SERVER_RESPONSE             = 30002,  //!< Invalid Response from Server
        CONNECTION_SERVICE_NOT_CONNECTED    = 30003,  //!< Connection Service not connected
        HEARTBEAT_TIMEOUT                   = 30004,  //!< Heartbeat timeout
        CONNECTION_SHUTDOWN                 = 30005,  //!< The Connection Service has shutdown 
        RATE_LIMIT_EXCEEDED                 = 30006,  //!< Typing Indicator request was sent too quickly (Client Side)

        // Server Error Codes
        SERVER_ERROR_UNRECOGNIZED           = 30100,  //!< Unrecognized server error.
        SERVER_ERROR_VALIDATION_FAILED      = 30101,  //!< Invalid parameter(s).
        SERVER_ERROR_AUTHORIZATION_ERROR    = 30102,  //!< Invalid access token.
        SERVER_ERROR_NOT_AUTHENTICATED      = 30103,  //!< The user is not authenticated.
        SERVER_ERROR_INVALID_REQUEST        = 30104,  //!< The request is invalid.
        SERVER_ERROR_USER_MUTED             = 30105,  //!< The user is muted.
        SERVER_ERROR_UNDERAGE_BAN           = 30106,  //!< The user is banned because they are underage.
        SERVER_ERROR_USER_BANNED            = 30107,  //!< The user is banned.
        SERVER_ERROR_SESSION_NOT_FOUND      = 30108,  //!< The session could not be found. Reconnect failure. 
        SERVER_ERROR_SESSION_ALREADY_EXIST  = 30109,  //!< The session already exists on another client.
        SERVER_ERROR_RATE_LIMIT_EXCEEDED    = 30110,  //!< The typing indicator update rate limit has been exceeded (Server Side). 
        SERVER_ERROR_SESSION_LIMIT_EXCEEDED = 30111,  //!< The session limit has been reached in a multiple sessions connection attempt.
    };

    /*!
     * @brief RealTimeMessagingError creation static help function with error code
     */
    static eadp::foundation::ErrorPtr create(eadp::foundation::Allocator& allocator, Code code);

    /*!
     * @brief RealTimeMessagingError creation helper with server string error code and reason
     */
    static eadp::foundation::ErrorPtr create(eadp::foundation::Allocator& allocator, eadp::foundation::String code, eadp::foundation::String reason);

    /*!@{
     * @brief RealTimeMessagingError creation helper with error code and reason
     */
    static eadp::foundation::ErrorPtr create(eadp::foundation::Allocator& allocator, Code code, eadp::foundation::String reason);
    static eadp::foundation::ErrorPtr create(eadp::foundation::Allocator& allocator, Code code, const char8_t* reason);
    /*!@}*/

    /*!@{
     * @brief RealTimeMessagingError creation helper with error code, reason, and cause error
     */
    static eadp::foundation::ErrorPtr create(eadp::foundation::Allocator& allocator, Code code, eadp::foundation::String reason, eadp::foundation::ErrorPtr cause);
    static eadp::foundation::ErrorPtr create(eadp::foundation::Allocator& allocator, Code code, const char8_t* reason, eadp::foundation::ErrorPtr cause);
    /*!@}*/

    /*!
     * @brief Deleted constructor to avoid direct RealTimeMessagingError construction
     */
    RealTimeMessagingError() = delete;

    /*!
     * @brief Deleted destructor
     */
    ~RealTimeMessagingError() = delete;
};

}
}

