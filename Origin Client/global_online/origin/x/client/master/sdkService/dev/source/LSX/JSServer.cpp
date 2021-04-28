///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: JSServer.cpp
//	Generic WebSocket server
//	
//	Author: Hans van Veenendaal

#include "LSX/JSServer.h"
#include "LSX/WebSocket.h"
#include <QTcpSocket>
#include <QTimer>

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            JSServer::JSServer(QObject *parent) : QTcpServer(parent), m_authDelegate(NULL)
            {
                connect(this, SIGNAL(newConnection()), this, SLOT(newConnectionPending()));
            }

            JSServer::~JSServer(void)
            {
            }

            void JSServer::setAuthDelegate(LsxAuthDelegate *delegate)
            {
                m_authDelegate = delegate;
            }

            void JSServer::newConnectionPending()
            {
                // Not configured yet.
                if(!m_authDelegate)
                    return;

                QTcpSocket *socket;
                while((socket = nextPendingConnection()))
                {
                    WebSocket *s = new WebSocket(socket);

                    connect(s, SIGNAL(readReady()), this, SLOT(readReady()));
                }
            }

            void JSServer::readReady()
            {
                WebSocket *socket = dynamic_cast<WebSocket*>(sender());
                QByteArray response;
                
                if(!m_authDelegate->authenticateClient(socket->readMessage(), "",  response, socket))
                {
                    socket->close();
                }
                else
                {
                    socket->writeMessage(response);

                    // Connection Established, we are no longer listening to this socket.
                    disconnect(socket, SIGNAL(readReady()), this, SLOT(readReady()));
                }
            }
        }
    }
}