///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2008 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: Server.cpp
//	LSX server implementation. Conforms to Flash XMLSocket specification
//	
//	Author: Sergey Zavadski
#include <lsx.h>
#include <lsxReader.h>

#include <EATrace/EATrace.h>

#include <QMutexLocker>
#include <QTcpSocket>

#include "LSX/LSX_Handler.h"
#include "LSX/LsxServer.h"
#include "LSX/LsxTcpConnection.h"
#include "LSX/LsxRijndaelEncryptor.h"

#include "services/crypto/SimpleEncryption.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"

#include "engine/content/ContentController.h"
#include "engine/content/Entitlement.h"
#include "engine/utilities/ContentUtils.h"

using Origin::Engine::Content::ContentController;

#define LSX_PORT 3216

// #define EVENT_DEBUG_LOG  // to turn on logging of all lsx server events

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            Server::Server( LSX_Handler* handler )
                : QObject(handler), 
                m_handler( handler )
            {
                m_handler->setServer( this );
                m_tcpServer = new TCPServer(handler);
                m_tcpServer->setAuthDelegate(this);

                m_jsServer = new JSServer(handler);
                m_jsServer->setAuthDelegate(this);

                // The encryption method used is determined by the SDK version.
                // populate table for sdk version vs encryptor.
                // TODO add encryptor for each major SDK version.
                m_encryptionLookup["0"] = new Encryptor(); // Plain text - no encryption.
                m_encryptionLookup["1"] = new Encryptor();
                m_encryptionLookup["2"] = new RijndaelEncryptor();
                m_encryptionLookup["3"] = new RijndaelEncryptor();
            }

            bool Server::listen()
            {
                unsigned short port = (unsigned short) Origin::Services::readSetting(Origin::Services::SETTING_LSX_PORT).toQVariant().toInt();

                if(port == 0)
                {
                    port = LSX_PORT;
                }

                return m_tcpServer->listen(QHostAddress::LocalHost, port) && m_jsServer->listen(QHostAddress::LocalHost, port+1);  
            }

            void Server::shutdown()
            {
                // Stop listening for new connections
                ORIGIN_LOG_EVENT << "Close server";
                m_tcpServer->close();

                m_jsServer->close();

                // Let it be known that the LSX server is being shutdown
                ORIGIN_LOG_EVENT << "Signal shutdown";
                emit (lsxServerShutdown());

                // Disconnect everyone connected and attempt to flush writes
                // They'll probably be forcibly disconnected pretty soon anyway but this
                // prevents new messages from coming in
                ORIGIN_LOG_EVENT << "Close connection";

                QMutexLocker locker(&m_tcpConnHashLock);

                while(!m_tcpConnHash.empty())
                {
                    m_tcpConnHash.begin().key()->close();
                }

                ORIGIN_LOG_EVENT << "Done";
            }

            Server::~Server()
            {
                ORIGIN_LOG_EVENT << "LSX Server cleanup";
            }

            void Server::process( const QByteArray &packet, LsxSocket *socket )
            {
                TcpConnection *connection;
                {
                    QMutexLocker locker(&m_tcpConnHashLock);
                    connection = m_tcpConnHash[socket];
                }

                Request *lsxRequest = new Request(packet, connection, socket->protocol());
                m_handler->processRequest(lsxRequest);
            }

            void Server::socketClosed()
            {
                LsxSocket *socket = dynamic_cast<LsxSocket*>(sender());
                // Remove the TCP connection from our hash if present
                TcpConnection *connection;
                {
                    QMutexLocker locker(&m_tcpConnHashLock);
                    connection = m_tcpConnHash.take(socket);
                }
                
                if (connection)
                {
                    // Get rid of the socket
                    socket->disconnect();
                    socket->deleteLater();

                    // This object will actually go away when its reference count hits
                    // zero
                    QString productId = connection->productId();
                    connection->hangup();
                    emit(connectionHungup(productId));
                }
            }

            void Server::socketReadable()
            {
                LsxSocket *socket = dynamic_cast<LsxSocket*>(sender());

                process(socket->readMessage(), socket);
            }

            unsigned int Server::connectionCount()
            {
                return m_tcpConnHash.size();
            }

            const QHash<LsxSocket*, TcpConnection*> &Server::connections() const
            {
                return m_tcpConnHash;
            }

            QByteArray Server::clientAuthenticationChallenge(QByteArray &key)
            {
                Lsx::Response response(Response::RESPONSETYPE_EVENT);

                m_handler->clientAuthenticationChallenge( response , key);		

                QByteArray eventBytes = response.toByteArray();

                return eventBytes;
            }


            bool Server::authenticateClient(const QByteArray& challengeResponse, const QByteArray& key, QByteArray& response, LsxSocket *socket)
            {
                // Create a connection for m_handler to set game information for
                // If auth succeeds this is put in m_tcpConnHash
                // If auth fails it's dereferenced immediately
                Lsx::TcpConnection *connection = new TcpConnection(this, socket);

                Lsx::Request request(challengeResponse, connection, socket->protocol());			// <== Protocol depends on connection.

				lsx::ChallengeResponseT resp;
				lsx::Read(request.document(), resp);


                Response serverResponse(request);
                bool auth = m_handler->authenticateClient( resp, connection, key, serverResponse );

                QByteArray serverResponseBytes = serverResponse.toByteArray();

                response.append(serverResponseBytes);

                if (auth)
                {
                    // Encryption method is derived from SDK version. The version is contained in each SDK LSX message received.
                    QString sdk_version = resp.ProtocolVersion;
                    TcpEncryptionLookup::iterator it = m_encryptionLookup.find(sdk_version);
                    if (it != m_encryptionLookup.end())
                    {
                        socket->setEncryptor(it.value(), connection->seed());
                        
                        m_tcpConnHash[socket] = connection;

                        ORIGIN_VERIFY_CONNECT(socket, SIGNAL(disconnected()), this, SLOT(socketClosed()));
                        ORIGIN_VERIFY_CONNECT(socket, SIGNAL(readReady()), this, SLOT(socketReadable()));

                        // check if ContentController exists at all (user might not have logged in yet)
                        Origin::Engine::Content::ContentController *cc = ContentController::currentUserContentController();
                        if (cc != NULL)
                        {
                            // Notify the content controller that an SDK connection has been established
                            QMetaObject::invokeMethod(cc, "sdkConnectionEstablished", Qt::QueuedConnection, 
                                Q_ARG(const QString &, request.connection()->productId()), 
                                Q_ARG(const QString &, request.connection()->gameName()));
                        }

                        connection->signalConnected();
                    }
                    else
                    {
                        auth = false;
                        connection->hangup();
                        EA_ASSERT(false);
                        ORIGIN_LOG_ERROR << "An SDK version not supported attempted a connection, version = " << sdk_version;
                    }
                }
                else
                {
                    connection->hangup();
                }

                return auth;
            }
        }
    }
}


