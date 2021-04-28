#ifndef __TCP_SOCKET_H__
#define __TCP_SOCKET_H__

#include <QTcpSocket>
#include <LSX/LsxSocket.h>

// QTcpWrapper for 

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            class TcpSocket : public LsxSocket
            {
                Q_OBJECT
            public:
                TcpSocket(QTcpSocket *);
                virtual ~TcpSocket();

                virtual QByteArray readMessage();
                virtual bool writeMessage(const QByteArray &data);
                virtual bool writeMessageRaw(const QByteArray &data);
                virtual void close();

                virtual Protocol protocol(){return PROTOCOL_XML;}

                void setEncryptor(Encryptor * encryptor, int seed);

            private slots:


            private:
                QTcpSocket * m_socket;
                Encryptor * m_encryptor;
                int m_seed;

                QList<QByteArray> m_messages;
                bool m_bIncompleteMessage;
            };
        }
    }
}

#endif //__TCP_SOCKET_H__