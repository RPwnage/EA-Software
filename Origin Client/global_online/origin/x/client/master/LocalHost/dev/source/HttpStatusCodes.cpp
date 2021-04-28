#include <QString>
#include "LocalHost/HttpStatusCodes.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            using namespace HttpStatus;
            struct ErrorCodes
            {
                int code;
                QString desc;
            };

            ErrorCodes errorCodes[] = 
            {
                { OK, "OK" },
                { Accepted, "ACCEPTED"},
                { BadRequest, "BAD REQUEST" },
                { Forbidden, "FORBIDDEN" },
                { NotFound, "NOT FOUND" },
                { MethodNotAllowed, "METHOD NOT ALLOWED" },
                { RequestTimeout, "REQUEST TIMEOUT"},
                { Conflict, "CONFLICT"},
                { RequestEntityTooLarge, "REQUEST ENTITY TOO LARGE"},
                { InternalServerError, "INTERNAL SERVER ERROR"},
                { NotImplemented, "NOT IMPLEMENTED"},
                { HttpVersionNotSupported, "HTTP VERSION NOT SUPPORTED"},
                { -1, ""}
            };

            QString getMessageFromCode(int code)
            {
                for (int i = 0; errorCodes[i].code != -1; i++)
                {
                    if (errorCodes[i].code == code)
                        return errorCodes[i].desc;
                }
                return "UNKNOWN";
            }

        }
    }
}