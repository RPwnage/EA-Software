#ifndef __LSX_SOCKET_H__
#define __LSX_SOCKET_H__

#include <QObject>
#include <QByteArray>

#include "LSX/LsxEncryptor.h"

namespace Origin
{
    namespace SDK
    {
        namespace Lsx
        {
            enum Protocol
            {
                PROTOCOL_UNKNOWN,
                PROTOCOL_XML,
                PROTOCOL_JSON,
            };

            class LsxSocket : public QObject
            {
                Q_OBJECT
            public:
                virtual QByteArray readMessage() = 0;
                virtual bool writeMessage(const QByteArray &data) = 0;
                virtual bool writeMessageRaw(const QByteArray &data) = 0;
                virtual void close() = 0;

                virtual Protocol protocol() = 0;

                virtual void setEncryptor(Encryptor * encryptor, int seed = 0) = 0;

            signals:
                void readReady();
                void disconnected();
            };

            class LsxAuthDelegate
            {
            public:
                virtual QByteArray clientAuthenticationChallenge(QByteArray &key) = 0;
                virtual bool authenticateClient(const QByteArray& challengeResponse, const QByteArray& challengeKey, QByteArray& response, LsxSocket *socket) = 0;
            };
        }
    }
}




#endif //__LSX_SOCKET_H__