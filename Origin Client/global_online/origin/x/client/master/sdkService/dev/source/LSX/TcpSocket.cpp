#include <LSX/TCPSocket.h>
#include <services/debug/DebugService.h>
#include <services/log/LogService.h>

Origin::SDK::Lsx::TcpSocket::TcpSocket( QTcpSocket * socket) : 
    m_socket(socket),
    m_encryptor(NULL),
    m_bIncompleteMessage(false)
{
    ORIGIN_VERIFY_CONNECT_EX(m_socket, SIGNAL(readyRead()), this, SIGNAL(readReady()), Qt::QueuedConnection);
    ORIGIN_VERIFY_CONNECT(m_socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
}

Origin::SDK::Lsx::TcpSocket::~TcpSocket()
{
    m_socket->deleteLater();
}

QByteArray Origin::SDK::Lsx::TcpSocket::readMessage()
{
    // xxxxxxxx0 ==> xxxxxxxx, <>
    // xxxxxx0xxxxxxxxxx0 ==> xxxxxx, xxxxxxxxxx, <>
    // xxxx ==> xxxx
    // xxxxx0xxxxxxxx ==> xxxxx, xxxxxxxx
    // xxxxx0xxxxxxxx0xxxxx ==> xxxxx, xxxxxxxx, xxxxx

    QList<QByteArray> messages = m_socket->readAll().split('\0');

    while(messages.size() > 0)
    {
        bool isNullTerminatedMessage = messages.size() > 1;
        bool isTerminator = messages.size() == 1 && messages[0].length() == 0;

        if(!isTerminator)
        {
            QByteArray msg;

            if(m_bIncompleteMessage)
            {
                msg = m_messages.back() + messages.first();
                m_messages.pop_back();
            }
            else
            {
                msg = messages.first();
            }

            m_bIncompleteMessage = !isNullTerminatedMessage;

            if(m_encryptor)
                msg = m_encryptor->decrypt(msg, m_seed);

            // Only insert if the message is valid
            if(m_bIncompleteMessage || msg[0] == '<')
                m_messages.push_back(msg);
        }

        messages.pop_front();
    }

    if(m_messages.size() > 0 && !m_bIncompleteMessage || m_messages.size() > 1)
    {
        QByteArray msg = m_messages[0];
        m_messages.pop_front();

        if(m_messages.size() > 0)
            emit readReady();

        ORIGIN_LOG_DEBUG << "(TCP) Receiving request: " << msg.data();

        return msg;
    }

    return "";
}

bool Origin::SDK::Lsx::TcpSocket::writeMessage( const QByteArray &data )
{
    ORIGIN_LOG_DEBUG << "(TCP) Sending response: " << data.data();

    if(m_encryptor)
    {
        QByteArray response = m_encryptor->encrypt(data, m_seed);
        response.append('\0');
        
        return m_socket->write(response) == response.length();
    }
    else
    {
        QByteArray response = data;
        response.append('\0');

        return m_socket->write(response) == response.length();
    }
}

bool Origin::SDK::Lsx::TcpSocket::writeMessageRaw( const QByteArray &data )
{
    // This function is here because it would take a lot more rewriting to make this work without it...
    // TODO: remove this function.

    QByteArray response = data;
    response.append('\0');

    return m_socket->write(response) == response.length();
}

void Origin::SDK::Lsx::TcpSocket::close()
{
    m_socket->disconnectFromHost();
}

void Origin::SDK::Lsx::TcpSocket::setEncryptor( Encryptor * encryptor, int seed )
{
    m_encryptor = encryptor;
    m_seed = seed;
}
