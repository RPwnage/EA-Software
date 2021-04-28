///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: Server.h
//	LSX server and protocol definitions
//	
//	Author: Hans van Veenendaal


#ifndef _LSX_SERVER_H_
#define _LSX_SERVER_H_

#include <QObject>
#include <QHash>
#include <QMutex>
#include "LSX/LsxMessages.h"
#include "LSX/LsxEncryptor.h"
#include "LSX/tcpserver.h"
#include "LSX/jsserver.h"
#include "LSX/LsxConnection.h"
#include "LSX/LsxTcpConnection.h"
#include <LsxWriter.h>

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            class Server : public QObject, public LsxAuthDelegate
            {
                Q_OBJECT
            public:
                Server( class LSX_Handler* handler );
                virtual ~Server();

                template<class T>
                int sendEvent( T & msg, const QString &sender, const QByteArray &multiplayerId = "" )
                {
                    int sentTo = 0;

                    QMutexLocker locker(&m_tcpConnHashLock);

                    if(m_tcpConnHash.empty())
                        return sentTo;

                    // And all of our sockets
                    for(TcpConnHash::ConstIterator it = m_tcpConnHash.begin();
                        it != m_tcpConnHash.end();
                        it++)
                    {
                        if(multiplayerId.isEmpty() || (*it)->multiplayerId() == multiplayerId)
                        {
                            if(!(*it)->blockUnsolicitedEvents())
                            {
                                Response response(Response::RESPONSETYPE_EVENT, (*it)->protocol());
                                response.setSender(sender.toLatin1());

                                lsx::Write(response.document(), msg);

                                (*it)->sendResponse(response);
                                sentTo++;
                            }
                        }
                    }
                    return sentTo;
                }
                

                // Listens on our TCP socket
                bool listen();

                // Shut down the LSX server
                void shutdown();

                unsigned int connectionCount();

                const QHash<LsxSocket*, TcpConnection*> &connections() const;

            signals:

                // This is so that our internal server know that we have shut down
                void lsxServerShutdown();

                // This is helpful so you don't have to listen to hungup() on every
                // connection
                // There's no Connection* here as it's probably been freed. If you need
                // to know use the hungup() signals as above
                void connectionHungup(const QString& contentId);

            protected slots:
                void socketClosed();
                void socketReadable();

            protected:
                typedef QHash<LsxSocket*, TcpConnection*> TcpConnHash;
                typedef QMap<QString, Encryptor*> TcpEncryptionLookup;

                virtual QByteArray clientAuthenticationChallenge(QByteArray &key);
                virtual bool authenticateClient( const QByteArray& challengeResponse, const QByteArray& key, QByteArray& response, LsxSocket *socket );

            private:
                void process( const QByteArray &packet, LsxSocket *socket);

            private:
                class LSX_Handler* m_handler;

                TcpConnHash m_tcpConnHash;
                QHash<LsxSocket*, QByteArray> m_receiveBuffers;

                TCPServer *m_tcpServer;
                JSServer *m_jsServer;

                TcpEncryptionLookup m_encryptionLookup;

                QMutex m_tcpConnHashLock;
            };
        }
    }
}


#endif //_LSX_SERVER_H_
