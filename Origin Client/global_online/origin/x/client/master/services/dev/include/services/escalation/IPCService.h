#ifndef _IPCSERVICE_H_
#define _IPCSERVICE_H_

#include <QObject>
#include <QtNetwork>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{ 
    namespace Escalation
    {
        class ORIGIN_PLUGIN_API IPCClient : public QObject
        {
            Q_OBJECT
        public:
            explicit IPCClient(QObject* parent = 0);
            ~IPCClient();

        public slots:
            QString sendMessage(const QString &message);

            void socket_connected();
            void socket_disconnected();
            void socket_readReady();
            void socket_error(QLocalSocket::LocalSocketError);

        private:
            QLocalSocket*     mSocket;
            QString         mMessage;
            QString         mResponse;

        };

        class ORIGIN_PLUGIN_API IPCServer: public QObject
        {
            Q_OBJECT
        public:
            // Initialize the IPC server to listen for new connections
            explicit IPCServer(QObject* parent = 0);
            
            // Initialize the IPC server to use pending connections that a separate entity/object was listening for/accepted
            explicit IPCServer(bool useManualConnections, QObject* parent = 0);
            
            ~IPCServer();

            virtual bool validatePipeClient(const quint32 clientPid, QString& errorResponse) = 0;
            virtual QString processMessage(const QString) = 0;
            
            // Use this method only when the server is setup to use pending connections managed by a separate entity/object.
            void addPendingConnection(int socketID);
            
        public slots:
            void newConnectionEstablished();
			void newDataAvailable();
			void connectionDisconnected();
            
        signals:
            void clientValidationFailed();
            void newDataProcessed();
            
        private:
            void sendResponse(const QString &messag, QLocalSocket *clientConnection);
            QLocalServer*        mServer;
            QList<QLocalSocket*> mManualConnections;
            void*                mFnCheckPipeClientPID;
            QSet<quint32>        mValidatedCallers;
            
		protected:
			bool mIsValid;
        };
    }
}
#endif // _IPCSERVICE_H_
