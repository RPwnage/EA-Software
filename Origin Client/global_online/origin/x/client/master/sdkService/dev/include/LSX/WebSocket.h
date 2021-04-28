#ifndef __WEB_SOCKET_H__
#define __WEB_SOCKET_H__

#include <QTcpSocket>
#include <LSX/LsxSocket.h>

// QTcpWrapper for 

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            class WebSocket : public LsxSocket
            {
                Q_OBJECT
            public:
                WebSocket(QTcpSocket *);
                virtual ~WebSocket();

                virtual QByteArray readMessage();
                virtual bool writeMessage(const QByteArray &data);
                virtual bool writeMessageRaw(const QByteArray &data);
                virtual void close();

                virtual Protocol protocol(){return PROTOCOL_JSON;}

                virtual void setEncryptor(Encryptor * encryptor, int seed = 0);

				virtual void closeConnection();
            private slots:
                void onDataReady();

            private:
                void handleUpgrade();
                void processMessages();
                QByteArray constructResponse( QByteArray requestHeader );
                void unmask( QByteArray &payload, const QByteArray &mask );
            private:
                QTcpSocket * m_socket;
                bool m_connected;
                QList<QByteArray> m_messages;
            };
        }
    }
}

#endif //__WEB_SOCKET_H__