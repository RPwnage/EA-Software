#ifndef LOCALHOST_ORIGIN2PASSTHROUGHSERVICE_H
#define LOCALHOST_ORIGIN2PASSTHROUGHSERVICE_H

#include "LocalHost/LocalHostServices/IService.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class Origin2PassThroughService : public IService
            {
                public:
                    Origin2PassThroughService(QObject *parent):IService(parent) {mMethod = "POST";}
                    ~Origin2PassThroughService() {};

                    virtual ResponseInfo processRequest(QUrl requestUrl);

                protected:
                    virtual bool validateQueryParameters(QMap<QString, QString> params);
            };
        }
    }
}

#endif