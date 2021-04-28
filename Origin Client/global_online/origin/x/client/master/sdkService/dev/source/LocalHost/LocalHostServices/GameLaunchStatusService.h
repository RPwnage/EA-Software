#ifndef LOCALHOST_GAMELAUNCHSTATUS_SERVICE_H
#define LOCALHOST_GAMELAUNCHSTATUS_SERVICE_H

#include "Origin2StatusService.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class GameLaunchStatusService : public Origin2StatusService
            {
                public:
                    GameLaunchStatusService(QObject *parent):Origin2StatusService(parent) {};
                    ~GameLaunchStatusService() {};

                    virtual ResponseInfo processRequest(QUrl requestUrl);

                protected:
                    QString rtpStateToStringLookup(int state);
            };
        }
    }
}

#endif