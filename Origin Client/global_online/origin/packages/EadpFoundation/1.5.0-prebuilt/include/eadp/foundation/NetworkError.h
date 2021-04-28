// Copyright(C) 2016-2018 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Error.h>

namespace eadp
{
namespace foundation
{

/*!
 * @brief EADP SDK gRPC status error
 * 
 * This class is mainly for hosting error code in specific domain for documentation purpose, so no create() method provided.
 */
class GrpcStatusError
{
public:
    /*!
     * Domain for EADP SDK gRPC status error.
     */
    static const char8_t* kDomain;

    /*!
     * Error code for EADP SDK gRPC status error.
     * 
     * The error code value is defined by gRPC protocol. The enum is exposed for convenience of EADP SDK user.
     * 
     * @see https://github.com/grpc/grpc/blob/master/doc/statuscodes.md
     * @see https://github.com/grpc/grpc/blob/master/include/grpcpp/impl/codegen/status_code_enum.h
     */
    enum class Code : uint32_t
    {
        OK                  = 0, //!< Not an error; returned on success.
        CANCELED            = 1, //!< The operation was cancelled (typically by the caller).
        UNKNOWN             = 2, //!< Unknown error.
        INVALID_ARGUMENT    = 3, //!< Client specified an invalid argument.
        DEADLINE_EXCEEDED   = 4, //!< Deadline expired before operation could complete.
        NOT_FOUND           = 5, //!< Some requested entity (e.g., file or directory) was not found.
        ALREADY_EXISTS      = 6, //!< Some entity that we attempted to create (e.g., file or directory) already exists
        PERMISSION_DENIED   = 7, //!< The caller does not have permission to execute the specified operation.
        RESOURCE_EXHAUSTED  = 8, //!< Some resource has been exhausted, perhaps a per-user quota, or perhaps the entire file system is out of space.
        FAILED_PRECONDITION = 9, //!< Operation was rejected because the system is not in a state required for he operation's execution.
        ABORTED            = 10, //!< The operation was aborted, typically due to a concurrency issue like sequencer check failures, transaction aborts, etc.
        OUT_OF_RANGE       = 11, //!< Operation was attempted past the valid range.
        UNIMPLEMENTED      = 12, //!< Operation is not implemented or not supported/enabled in this service.
        INTERNAL           = 13, //!< Internal errors.
        UNAVAILABLE        = 14, //!< The service is currently unavailable.
        DATA_LOSS          = 15, //!< Unrecoverable data loss or corruption.
        UNAUTHENTICATED    = 16, //!< The request does not have valid authentication credentials for the operation.
        DO_NOT_USE = 0xFFFFFFFF  //!< Force users to include a default branch
    };

    /*!
     * @brief Deleted constructor to prevent accidental creation.
     */
    GrpcStatusError() = delete;

    /*!
     * @brief Deleted destructor.
     */
    ~GrpcStatusError() = delete;
};

/*!
 * @brief EADP SDK socket error
 * 
 * This class is mainly for hosting error code in specific domain for documentation purpose, so no create() method provided.
 */
class SocketError
{
public:
    /*!
     * Domain for EADP SDK socket error.
     */
    static const char8_t* kDomain;

    /*!
     * Error code for EADP SDK socket error.
     */
    enum class Code : uint32_t
    {
        ERROR_NONE      =        0x0, //!< no error
        ERROR_CLOSED    = 0xFFFFFFFF, //!< the socket is closed
        ERROR_NOTCONN   = 0xFFFFFFFE, //!< the socket is not connected
        ERROR_BLOCKED   = 0xFFFFFFFD, //!< operation would result in blocking
        ERROR_ADDRESS   = 0xFFFFFFFC, //!< the address is invalid
        ERROR_UNREACH   = 0xFFFFFFFB, //!< network cannot be accessed by this host
        ERROR_REFUSED   = 0xFFFFFFFA, //!< connection refused by the recipient
        ERROR_OTHER     = 0xFFFFFFF9, //!< unclassified error
        ERROR_NOMEM     = 0xFFFFFFF8, //!< out of memory
        ERROR_NORSRC    = 0xFFFFFFF7, //!< out of resources
        ERROR_UNSUPPORT = 0xFFFFFFF6, //!< unsupported operation
        ERROR_INVALID   = 0xFFFFFFF5, //!< resource or operation is invalid
        ERROR_ADDRINUSE = 0xFFFFFFF4, //!< address already in use
        ERROR_CONNRESET = 0xFFFFFFF3, //!< connection has been reset
        ERROR_BADPIPE   = 0xFFFFFFF2  //!< EBADF or EPIPE
    };

