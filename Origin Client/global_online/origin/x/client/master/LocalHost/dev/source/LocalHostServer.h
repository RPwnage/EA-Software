#ifndef LOCALHOST_LocalHostServer_H
#define LOCALHOST_LocalHostServer_H

#include <QSslError>
#include <QTcpServer>
#include <QThread>
#include <QSslSocket>
#include <QQueue>

#include "LocalHost/LocalHostConfig.h"

namespace Origin
{
	namespace Sdk
	{
        namespace LocalHost
        {
            class IService;

		    class LocalHostServer : public QTcpServer
		    {
			    Q_OBJECT

		    public:
                LocalHostServer(QSharedPointer<ILocalHostConfig> config);
                ~LocalHostServer();

                virtual quint16 getListenPort();
                
                virtual QTcpSocket *nextPendingConnection();

            protected:
                virtual void incomingConnection(qintptr handle);
                
            private slots:
                ///
                /// start listening for incoming connections
                ///
                void startListenForConnection();

                ///
                /// perform a little initialization, then wait for connections to be provided manually
                ///
                void startWaitingForConnections();
                
                ///
                /// stop listening for incoming connections
                ///
                void stopListenForConnection();

                ///
                /// triggered on new incoming connection
                ///
                void onNewConnectionPending();
                
                ///
                /// invoked when a new socket descriptor is available with a pending connection
                ///
                void onNewConnectionPendingManual(int sd);

            protected:
                QSharedPointer<ILocalHostConfig> mConfig;
                QQueue<QTcpSocket*> mPendingConnections;
		    };

            class LocalHostServerSSL : public LocalHostServer
            {
                Q_OBJECT

            public:
                LocalHostServerSSL(QSharedPointer<ILocalHostConfig> config);

                virtual quint16 getListenPort();

            protected:
                virtual void incomingConnection(qintptr handle);

            private slots:

                ///
                /// triggered on new incoming connection
                ///
                void onNewConnectionPendingSSL();

            };


        }
	}
}
#endif // HTTPSLocalHostServer_H