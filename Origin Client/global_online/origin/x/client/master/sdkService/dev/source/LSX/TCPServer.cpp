///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2008 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: TCPServer.cpp
//	Generic TCP server
//	
//	Author: Sergey Zavadski


#include "LSX/TCPServer.h"
#include "LSX/TCPSocket.h"
#include <QTcpSocket>
#include <QTimer>

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            //
            //	Time to wait client authentication response, in seconds
            //
            const static int clientAuthTimeout = 30;

            TCPServer::TCPServer(QObject *parent) :
                QTcpServer(parent),
                m_authDelegate(NULL)
            {
                // Start our timer to timeout pending authentications
                m_pendingAuthSweepTimer = new QTimer(this);
                // Give us a little extra room to make sure we don't race a timeout
                m_pendingAuthSweepTimer->setInterval((clientAuthTimeout + 1) * 1000);

                connect(m_pendingAuthSweepTimer, SIGNAL(timeout()),
                    this, SLOT(sweepPendingAuths()));
                connect(this, SIGNAL(newConnection()), this, SLOT(newConnectionPending()));
            }

            TCPServer::~TCPServer(void)
            {
            }

            void TCPServer::newConnectionPending()
            {
                if (!m_authDelegate)
                {
                    // Nothing to do
                    return;
                }

                QTcpSocket *socket;
                while((socket = nextPendingConnection()))
                {
                    TcpSocket *s = new TcpSocket(socket);

                    // Note that if a socket is disconnected during authentication we treat
                    // it just like a timeout

					PendingAuth pending;

                    //
                    //	Authenticate client
                    //	
                    QByteArray challengeMessage = m_authDelegate->clientAuthenticationChallenge(pending.key);
                    challengeMessage.append('\0');

                    if ( challengeMessage.length() )
                    {
                        socket->write(challengeMessage);
                    }

                    // 
                    //  Record the fact we're waiting for this auth
                    //                  
                    pending.connectTime = QDateTime::currentDateTime();                 
                    m_pendingAuths.insert(s, pending);

                    //
                    //	Wait for response
                    //
                    connect(s, SIGNAL(readReady()), this, SLOT(challengeResponseReady()));

                    //
                    // Start our timeout timer if its not already running
                    //
                    if (!m_pendingAuthSweepTimer->isActive())
                    {
                        m_pendingAuthSweepTimer->start();
                    }	
                }
            }

            void TCPServer::challengeResponseReady()
            {
                TcpSocket *socket = dynamic_cast<TcpSocket*>(sender());

                if (!m_pendingAuths.contains(socket))
                {
                    qWarning("Got an unexpected challenge response");
                    return;
                }

                PendingAuth auth = m_pendingAuths.take(socket);

                if (m_pendingAuths.isEmpty())
                {
                    // We can stop sweeping
                    m_pendingAuthSweepTimer->stop();
                }

                if (!m_authDelegate)
                {
                    // The delegate went away while we were waiting for a response
                    // Lets hope the caller knows what they're doing
                    return;
                }

                QByteArray serverResponse;
                bool authenticated = false;

                QByteArray challengeResponseMessage = socket->readMessage();
                if (challengeResponseMessage.length() > 0)
                {
                    authenticated = m_authDelegate->authenticateClient( challengeResponseMessage, auth.key, serverResponse, socket );
                }

                if ( !authenticated )
                {
                    //
                    //	close connection
                    //
                    socket->deleteLater();
                    return;
                }

                else if ( serverResponse.length() )
                {
                    //
                    //	send response
                    //
                    socket->writeMessageRaw(serverResponse);
                }

                // Let the socket live free
                disconnect(socket, SIGNAL(readReady()), this, SLOT(challengeResponseReady()));
            }

            // Kill any pending authentications that are taking too long
            void TCPServer::sweepPendingAuths()
            {
                PendingAuthHash::Iterator it = m_pendingAuths.begin();

                // Any auths started before this time should be removed
                QDateTime removeBefore = QDateTime::currentDateTime().addSecs(-clientAuthTimeout);

                while(it != m_pendingAuths.end())
                {
                    if (it.value().connectTime < removeBefore)
                    {
                        // Kill the socket
                        delete it.key();

                        // Remove us from the pending auth list
                        it = m_pendingAuths.erase(it);
                    }
                    else
                    {
                        it++;
                    }
                }

                if (m_pendingAuths.isEmpty())
                {
                    // No need to sweep
                    m_pendingAuthSweepTimer->stop();
                }
            }


            void TCPServer::setAuthDelegate(LsxAuthDelegate *delegate)
            {
                m_authDelegate = delegate;
            }
        }
    }
}