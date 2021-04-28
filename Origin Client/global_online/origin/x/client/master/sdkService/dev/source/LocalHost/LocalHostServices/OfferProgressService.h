#ifndef LOCALHOST_OFFERPROGRESSSERVICE_H
#define LOCALHOST_OFFERPROGRESSSERVICE_H

#include "LocalHost/LocalHostServices/IService.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class OfferProgressService : public IService
            {
                public:
                    OfferProgressService(QObject *parent):IService(parent) {};
                    ~OfferProgressService() {};

                    virtual ResponseInfo processRequest(QUrl requestUrl);

            };
        }
    }
}

#endif