    /*!
     * @brief Deleted constructor to prevent accidental creation.
     */
    SocketError() = delete;

    /*!
     * @brief Deleted destructor.
     */
    ~SocketError() = delete;
};

/*!
 * @brief EADP SDK http Error.
 * 
 * This class is mainly for hosting error code in specific domain for documentation purpose, so no create() method provided.
 */
class HttpError
{
public:
    /*!
     * Domain for EADP SDK http error.
     */
    static const char8_t* kDomain;

    /*!
     * Error code for EADP SDK http error.
     * 
     * The error code value is defined by HTTP protocol. The enum is exposed for convenience of EADP SDK user.
     * The enum definition is from low level network implementation EADP SDK depends on, including all HTTP status code it supports -- including those are not error -- and other error code it uses.
     * 
     * @see https://en.wikipedia.org/wiki/List_of_HTTP_status_codes
     */
    enum class Code : uint32_t
    {
        // 1xx - informational reponse
        INFORMATIONAL       = 100, //!< generic name for a 1xx class response
        CONTINUE            = 100, //!< continue with request, generally ignored
        SWITCHPROTO         = 101, //!< 101 - OK response to client switch protocol request

        // 2xx - success response
        SUCCESSFUL          = 200, //!< generic name for a 2xx class response
        OK                  = 200, //!< client's request was successfully received, understood, and accepted
        CREATED             = 201, //!< new resource has been created
        ACCEPTED            = 202, //!< request accepted but not complete
        NONAUTH             = 203, //!< non-authoritative info (ok)
        NOCONTENT           = 204, //!< request fulfilled, no message body
        RESETCONTENT        = 205, //!< request success, reset document view
        PARTIALCONTENT      = 206, //!< server has fulfilled partial GET request

        // 3xx - redirection response
        REDIRECTION         = 300, //!< generic name for a 3xx class response
        MULTIPLECHOICES     = 300, //!< requested resource corresponds to a set of representations
        MOVEDPERMANENTLY    = 301, //!< requested resource has been moved permanently to new URI
        FOUND               = 302, //!< requested resources has been moved temporarily to a new URI
        SEEOTHER            = 303, //!< response can be found under a different URI
        NOTMODIFIED         = 304, //!< response to conditional get when document has not changed
        USEPROXY            = 305, //!< requested resource must be accessed through proxy
        RESERVED            = 306, //!< old response code - reserved
        TEMPREDIRECT        = 307, //!< requested resource resides temporarily under a different URI

        // 4xx - client error response
        CLIENTERROR         = 400, //!< generic name for a 4xx class response
        BADREQUEST          = 400, //!< request could not be understood by server due to malformed syntax
        UNAUTHORIZED        = 401, //!< request requires user authorization
        PAYMENTREQUIRED     = 402, //!< reserved for future user
        FORBIDDEN           = 403, //!< request understood, but server will not fulfill it
        NOTFOUND            = 404, //!< Request-URI not found
        METHODNOTALLOWED    = 405, //!< method specified in the Request-Line is not allowed
        NOTACCEPTABLE       = 406, //!< resource incapable of generating content acceptable according to accept headers in request
        PROXYAUTHREQ        = 407, //!< client must first authenticate with proxy
        REQUESTTIMEOUT      = 408, //!< client did not produce response within server timeout
        CONFLICT            = 409, //!< request could not be completed due to a conflict with current state of the resource
        GONE                = 410, //!< requested resource is no longer available and no forwarding address is known
        LENGTHREQUIRED      = 411, //!< a Content-Length header was not specified and is required
        PRECONFAILED        = 412, //!< precondition given in request-header field(s) failed
        REQENTITYTOOLARGE   = 413, //!< request entity is larger than the server is able or willing to process
        REQURITOOLONG       = 414, //!< Request-URI is longer than the server is willing to interpret
        UNSUPPORTEDMEDIA    = 415, //!< request entity is in unsupported format
        REQUESTRANGE        = 416, //!< invalid range in Range request header
        EXPECTATIONFAILED   = 417, //!< expectation in Expect request-header field could not be met by server

