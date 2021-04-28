#include "LSX/WebSocket.h"
#include <QCryptographicHash>
#include <services/debug/DebugService.h>
#include <services/settings/SettingsManager.h>
#include <services/log/LogService.h>

#ifndef __APPLE__
#include <WinSock2.h>
#else
#include <netinet/in.h>
#endif

Origin::SDK::Lsx::WebSocket::WebSocket( QTcpSocket * socket) : m_connected(false)
{
    m_socket = socket;

    ORIGIN_VERIFY_CONNECT(m_socket, SIGNAL(readyRead()), this, SLOT(onDataReady()));
    ORIGIN_VERIFY_CONNECT(m_socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
}

Origin::SDK::Lsx::WebSocket::~WebSocket()
{

}

QByteArray Origin::SDK::Lsx::WebSocket::readMessage()
{
    if(m_messages.size()>0)
    {
        QByteArray msg = m_messages[0];
        m_messages.pop_front();

        ORIGIN_LOG_DEBUG << "(WS) Receiving request: " << msg.data();
        return msg;
    }
    else
    {
        return "";
    }
}

bool Origin::SDK::Lsx::WebSocket::writeMessage( const QByteArray &data )
{
    ORIGIN_LOG_DEBUG << "(WS) Sending response: " << data.data();

    QByteArray msg;

    msg.reserve(data.length() + 9);

    msg.push_back('\x81');

    if(data.length()<126)
    {
        msg.push_back((char)data.length());
    } 
    else if(data.length()<65536)
    {
        msg.push_back(126);

        union{short s; char c[2];} cvrt;

        cvrt.s = data.length();

        msg.push_back(cvrt.c[1]);
        msg.push_back(cvrt.c[0]);
    } 
    else
    {
        msg.push_back(127);

        union{long long ll; char c[8];} cvrt;

        cvrt.ll = data.length();

        msg.push_back(cvrt.c[7]);
        msg.push_back(cvrt.c[6]);
        msg.push_back(cvrt.c[5]);
        msg.push_back(cvrt.c[4]);
        msg.push_back(cvrt.c[3]);
        msg.push_back(cvrt.c[2]);
        msg.push_back(cvrt.c[1]);
        msg.push_back(cvrt.c[0]);
    }

    msg.push_back(data);

    m_socket->write(msg);

    return true;
}

bool Origin::SDK::Lsx::WebSocket::writeMessageRaw( const QByteArray &data )
{
    return writeMessage(data);
}

void Origin::SDK::Lsx::WebSocket::close()
{
    m_socket->close();
}

void Origin::SDK::Lsx::WebSocket::setEncryptor( Encryptor * encryptor, int seed /*= 0*/ )
{
    // Ignore this.
}

void Origin::SDK::Lsx::WebSocket::onDataReady()
{
    if(!m_connected)
    {
        handleUpgrade();
    }
    else
    {
        processMessages();
    }
}

void Origin::SDK::Lsx::WebSocket::handleUpgrade()
{
    QByteArray response = constructResponse(m_socket->readAll());

    if(!response.isEmpty())
    {
        m_socket->write(response);
        m_connected = true;
    }
    else
    {
        m_socket->close();
        deleteLater();
    }
}

void Origin::SDK::Lsx::WebSocket::processMessages()
{
    while(m_socket->bytesAvailable()>=2)
    {
        QByteArray header = m_socket->read(2);

        if(header.length() == 2)
        {
            bool finalFrame = (header[0] & 0x80) != 0;
            char opcode = header[0] & 0xF;

            bool textFrame = false;
            bool binaryFrame = false;

            switch(opcode)
            {
            case 0:
            case 1:
                textFrame = true;
                break;
            case 2:
                binaryFrame = true;
                break;
            case 8:
            default:
                m_socket->close();
                return;
            }

            bool masked = (header[1] & 0x80) != 0;
            qint64 frameLength = header[1] & 0x7F;

            if(frameLength == 126)
            {
                frameLength = ntohs(*(short *)m_socket->read(2).data());
            }
            else if(frameLength == 127)
            {
                frameLength = ((qint64)ntohl(*(long *)m_socket->read(4).data()) << 32)  + (qint64)ntohl(*(long *)m_socket->read(4).data());    
            }

            if(frameLength>0)
            {
                QByteArray mask = masked ? m_socket->read(4).data() : "";
                QByteArray payload = m_socket->read(frameLength);

                // Make sure we got all our data.
                if(mask.length() == 4 && payload.length() == frameLength)
                {
                    if(masked)unmask(payload, mask);
                    m_messages.push_back(payload);
                    emit readReady();
                }
            }
        }
    }
}

void Origin::SDK::Lsx::WebSocket::unmask( QByteArray &payload, const QByteArray & mask )
{
    for(int i=0; i<payload.length(); i++)
    {
        payload[i] = payload[i] ^ mask[i&3]; 
    }
}

QByteArray Origin::SDK::Lsx::WebSocket::constructResponse( QByteArray requestHeader )
{
    QList<QByteArray> lines = requestHeader.split('\n');

    if(lines.size()==0)
        return QByteArray();

    QByteArray signature = "GET /origin/" + Origin::Services::readSetting(Origin::Services::SETTING_OriginSessionKey).toString().toUtf8();

    if(!QUrl::fromPercentEncoding(lines[0]).startsWith(signature))
        return QByteArray();

    QByteArray key;

    for(int i=1; i<lines.size(); i++)
    {
        if(lines[i].startsWith("Sec-WebSocket-Key:"))
        {
            key = lines[i].split(' ')[1];
            break;
        }
    }

    key = key.trimmed();
    key.append("258EAFA5-E914-47DA-95CA-C5AB0DC85B11");

    QCryptographicHash hash(QCryptographicHash::Sha1);

    hash.addData(key);

    QByteArray response =  "HTTP/1.1 101 Switching Protocols\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        "Sec-WebSocket-Accept: ";

    response.append(hash.result().toBase64());
    response.append("\r\n\r\n");

    return response;
}

void Origin::SDK::Lsx::WebSocket::closeConnection()
{
	if (m_socket != NULL)
	{
		QTcpSocket * temp = m_socket;
		m_socket = NULL;
		temp->close();
	}
}


