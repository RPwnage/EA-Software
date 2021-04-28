#ifndef LOCALHOST_AUTHSTATUSSERVICE_H
#define LOCALHOST_AUTHSTATUSSERVICE_H

#include "LocalHost/LocalHostServices/IService.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class AuthenticationStatusService : public IService
            { 
                public:
                    AuthenticationStatusService(QObject *parent);
                    ~AuthenticationStatusService();

                    virtual ResponseInfo processRequest(QUrl requestUrl);
            };
        }
    }
}

#endif