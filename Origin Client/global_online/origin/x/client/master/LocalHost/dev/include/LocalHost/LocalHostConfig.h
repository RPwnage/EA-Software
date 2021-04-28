#ifndef LOCALHOSTCONFIG_H
#define LOCALHOSTCONFIG_H

#include <QObject>

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class LocalHostServiceHandler;

            class ILocalHostConfig : public QObject
            {
                Q_OBJECT
            public:
                virtual ~ILocalHostConfig() {}

                virtual quint16 getUnsecurePort() = 0;
                virtual quint16 getSecurePort() = 0;

                // Called when the LocalHost is being started or stopped
                virtual void onStart() = 0;
                virtual void onStop() = 0;

                virtual void registerServices(LocalHostServiceHandler* handler) = 0;
                virtual void deregisterServices(LocalHostServiceHandler* handler) = 0;
            };
        }
    }
}
#endif // LOCALHOSTCONFIG_H