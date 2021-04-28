#ifndef LOCALHOST_H
#define LOCALHOST_H
#include <QSslError>
#include <QTcpServer>
#include <QThread>

#include "LocalHostConfig.h"

namespace Origin
{
	namespace Sdk
	{
        namespace LocalHost
        {
            ///
            /// starts the local host service
            ///
            void start(ILocalHostConfig *config, bool useManualConnections = false);
 
#ifdef ORIGIN_MAC
            ///
            /// adds a manual localhost connection via socket descriptor (used by Mac launchd service only)
            ///
            void addManualConnection(int sd);
            
            ///
            /// adds a manual localhost SSL connection via socket descriptor (used by Mac launchd service only)
            ///
            void addManualConnectionSSL(int sd);
#endif

            ///
            /// stops the local host service
            ///
            void stop();

            ///
            /// starts the local host responder service (beacon)
            ///
            void startResponderService();

            ///
            /// stops the local host responder service (beacon)
            ///
            void stopResponderService();
        }
	}
}
#endif // LOCALHOST_LOCALHOST_H