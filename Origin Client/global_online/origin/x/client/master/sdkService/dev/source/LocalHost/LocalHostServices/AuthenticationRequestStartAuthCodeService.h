#ifndef LOCALHOST_AUTHREQSTARTAUTHCODESERVICE_H
#define LOCALHOST_AUTHREQSTARTAUTHCODESERVICE_H

#include "LocalHost/LocalHostServices/IService.h"
#include <QTimer>

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class AuthenticationRequestStartAuthcodeService : public IService
            { 
                public:
                    AuthenticationRequestStartAuthcodeService(QObject *parent);
                    ~AuthenticationRequestStartAuthcodeService();

                    virtual ResponseInfo processRequest(QUrl requestUrl);

                private:
                    QTimer mRequestTimeoutTimer;
            };
        }
    }
}

#endif