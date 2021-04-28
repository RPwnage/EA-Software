#ifndef LOCALHOST_PINGSERVICE_H
#define LOCALHOST_PINGSERVICE_H

#include "IService.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class PingService : public IService
            {
                public:
                    PingService(QObject *parent):IService(parent) {};
                    ~PingService() {};

                    virtual ResponseInfo processRequest(QUrl requestUrl);
            };
        }
    }
}

#endif