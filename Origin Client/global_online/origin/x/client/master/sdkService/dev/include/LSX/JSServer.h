///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2008 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: JSServer.h 
//	Generic TCP server
//	
//	Author: Sergey Zavadski

#ifndef _JS_SERVER_H_
#define _JS_SERVER_H_

#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QThread>
#include <QHash>
#include <QTcpServer>

#include "LSX/LsxSocket.h"

class QTcpSocket;
class QTimer;

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            class JSServer : public QTcpServer
            {
                Q_OBJECT
            public:
                JSServer(QObject *parent);

                virtual ~JSServer();

                void setAuthDelegate(LsxAuthDelegate *delegate);

            protected slots:
                void newConnectionPending();
                void readReady();
                
            private:
                void closeConnection( QTcpSocket* connection );
                
            private:
                LsxAuthDelegate * m_authDelegate;
            };
        }
    }
}

#endif //_JS_SERVER_H_