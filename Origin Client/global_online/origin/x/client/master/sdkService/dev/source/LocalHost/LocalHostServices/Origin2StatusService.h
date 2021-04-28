#ifndef LOCALHOST_ORIGIN2STATUS_SERVICE_H
#define LOCALHOST_ORIGIN2STATUS_SERVICE_H

#include "LocalHost/LocalHostServices/IService.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class Origin2StatusService : public IService
            {
                public:
                    Origin2StatusService(QObject *parent):IService(parent) {};
                    ~Origin2StatusService() {};

                    virtual ResponseInfo processRequest(QUrl requestUrl);

                protected:
                    QString stateToStringLookup(int state);
            };
        }
    }
}

#endif