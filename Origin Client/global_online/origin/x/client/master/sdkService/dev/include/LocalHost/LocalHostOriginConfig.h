#ifndef LOCALHOSTORIGINCONFIG_H
#define LOCALHOSTORIGINCONFIG_H

#include "LocalHost/LocalHostConfig.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class LocalHostOriginConfig : public ILocalHostConfig
            {
                Q_OBJECT
            public:
                ~LocalHostOriginConfig();

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
#endif // LOCALHOSTDEFAULTCONFIG_H