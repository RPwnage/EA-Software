#ifndef LOCALHOST_OFFERLAUNCHGAMESERVICE_H
#define LOCALHOST_OFFERLAUNCHGAMESERVICE_H

#include "LocalHost/LocalHostServices/IService.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class OfferLaunchGameService : public IService
            {
            public:
                OfferLaunchGameService(QObject *parent);
                ~OfferLaunchGameService() {};

                virtual ResponseInfo processRequest(QUrl requestUrl);
            };
        }
    }
}

#endif