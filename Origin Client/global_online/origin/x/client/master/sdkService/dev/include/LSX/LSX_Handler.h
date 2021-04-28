///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: LSX_Handler.h
//	Main handler for LSX requests
//	
//	Author: Hans van Veenendaal


#ifndef _LSX_HANDLER_H_
#define _LSX_HANDLER_H_

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QHash>
#include <QString>

#include <lsx.h>

#include "LSX/LsxServer.h"
#include "LSX/LsxMessages.h"

#include "services/crypto/SimpleEncryption.h"

#define SYSTEM_SERVICE_AREA_NAME "EALS"

namespace Origin
{
    namespace SDK
    {
        class BaseService;

        namespace Lsx
        {
            class Connection;

            class LSX_Handler : public QObject
            {
                Q_OBJECT
            public:

                LSX_Handler();
                virtual ~LSX_Handler();

                enum Status
                {
                    StatusSuccess = 0,
                    StatusFailed,
                    StatusFailedConnection,
                    StatusNeedToCreateUser
                };

                //
                // Process request
                void processRequest( Lsx::Request * request );

                void clientAuthenticationChallenge( Lsx::Response& response, QByteArray &key );

                bool authenticateClient( const lsx::ChallengeResponseT &challengeResponse, Lsx::Connection *connection, const QByteArray & key, Lsx::Response& response );

                template<class T>
                int sendEvent(T & msg, const QString &sender, const QByteArray &multiplayerId )
                {
                    if( m_server )
                        return m_server->sendEvent( msg, sender, multiplayerId );
                    else
                        return 0;
                }

                class Lsx::Server *lsxServer() const
                {
                    return m_server;
                }

                void setServer(class Lsx::Server* server)
                {
                    m_server = server;
                }

                unsigned int connectionCount() { return m_server->connectionCount(); }

                const QHash<LsxSocket*, TcpConnection*> &connections(){ return m_server->connections(); }

            signals:
                void loggedIn(QString username, bool authenticatedOnline);
                void loggedOut();

                public slots:
                    // For testing SDK games launched externally
                void onConnectionHangup( const QString& productId );

            private:
                void workerRoutine();

                bool extractGameInformationToConnection(const lsx::ChallengeResponseT &resp, Lsx::Connection *connection);

                void startServiceAreas();
                void stopServiceAreas();

                void checkSettings();

            private:
                QHash< QString, BaseService*> m_serviceAreas;
                Services::Crypto::SimpleEncryption m_encryption;
                QMutex m_lock;
                QMutex m_settingsLock;
                class Lsx::Server* m_server;
            };
        }
    }
}

#endif //_LSX_HANDLER_H_
