#include "LocalHost/localHost.h"
#include "LocalHostServer.h"
#include "services/debug/DebugService.h"

#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"

#if defined(ORIGIN_PC)
#include "services/escalation/ServiceUtilsWin.h"
#elif defined(ORIGIN_MAC)
#include "services/escalation/ServiceUtilsOSX.h"
#endif


#include <QThread>

namespace Origin
{
	namespace Sdk
	{
        namespace LocalHost
        {
		    static QThread *sLocalHostServerThread= NULL;
            static QSharedPointer<ILocalHostConfig> sLocalHostConfig;
            static LocalHostServer *sLocalHostServer = NULL;
            static LocalHostServerSSL *sLocalHostServerSSL = NULL;

            void start(ILocalHostConfig *config, bool useManualConnections)
            {
                // Take ownership via a QSharedPointer
                QSharedPointer<ILocalHostConfig> pConfig(config);

                if(!sLocalHostServerThread && !sLocalHostServer)
                {
                    // Save the config for later use
                    sLocalHostConfig = pConfig;

                    // Call the onStart handler
                    sLocalHostConfig->onStart();

                    sLocalHostServerThread = new QThread;
                    sLocalHostServerThread->setObjectName("LocalHostServer Thread");
                    sLocalHostServer = new LocalHostServer(sLocalHostConfig);  // non-SSL server
                    sLocalHostServer->moveToThread(sLocalHostServerThread);
                    sLocalHostServerSSL = new LocalHostServerSSL(sLocalHostConfig);  // SSL server
                    sLocalHostServerSSL->moveToThread(sLocalHostServerThread);
                    sLocalHostServerThread->start();
                    
                    if (!useManualConnections)
                    {
                        QMetaObject::invokeMethod(sLocalHostServer,    "startListenForConnection", Qt::QueuedConnection);
                        QMetaObject::invokeMethod(sLocalHostServerSSL, "startListenForConnection", Qt::QueuedConnection);
                    }
                    else
                    {
                        QMetaObject::invokeMethod(sLocalHostServer,    "startWaitingForConnections", Qt::QueuedConnection);
                        QMetaObject::invokeMethod(sLocalHostServerSSL, "startWaitingForConnections", Qt::QueuedConnection);
                    }
                }
                else
                {
                    ORIGIN_ASSERT_MESSAGE(0, "Local Host service already started!!!");
                }
            }
            
#ifdef ORIGIN_MAC
            ///
            /// adds a manual localhost connection via socket descriptor (used by Mac launchd service only)
            ///
            void addManualConnection(int sd)
            {
                if(sLocalHostServer)
                {
                    QMetaObject::invokeMethod(sLocalHostServer, "onNewConnectionPendingManual", Qt::QueuedConnection, Q_ARG(int,sd));
                }
            }
            
            ///
            /// adds a manual localhost SSL connection via socket descriptor (used by Mac launchd service only)
            ///
            void addManualConnectionSSL(int sd)
            {
                if(sLocalHostServerSSL)
                {
                    QMetaObject::invokeMethod(sLocalHostServerSSL, "onNewConnectionPendingManual", Qt::QueuedConnection, Q_ARG(int,sd));
                }
            }
#endif

            void stop()
            {
                if(sLocalHostServer)
                {
                    sLocalHostServer->deleteLater();
                    sLocalHostServer = NULL;
                }
                if(sLocalHostServerSSL)
                {
                    sLocalHostServerSSL->deleteLater();
                    sLocalHostServerSSL = NULL;
                }

                if(sLocalHostServerThread)
                {
                    const int waitForThreadTimeOutinMsecs = 5000;
                    sLocalHostServerThread->quit();

                    //make sure the thread quit before deleting it
                    if(sLocalHostServerThread->wait(waitForThreadTimeOutinMsecs))
                    {
                        delete sLocalHostServerThread;
                        sLocalHostServerThread = NULL;
                    }
                }

                // Call the onStop handler
                if (!sLocalHostConfig.isNull())
                {
                    sLocalHostConfig->onStop();

                    // Release the reference
                    sLocalHostConfig.clear();
                }                
            }


            ///
            /// starts the local host responder service (beacon)
            ///
            void startResponderService()
            {
#if defined(ORIGIN_PC)
                // Is the Origin Beacon Service already running?  If so, we don't need to do anything else
                bool serviceInstalled = false;
                bool running = false;
                if (Escalation::QueryOriginServiceStatus(QS16(ORIGIN_BEACON_SERVICE_NAME), running))
                {
                    serviceInstalled = true;
                    if (running)
                    {
                        ORIGIN_LOG_EVENT << "Web Helper Service already running.";
                        return;
                    }
                }
                else
                {
                    ORIGIN_LOG_WARNING << "Unable to query Web Helper Service status.";
                    return;
                }

                if (!serviceInstalled)
                {
                    ORIGIN_LOG_WARNING << "Web Helper service not installed.";
                    return;
                }

                // Starting beacon service
                if (!Escalation::StartOriginService(QS16(ORIGIN_BEACON_SERVICE_NAME), QString(""), 2000)) // Wait only 2 seconds instead of the default 15
                {
                    ORIGIN_LOG_WARNING << "Unable to start Web Helper service.";
                    return;
                }

                ORIGIN_LOG_EVENT << "Web Helper Service started.";
#elif defined(ORIGIN_MAC)
                ORIGIN_LOG_EVENT << "Installing Web Helper Service (if not already installed)...";
                
                Escalation::InstallService(ORIGIN_BEACON_SERVICE_NAME);
#endif
            }

            ///
            /// stops the local host responder service (beacon)
            ///
            void stopResponderService()
            {
#if defined(ORIGIN_PC)
                bool running = false;
                if (Escalation::QueryOriginServiceStatus(QS16(ORIGIN_BEACON_SERVICE_NAME), running))
                {
                    if (running)
                    {
                        ORIGIN_LOG_EVENT << "Web Helper Service running.  Stopping service.";

                        Escalation::StopOriginService(QS16(ORIGIN_BEACON_SERVICE_NAME), 2000); // Wait only 2 seconds instead of the default 15

                        return;
                    }
                }
#elif defined(ORIGIN_MAC)
                ORIGIN_LOG_EVENT << "Removing Web Helper Service (if installed)...";
                
                Escalation::UninstallService(ORIGIN_BEACON_SERVICE_NAME);
#endif
            }
        }
	}
}
