#include <TestUtils/BackendConnection.h>
#include <TestUtils/BackendEvent.h>
#include <assert.h>

namespace sonar
{

BackendConnection::BackendConnection(CString &pubRsaBase64)
	: m_isRegistered(false)
	, m_pubRsaBase64(pubRsaBase64)
	, m_firstValidation(true)
	, m_ignoreKeepalives(true)
{
	m_acceptfd = ::socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in bindAddr;
	memset(&bindAddr, 0, sizeof(bindAddr));

	bindAddr.sin_family = AF_INET;
	bindAddr.sin_port = htons(0);
	bindAddr.sin_addr.s_addr = INADDR_ANY;

	if (::bind(m_acceptfd, (sockaddr *) &bindAddr, sizeof(bindAddr)) == -1)
	{
		throw -1;
	}

	socklen_t len = sizeof (bindAddr);
	if (::getsockname (m_acceptfd, (struct sockaddr *) &bindAddr, &len) == -1)
	{
		throw -1;
	}

	::listen(m_acceptfd, 5);

	m_port = ntohs(bindAddr.sin_port);
}

int BackendConnection::getPort(void)
{
	return m_port;
}


BackendConnection::~BackendConnection()
{
	if (!m_events.empty())
	{
		throw EventNotValidated();
	}

	jlib::SocketClose(m_acceptfd);
}

void BackendConnection::setKeepaliveIgnore(bool state)
{
	m_ignoreKeepalives = state;
}


void BackendConnection::waitForConnection()
{
	struct sockaddr_in remoteAddr;
	int len = sizeof(remoteAddr);
	jlib::SOCKET newfd = ::accept(m_acceptfd, (sockaddr *) &remoteAddr, &len);

	assert(newfd != -1);
	jlib::SocketSetNonBlock(newfd, true);

	setSocket(newfd);
}

void BackendConnection::update(void)
{
	SocketConnection::update();

}
void BackendConnection::handleRegister(IpcMessage &msg)
{
	assert (!m_isRegistered);
	m_isRegistered = true;
	m_guid = msg.getString(0);

	IpcMessage reply;
	reply.writeString("OK");
	reply.writeString("RSA");
	reply.writeString("X.509");
	reply.writeString(m_pubRsaBase64);
	this->sendReply(msg, reply);
}

void BackendConnection::handleKeepalive(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeString("SUCCESS");
	this->sendReply(msg, reply);

	if (!m_ignoreKeepalives)
	{
		jlib::Spinlocker locker(m_lock);
		m_events.push_back(new BackendEvent(msg, KEEPALIVE));
	}
}

void BackendConnection::handleClientRegisteredToChannel(IpcMessage &msg)
{
	jlib::Spinlocker locker(m_lock);
	m_events.push_back(new BackendEvent(msg, CLIENT_REGISTERED));
}

void BackendConnection::handleClientUnregistered(IpcMessage &msg)
{
	jlib::Spinlocker locker(m_lock);
	m_events.push_back(new BackendEvent(msg, CLIENT_UNREGISTERED));
}

void BackendConnection::handleChannelDestroyed(IpcMessage &msg)
{
	jlib::Spinlocker locker(m_lock);
	m_events.push_back(new BackendEvent(msg, CHANNEL_DESTROYED));
}

void BackendConnection::onMessage(IpcMessage &msg)
{
	if (msg.getName() == "REGISTER")
	{
		handleRegister(msg);
	}
	else
	if (msg.getName() == "KEEPALIVE")
	{
		handleKeepalive(msg);
	}
	else
	if (msg.getName() == "EVENT_CLIENT_REGISTERED_TOCHANNEL")
	{
		handleClientRegisteredToChannel(msg);
	}
	else
	if (msg.getName() == "EVENT_CLIENT_UNREGISTERED")
	{
		handleClientUnregistered(msg);
	}
	else
	if (msg.getName() == "EVENT_CHANNEL_DESTROYED")
	{
		handleChannelDestroyed(msg);
	}
	else
	{
		assert(false);
		throw UnknownCommand();

	}
}

void BackendConnection::validateNextEvent(EventType type, BackendEvent **outEvent)
{
	if (m_firstValidation)
	{
		common::sleepExact(1000);
		m_firstValidation = false;
	}

	jlib::Spinlocker locker(m_lock);

	if (m_events.front()->type != type)
	{
		throw UnexpectedEvent();
	}

	if (outEvent == NULL)
		delete m_events.front();
	else
		*outEvent = m_events.front();

	m_events.pop_front();
}

CString &BackendConnection::getGuid()
{
	return m_guid;
}



}