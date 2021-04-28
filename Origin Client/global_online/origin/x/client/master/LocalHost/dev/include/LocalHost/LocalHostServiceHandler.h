#ifndef LOCALHOSTSERVICEHANDLER_H
#define LOCALHOSTSERVICEHANDLER_H

#include <QString>
#include <QMap>
#include <QObject>
#include <QHostAddress>

#include "LocalHost/LocalHostConfig.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            class IService;

            class LocalHostServiceHandler:public QObject
            {
                Q_OBJECT

                public:
                    ///
                    /// instantiate service handler singleton
                    ///
                    static void registerServices(ILocalHostConfig* config);

                    ///
                    /// clean up service handler singleton
                    ///
                    static void deregisterServices(ILocalHostConfig* config);

                    ///
                    /// pass the bounded port to the handler
                    ///
                    static void setBoundedPort(quint16 port);

                    ///
                    /// register a new service to use with this LocalHostServer
                    ///
                    static void registerService(QString uri, IService *service);

                    /// gets the service for the command
                    ///
                    /// \return NULL if not found, pointer to service if found
                    static IService *findService(QString uri);

                    /// gets the port that we registered the services to
                    ///
                    /// \return the port
                    static quint16 port();

                private:
                    ///
                    /// map of the command string to the service
                    ///
                    typedef QMap<QString, IService *> UrlToHandlerMap;
                    UrlToHandlerMap mUrlToHandlerMap;

                    quint16 mPort;
            };
        }
    }
}

#endif