        // 5xx - server error response
        SERVERERROR         = 500, //!< generic name for a 5xx class response
        INTERNALSERVERERROR = 500, //!< an unexpected condition prevented the server from fulfilling the request
        NOTIMPLEMENTED      = 501, //!< the server does not support the functionality required to fulfill the request
        BADGATEWAY          = 502, //!< invalid response from gateway server
        SERVICEUNAVAILABLE  = 503, //!< unable to handle request due to a temporary overloading or maintainance
        GATEWAYTIMEOUT      = 504, //!< gateway or DNS server timeout
        HTTPVERSUNSUPPORTED = 505, //!< the server does not support the HTTP protocol version that was used in the request

        PENDING      = 0xFFFFFFFF, //!< response has not yet been received
    };

    /*!
     * @brief Deleted constructor to prevent accidental creation.
     */
    HttpError() = delete;

    /*!
     * @brief Deleted destructor.
     */
    ~HttpError() = delete;
};


/*!
 * @brief EADP SDK SSL Error
 * 
 * This class is mainly for hosting error code in specific domain for documentation purpose, so no create() method provided.
 */
class SslError
{
public:
    /*!
     * Domain for EADP SDK SSL error.
     */
    static const char8_t* kDomain;

    /*!
     * Error code for EADP SDK SSL error.
     */
    enum class Code : uint32_t
    {
        ERROR_NONE          =        0x0, //!< no error
        ERROR_DNS           = 0xFFFFFFFF, //!< DNS failure
        ERROR_CONN          = 0xFFFFFFF6, //!< TCP connection failure
        ERROR_CONN_SSL2     = 0xFFFFFFF5, //!< connection attempt was using unsupported SSLv2 record format
        ERROR_CONN_NOTSSL   = 0xFFFFFFF4, //!< connection attempt was not recognized as SSL
        ERROR_CONN_MINVERS  = 0xFFFFFFF3, //!< request failed minimum protocol version restriction
        ERROR_CONN_MAXVERS  = 0xFFFFFFF2, //!< request failed maximum protocol version restriction
        ERROR_CONN_NOCIPHER = 0xFFFFFFF1, //!< no supported cipher
        ERROR_CERT_INVALID  = 0xFFFFFFEC, //!< certificate invalid
        ERROR_CERT_HOST     = 0xFFFFFFEB, //!< certificate not issued to this host
        ERROR_CERT_NOTRUST  = 0xFFFFFFEA, //!< certificate is not trusted (recognized)
        ERROR_CERT_MISSING  = 0xFFFFFFE9, //!< certificate not provided in certificate message
        ERROR_CERT_BADDATE  = 0xFFFFFFE8, //!< certificate date range validity check failed
        ERROR_CERT_REQUEST  = 0xFFFFFFE7, //!< CA fetch request failed
        ERROR_SETUP         = 0xFFFFFFE2, //!< failure in secure setup
        ERROR_SECURE        = 0xFFFFFFE1, //!< failure in secure connection after setup
        ERROR_UNKNOWN       = 0xFFFFFFE0, //!< unknown failure
    };

    /*!
     * @brief Deleted constructor to prevent accidental creation.
     */
    SslError() = delete;

    /*!
     * @brief Deleted destructor.
     */
    ~SslError() = delete;
};

}
}
