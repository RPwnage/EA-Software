#ifndef _LSXTCPCONNECTION
#define _LSXTCPCONNECTION

#include <QByteArray>
#include "LsxConnection.h"
#include "LsxEncryptor.h"
#include "LSX/TCPSocket.h"

class QTcpSocket;

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            class Server;

            class TcpConnection : public Connection
            {
                Q_OBJECT
            public:
                TcpConnection(Server *server, LsxSocket *socket);

                virtual void sendResponse(const Response& response);

                Protocol protocol(){return m_socket->protocol();}

				virtual void closeConnection();

            protected slots:
                // Writes to the socket in our owner thread
                void socketWrite(const QByteArray &);

            protected:
                Server		*m_server;
                LsxSocket	*m_socket;
            };
        }
    }
}

#endif