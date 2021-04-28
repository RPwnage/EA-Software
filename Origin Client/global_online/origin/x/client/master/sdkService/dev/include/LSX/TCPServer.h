///////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2008 Electronic Arts. All rights reserved.
//
///////////////////////////////////////////////////////////////////////////////
//
//  File: TCPServer.h 
//	Generic TCP server
//	
//	Author: Sergey Zavadski

#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_

#include <QByteArray>
#include <QDateTime>
#include <QMutex>
#include <QMutexLocker>
#include <QObject>
#include <QThread>
#include <QHash>
#include <QTcpServer>

#include <LSX/TCPSocket.h>

class QTimer;

namespace Origin
{
	namespace SDK
	{
		namespace Lsx
		{
			class TCPServer : public QTcpServer
			{
				Q_OBJECT
			public:
				TCPServer(QObject *parent);

				virtual ~TCPServer();

				void setAuthDelegate(LsxAuthDelegate *delegate);

            protected slots:
                void newConnectionPending();
                void challengeResponseReady();
                void sweepPendingAuths();

			private:
				void closeConnection( TcpSocket * connection );

				struct PendingAuth
				{
					QDateTime connectTime;
					QByteArray key;
				};

				typedef QHash<TcpSocket *, PendingAuth> PendingAuthHash;
				PendingAuthHash m_pendingAuths;

				QTimer *m_pendingAuthSweepTimer;
				LsxAuthDelegate * m_authDelegate;
			};
		}
	}
}

#endif //_TCP_SERVER_H_