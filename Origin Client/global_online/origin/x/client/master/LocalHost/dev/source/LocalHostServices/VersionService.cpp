//
//  VersionService.cpp
//  sdkService
//
//  Created by Frederic Meraud on 1/17/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#include "LocalHost/LocalHostServices/VersionService.h"

#include "version/version.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            ResponseInfo VersionService::processRequest(QUrl requestUrl)
            {
                ResponseInfo responseInfo;
                responseInfo.response = QString("{\"resp\":\"%1\"}").arg(EBISU_PRODUCT_VERSION_P_DELIMITED);
                responseInfo.errorCode = HttpStatus::OK;
                return responseInfo;
            }
        }
    }
}