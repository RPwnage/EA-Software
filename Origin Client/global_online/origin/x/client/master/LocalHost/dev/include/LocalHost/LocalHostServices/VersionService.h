//
//  VersionService.h
//  sdkService
//
//  Created by Frederic Meraud on 1/17/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#ifndef sdkService_VersionService_h
#define sdkService_VersionService_h

#include "IService.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class VersionService : public IService
            {
            public:
                VersionService(QObject *parent):IService(parent) {};
                ~VersionService() {};
                
                virtual ResponseInfo processRequest(QUrl requestUrl);
            };
        }
    }
}


#endif
