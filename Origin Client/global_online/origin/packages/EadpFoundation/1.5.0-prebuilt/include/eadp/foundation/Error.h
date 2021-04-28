// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Config.h>

#include <eadp/foundation/Allocator.h>

namespace eadp
{
namespace foundation
{

class Error;
using ErrorPtr = SharedPtr<Error>;

/*!
 * @brief EADP SDK error
 *
 * The error class is used for reporting internal error to SDK user.
 */
class EADPSDK_API Error
{
public:
    /*!
     * Domain for EADP SDK foundation error, used with FoundationError::Code enum value as error code
     * [Deprecated] Use FoundationError::kDomain in NetworkError.h instead
     * 
     * @see FoundationError::Code
     */
    static const char8_t* kFoundationErrorDomain;
    /*!
     * Domain for gRPC status code error, used with gRPC Status code
     * [Deprecated] Use GrpcStatusError::kDomain in NetworkError.h instead
     *
     * @see https://github.com/grpc/grpc/blob/master/doc/statuscodes.md
     * @see https://github.com/grpc/grpc/blob/master/include/grpcpp/impl/codegen/status_code_enum.h
     */
    static const char8_t* kGRpcStatusCodeErrorDomain;
    /*!
     * Domain for DirtySDK socket error, used with DirtySDK SOCKERR_xxx error code
     * [Deprecated] Use SocketError::kDomain in NetworkError.h instead
     *
     * @see <DirtySDK/dirtysock/dirtynet.h>
     */
    static const char8_t* kDirtySdkSocketErrorDomain;
    /*!
     * Domain for DirtySDK HTTP error, used with HTTP status code
     * [Deprecated] Use HttpError::kDomain in NetworkError.h instead
     *
     * @see <DirtySDK/proto/protohttputil.h>
     * @see https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
     */
    static const char8_t* kDirtySdkHttpErrorDomain;
    /*!
     * Domain for DirtySDK SSL error, used with DirtySDK PROTOSSL_ERROR_xxx error code
     * [Deprecated] Use SslError::kDomain in NetworkError.h instead
     * 
     * @see <DirtySDK/proto/protossl.h>
     */
    static const char8_t* kDirtySdkSslErrorDomain;
    /*!
     * Domain for DirtySDK QoS error, used with DirtySDK QOSAPI_STATUS_xxx status code
     * [Deprecated] Doesn't support this erorr domain anymore as DirtySDK removed qosapi.h
     *
     * @see <DirtySDK/misc/qosapi.h>
     */
    static const char8_t* kDirtySdkQosErrorDomain;

    /*!
     * @brief Generic constructor
     *
     * The (@a domain, @a code) pair uniquely distinguish an error from others.
     *
     * @param domain The scope or field which error is coming from.
     * @param code The error code identify the error in the given error domain.
     * @param reason The descriptive string to help understanding the problem and possible solution,
     * but it is not necessary to be appropriate for end user, so don't expose it directly in application UI.
     */
    Error(String domain, uint32_t code, String reason)
        : m_domain(EADPSDK_MOVE(domain)), m_code(code), m_reason(EADPSDK_MOVE(reason))
    {
    }
    /*!
     * @brief Constructor with chained error
     *
     * The @a cause contains lower level error details, but maybe not be understandable or distinguishable
     * in some context, so the SDK will construct a high level error to help developer handle it.
     *
     * @param domain The scope or field which error is coming from.
     * @param code The error code identify the error in the given error domain.
     * @param reason The descriptive string to help understanding the problem and possible solution,
     * but it is not necessary to be appropriate for end user, so don't expose it directly in application UI.
     * @param cause The source error which causes the current error, it can be used to chain a series of
     * error object from high level to low level to describe the details of root cause of error.
     */
    Error(String domain, uint32_t code, String reason, ErrorPtr cause)
        : m_domain(EADPSDK_MOVE(domain)), m_code(code), m_reason(EADPSDK_MOVE(reason)), m_cause(EADPSDK_MOVE(cause))
    {
    }

