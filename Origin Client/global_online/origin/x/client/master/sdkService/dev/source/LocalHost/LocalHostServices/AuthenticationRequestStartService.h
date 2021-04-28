#ifndef LOCALHOST_AUTHREQSTARTSERVICE_H
#define LOCALHOST_AUTHREQSTARTSERVICE_H

#include "LocalHost/LocalHostServices/IService.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class AuthenticationRequestStartService : public IService
            { 
                public:
                    AuthenticationRequestStartService(QObject *parent);
                    ~AuthenticationRequestStartService();

                    virtual ResponseInfo processRequest(QUrl requestUrl);
            };
        }
    }
}

#endif