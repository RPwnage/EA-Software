#include "LocalHost/LocalHostServices/PingService.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            ResponseInfo PingService::processRequest(QUrl requestUrl)
            {
                ResponseInfo responseInfo;
                responseInfo.response = "{\"resp\":\"pong\"}";
                responseInfo.errorCode = HttpStatus::OK;
                return responseInfo;
            }
        }
    }
}