    /*!
     * @brief Virtual default destructor
     */
    virtual ~Error() = default;

    /*!
     * @brief Error code get accessor
     */
    const String& getDomain() const { return m_domain; }

    /*!
     * @brief Error code get accessor
     *
     * Error code is defined as uint32_t type to accommodate arbitrary error code used in different sub-system.
     */
    uint32_t getCode() const { return m_code; }

    /*!
     * @brief Error reason get accessor
     *
     * Error reason is helpful information for SDK user, but it could be changed and shouldn't used in
     * code to determine the reason.
     */
    const String& getReason() const { return m_reason; }

    /*!
     * @brief Error cause get accessor
     *
     * Error cause is the lower level error which causes the current error. With echo error having another error
     * as cause, it builds up an error object cascade chain to contain all the details from high level to low leve.
     */
    ErrorPtr getCause() const { return m_cause; }

    /*!
     * @brief Error equality check
     *
     * A error is uniquely identified by the domain and the code pair. In the code, you should use this API to
     * determine if the error is specific error for error handling and/or code execution branch.
     */
    bool equals(const char8_t* domain, uint32_t code) const
    {
        return m_domain == domain && m_code == code;
    }

    template<typename CodeType>
    inline bool equals(const char8_t* domain, CodeType code) const
    {
        return equals(domain, static_cast<uint32_t>(code));
    }

    template<typename CodeType>
    inline bool equals(const String& domain, CodeType code) const
    {
        return equals(domain.c_str(), static_cast<uint32_t>(code));
    }

    /*!
     * @brief Convert error into descriptive string
     *
     * @param wholeChain The result string contains the whole error chain or not. If true, every cause
     * error chained will be appended to the descriptive string as well.
     */
    String toString(bool wholeChain = false) const;

private:
    String m_domain;
    uint32_t m_code;
    String m_reason;
    ErrorPtr m_cause;
};

/*!
 * @brief EADP SDK error helper for foundation module
 *
 * This class contains the generic error code could be reported from EADP SDK, and provide help API to simplify
 * the error creation.
 */
class EADPSDK_API FoundationError
{
public:
    /*!
     * Domain for EADP SDK foundation error.
     */
    static const char8_t* kDomain;

    /*!
     * EADP SDK foundation error code for generic error cases.
     */
    enum class Code : uint32_t
    {
        UNKNOWN = 0,                             //!< Default unknown error

        SYSTEM_UNEXPECTED = 100,                 //!< Unexpected error from OS system
        NOT_READY = 101,                         //!< SDK or component is not ready for use
        UNSUPPORTED = 102,                       //!< Unsupported usage
        NOT_AVAILABLE = 103,                     //!< Service is not available
        NOT_IMPLEMENTED = 104,                   //!< Function is not implemented yet
        INVALID_ARGUMENT = 105,                  //!< Argument out of range, does not evaluate, etc.
        MISSING_CALLBACK = 106,                  //!< Callback object/block is null
        NOT_AUTHENTICATED = 107,                 //!< No identity authenticated yet, cannot provide service
        NOT_INITIALIZED = 108,                   //!< Service requires explicit intialize() call but hasn't been initialized yet.
        SERVER_ERROR = 109,                      //!< Server responded with an error
        UNEXPECTED_RESPONSE = 110,               //!< Unexpected response from the server.
        TIMEOUT = 111,                           //!< Response not received within time limit.
        CANCELED = 112,                          //!< The request was canceled.
        INVALID_STATUS = 113,                    //!< Invalid status for the request.

