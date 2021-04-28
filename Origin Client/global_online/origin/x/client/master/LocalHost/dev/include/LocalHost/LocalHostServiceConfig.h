#ifndef LOCALHOSTSERVICECONFIG_H
#define LOCALHOSTSERVICECONFIG_H

#include "LocalHostConfig.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class LocalHostServiceConfig : public ILocalHostConfig
            {
                Q_OBJECT
            public:
                ~LocalHostServiceConfig();

                virtual quint16 getUnsecurePort();
                virtual quint16 getSecurePort();

                // Called when the LocalHost is being started or stopped
                virtual void onStart();
                virtual void onStop();

                virtual void registerServices(LocalHostServiceHandler* handler);
                virtual void deregisterServices(LocalHostServiceHandler* handler);
            };
        }
    }
}
#endif // LOCALHOSTSERVICECONFIG_H