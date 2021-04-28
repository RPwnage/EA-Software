#ifndef LOCALHOST_HTTPSTATUSCODES_H
#define LOCALHOST_HTTPSTATUSCODES_H

#include <QString>

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            namespace HttpStatus
            {
                const int OK = 200;
                const int Accepted = 202;
                const int BadRequest = 400;
                const int Forbidden = 403;
                const int NotFound = 404;
                const int MethodNotAllowed = 405;
                const int RequestTimeout = 408;
                const int Conflict = 409;
                const int RequestEntityTooLarge = 413;
                const int InternalServerError = 500;
                const int NotImplemented = 501;
                const int HttpVersionNotSupported= 505;
            }
            QString getMessageFromCode(int code);
        }


    }
}

#endif