///////////////////////////////////////////////////////////////////////////////
// AuthenticationServiceResponse.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef __HTTP_STATUS_CODES_H_INCLUDED_
#define __HTTP_STATUS_CODES_H_INCLUDED_

namespace Origin
{
    namespace Services
    {
        ///
        /// Reference: http://www.w3.org/Protocols/rfc2616/rfc2616-sec10.html
        ///
        enum HttpStatusCode
        {
            HttpSuccess                                     = 0
            // informational
            ,Http100Continue                                = 100 // continue
            ,Http101SwitchingProtocols
            // succesful
            ,Http200Ok                                      = 200 // Ok
            ,Http201Created
            ,Http202Accepted
            ,Http203NonAuthoritativeInfo
            ,Http204NoContent
            ,Http205ResetContent
            ,Http206PartialContent
            // redirection
            ,Http300MultipleChoices                         = 300
            ,Http301MovedPermanently
            ,Http302Found
            ,Http303SeeOther
            ,Http304NotModified
            ,Http305UseProxy
            ,Http306Unused
            ,Http307TemporaryRedirect
            // errors
            ,Http400ClientErrorBadRequest                   = 400 // Bad Request
            ,Http401ClientErrorUnauthorized                 = 401 // Unauthorized
            ,Http402ClientErrorPaymentRequired              = 402 // Payment Required
            ,Http403ClientErrorForbidden                    = 403 // Forbidden
            ,Http404ClientErrorNotFound                     = 404 // Not Found
            ,Http405ClientErrorMethodNotAllowed             = 405 // Method Not Allowed
            ,Http406ClientErrorNotAcceptable                = 406 // Not Acceptable
            ,Http407ClientErrorProxyAuthenticationRequired  = 407 // Proxy Authentication Required
            ,Http408ClientErrorRequestTimeout               = 408 // Request Timeout
            ,Http409ClientErrorConflict                     = 409 // Conflict
            ,Http410ClientErrorGone                         = 410 // Gone
            ,Http411ClientErrorLengthRequired               = 411 // Length Required
            ,Http412ClientErrorPreconditionFailed           = 412 // Precondition Failed
            ,Http413ClientErrorRequestEntityTooLarge        = 413 // Request Entity Too Large
            ,Http414ClientErrorRequestURITooLong            = 414 // Request-URI Too Long
            ,Http415ClientErrorUnsupportedMediaType         = 415 // Unsupported Media Type
            ,Http416ClientErrorRequestedRangeNotSatisfiable = 416 // Requested Range Not Satisfiable
            ,Http417ClientErrorExpectationFailed            = 417 // Expectation Failed
            // server errors
            ,Http500InternalServerError = 500
            ,Http501NotImplemented
            ,Http502BadGateway
            ,Http503ServiceUnavailable
            ,Http504GatewayTimeout
            ,Http505HTPVersionNotSupported

        };
    }
}


#endif // __HTTP_STATUS_CODES_H_INCLUDED_