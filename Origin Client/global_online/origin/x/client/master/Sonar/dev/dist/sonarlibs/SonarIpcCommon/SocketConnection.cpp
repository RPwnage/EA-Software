#include <SonarCommon/Common.h>
#include <SonarIpcCommon/SocketConnection.h>
#include <SonarIpcCommon/IpcMessage.h>

namespace sonar
{

SocketConnection::SocketConnection(void)
	: m_rxBuffer(BUFFER_SIZE)
	, m_txBuffer(BUFFER_SIZE)
	, m_sockfd( (jlib::SOCKET) -1)
	, m_idCounter(0)
{

}

SocketConnection::~SocketConnection(void)
{
	jlib::SocketClose(m_sockfd);
}

void SocketConnection::setSocket(jlib::SOCKET sockfd)
{
	m_sockfd = sockfd;
}

void SocketConnection::processData(const char *buffer, size_t length)
{
	String utfStr(buffer, length);
	size_t pos = utfStr.rfind('\n');
	if (pos != String::npos)
	{
		utfStr = utfStr.substr(0, pos);
	}
	
	String cmd = utfStr;

	TokenList list = common::tokenize(cmd, '\t');

	if (list.size() < 2)
	{
		return;
	}

	TokenList::iterator iter = list.begin();

	String &msgId = *iter++;
	String &msgName = *iter++;

	IpcMessage msg(msgId, msgName);
	
	IpcMessage::Arguments args(iter, list.end());

	msg.setArgument(args);
	onMessage(msg);
}


bool SocketConnection::update()
{
	if (m_sockfd == -1)
	{
		return true;
	}

	// Rx Path
	for(;;)
	{
		int result = ::recv(m_sockfd, (char *) m_rxBuffer.getPushPtr(), (size_t) (m_rxBuffer.getEndPtr() - m_rxBuffer.getPushPtr()), jlib::MSG_NOSIGNAL);

		if (result == -1)
		{
			if (!jlib::SocketWouldBlock(result))
			{
				return false;
			}

			break;
		}

		if (result == 0)
		{
			return false;
		}

		m_rxBuffer.push(result);

		size_t popLength;

		char buffer[COMMAND_SIZE];

		while ( (popLength = m_rxBuffer.popUntilChar(buffer, COMMAND_SIZE, '\n')))
		{
			processData(buffer, popLength);
		}
	}

	// Tx path

	for (;;)
	{
		size_t bytesToSend = m_txBuffer.getPushPtr() - m_txBuffer.getPopPtr();

		if (bytesToSend == 0)
		{
			break;
		}

		int result = ::send(m_sockfd, (char *) m_txBuffer.getPopPtr(), bytesToSend, jlib::MSG_NOSIGNAL);

		if (result == -1)
		{
			if (!jlib::SocketWouldBlock(result))
			{
				return false;
			}

			break;
		}

		if (result == 0)
		{
			return false;
		}

		m_txBuffer.pop(result);
	}

	return true;
}

jlib::SOCKET SocketConnection::getSocket(void)
{
	return m_sockfd;
}


bool SocketConnection::hasTxBytes(void)
{
	return (m_txBuffer.getPushPtr() - m_txBuffer.getPopPtr()) > 0;
}


void SocketConnection::send(const IpcMessage &msg)
{
	ByteString bs = msg.toByteString();
	m_txBuffer.push(bs.c_str(), bs.size());
}


void SocketConnection::sendReply(const IpcMessage &request, IpcMessage &reply)
{
	reply.setId(request.getId());
	reply.setName("reply");

	ByteString bs = reply.toByteString();
	m_txBuffer.push(bs.c_str(), bs.size());
}


CString SocketConnection::generateId()
{
	StringStream ss;
	ss << m_idCounter++;
	return ss.str();
}


}