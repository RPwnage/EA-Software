#include "LocalHost/LocalHostServiceHandler.h"
#include "LocalHostSecurity.h"
#include "LocalHost/LocalHostServices/IService.h"

namespace Origin
{
    namespace Sdk
    {
        namespace LocalHost
        {
            static LocalHostServiceHandler *sLocalHostServiceHandlerInstance = NULL;

            void LocalHostServiceHandler::registerServices(ILocalHostConfig* config)
            {
                if(!sLocalHostServiceHandlerInstance)
                {
                    sLocalHostServiceHandlerInstance = new LocalHostServiceHandler();
                }

                LocalHostSecurity::start();

                //register all the services available for the local host
                config->registerServices(sLocalHostServiceHandlerInstance);
            }

            void LocalHostServiceHandler::setBoundedPort(quint16 port)
            {
                if(sLocalHostServiceHandlerInstance)
                {
                    sLocalHostServiceHandlerInstance->mPort = port;
                }
            }

            void LocalHostServiceHandler::registerService(QString uri, IService *service)
            {
                if(sLocalHostServiceHandlerInstance)
                {
                    sLocalHostServiceHandlerInstance->mUrlToHandlerMap[uri.toLower()] = service;
                }
            }

            void LocalHostServiceHandler::deregisterServices(ILocalHostConfig* config)
            {
                if(sLocalHostServiceHandlerInstance)
                {
                    config->deregisterServices(sLocalHostServiceHandlerInstance);

                    delete sLocalHostServiceHandlerInstance;
                    sLocalHostServiceHandlerInstance = NULL;
                }

                LocalHostSecurity::stop();
            }

            IService *LocalHostServiceHandler::findService(QString uri)
            {
                uri = uri.toLower();
                for(QMap<QString, IService *>::ConstIterator it = sLocalHostServiceHandlerInstance->mUrlToHandlerMap.constBegin();
                    it != sLocalHostServiceHandlerInstance->mUrlToHandlerMap.constEnd();
                    it++)
                {
                    QString path = it.key();

                    QRegExp reg(path);
                    if(reg.exactMatch(uri))
                    {
                        return dynamic_cast<IService *>(it.value());
                    }
                }

                return NULL;
            }

            quint16 LocalHostServiceHandler::port()
            {
                quint16 listeningPort = 0;
                if(sLocalHostServiceHandlerInstance)
                {
                    listeningPort = sLocalHostServiceHandlerInstance->mPort;
                }

                return listeningPort;
            }
        }
    }
}