        PROTOHTTP_INIT_FAIL = 1001,              //!< DirtySDK ProtoHttp initialization failed
        PROTOHTTP2_INIT_FAIL = 1002,             //!< DirtySDK ProtoHttp2 initialization failed
        PROTOHTTP_OTHER_ERROR = 1003,            //!< DirtySDK ProtoHttp error without details, check DirtySDK log output for details
        PROTOHTTP2_OTHER_ERROR = 1004,           //!< DirtySDK ProtoHttp2 error without details, check DirtySDK log output for details
        PROTOHTTP_ALREADY_CONNECTED = 1005,      //!< @deprecated
        PROTOHTTP_CONTENT_UNEXPECTED = 1006,     //!< @deprecated

        GRPC_STREAM_NOT_FOUND = 1100,            //!< Stream not found for specific stream id
        GRPC_STREAM_ALREADY_INIT = 1101,         //!< Reinitialize already initialized stream
        GRPC_REQUEST_FAILED = 1102,              //!< gPRC request failed
        GRPC_TOO_MANY_STREAM = 1103,             //!< Exceeded maximum number of stream allowed
        GRPC_INVALID_STREAM_ID = 1104,           //!< DirtySDK ProtoHttp2 returned invalid stream ID.
        GRPC_NOT_ENOUGH_BUFFER = 1105,           //!< Not enough buffer for request construction, check DirtySDK log for details
        GRPC_DUPLICATE_STREAM_ID = 1106,         //!< DirtySDK ProtoHttp2 issued a stream ID which already exists.
        GRPC_STREAM_ALREADY_FINISHED = 1107,     //!< Try to send data on already finished stream

        HTTP_CONTENT_UNEXPECTED = 1200,          //!< Got more or less response data than content-length specified
        HTTP_ALREADY_CONNECTED = 1201,           //!< try to connect REST network handler multiple times
        HTTP_NOT_ENOUGH_BUFFER = 1205,           //!< Not enough buffer for http request construction, check DirtySDK log for details
        HTTP_ALREADY_FINISHED = 1207,            //!< Try to send data on a http connection which already received response

        INVALID_RPC_JOB_SEQUENCE = 2001,         //!< Initial RPC job and following RPC job are not scheduled in right sequence.

        PERSISTENCE_INCOMPATIBLE_VERSION = 3001, //!< Incompatible persistence data version, cannot load.
        PERSISTENCE_DATA_READ = 3002,            //!< Failed to read necessary data from persistence storage.
        PERSISTENCE_DATA_WRITE = 3003,           //!< Failed to write necessary data into persistence storage.
        PERSISTENCE_OVERSIZE = 3004,             //!< Failed to load or save persistence data because the data size is over limit.
        PERSISTENCE_STORAGE_ACCESS = 3005,       //!< Failed to access persistence storage file

        IDENTITY_DEVICE_TOKEN_FAILURE = 4000,    //!< Failed to retrieve device token while authenticating
        IDENTITY_LOGIN_CANCELED = 4001,          //!< Login was canceled by user.
        IDENTITY_BROWSER_ERROR = 4002,           //!< Error reported from browser provider during UI login.
        IDENTITY_BROWSER_LOGIN_FAILURE = 4003,   //!< Unexpected response from browser provider during UI login.
    };

    /*!
     * @brief FoundationError creation static help function with error code
     */
    static ErrorPtr create(Allocator& allocator, Code code);

    /*!
     * @brief FoundationError creation static help function with error code and reason
     */
    static ErrorPtr create(Allocator& allocator, Code code, String reason);
    static ErrorPtr create(Allocator& allocator, Code code, const char8_t* reason);

    /*!
     * @brief FoundationError creation static help function with error code, reason and cause error
     */
    static ErrorPtr create(Allocator& allocator, Code code, String reason, ErrorPtr cause);
    static ErrorPtr create(Allocator& allocator, Code code, const char8_t* reason, ErrorPtr cause);

    /*!
     * @brief Deleted constructor to avoid construct FoundatiomError
     */
    FoundationError() = delete;

    /*!
     * @brief Deleted destructor
     */
    ~FoundationError() = delete;
};

}
}
