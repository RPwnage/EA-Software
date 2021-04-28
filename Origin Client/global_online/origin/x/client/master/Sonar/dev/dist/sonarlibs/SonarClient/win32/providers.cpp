#include <SonarCommon/Common.h>
#include <stdlib.h>
#include "providers.h"
#include <SonarConnection/Connection.h>
#include <iostream>

#ifdef _WIN32
#define USE_WSARECVEX 0
#endif

#ifndef _WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#endif

namespace sonar
{

common::Timestamp DefaultTimeProvider::getMSECTime() const
{
	return common::getTimeAsMSEC();
}

int Udp4Transport::m_txPacketLoss = 0;
int Udp4Transport::m_rxPacketLoss = 0;


struct Udp4Transport::Private
{
	SOCKET m_sockfd;
	struct sockaddr_in m_remoteAddr;
};

Udp4Transport::Udp4Transport(sonar::CString &address)
{
	m_prv = new struct Private;
	m_prv->m_sockfd = socket(AF_INET, SOCK_DGRAM, 0);

	int size = (32768);

	setsockopt (m_prv->m_sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &size, sizeof(size));
	setsockopt (m_prv->m_sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &size, sizeof(size));

    String host = address.substr(0, address.find(L':'));
    String service = address.substr(address.find(L':') + 1);

	struct addrinfo hints;
	struct addrinfo *pResults;

	memset (&hints, 0, sizeof (hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_protocol = IPPROTO_UDP;


	if (::getaddrinfo(host.c_str(), service.c_str(), &hints, &pResults) != 0)
	{
		return;
	}

	memcpy (&m_prv->m_remoteAddr, pResults->ai_addr, sizeof (sockaddr_in));

	freeaddrinfo (pResults);

	struct sockaddr_in bindAddr;
	memcpy (&bindAddr, &m_prv->m_remoteAddr, sizeof(bindAddr));

	bindAddr.sin_addr.s_addr = INADDR_ANY;
	bindAddr.sin_port = htons(0);

	bind(m_prv->m_sockfd, (sockaddr *) &bindAddr, sizeof(bindAddr));
	connect(m_prv->m_sockfd, (sockaddr *) &m_prv->m_remoteAddr, sizeof(m_prv->m_remoteAddr));		

    socklen_t len = sizeof (m_addr_local);
    if (getsockname (m_prv->m_sockfd, (struct sockaddr *) &m_addr_local, &len) != -1)
    {
#ifdef _WIN32
        int err = WSAGetLastError();
#else
        int err = errno;
#endif
        SYSLOG(LOG_DEBUG, "Udp4Transport: getsockname, err=%d", err);
    }
}

Udp4Transport::~Udp4Transport(void)
{
	close();
	delete m_prv;
}


void Udp4Transport::close()
{
    if (!isClosed())
    {
        common::SocketClose(m_prv->m_sockfd);
        m_prv->m_sockfd = -1;
    }
}

bool Udp4Transport::isClosed() const
{
    return m_prv->m_sockfd == -1;
}

bool Udp4Transport::rxMessagePoll(void *buffer, size_t *cbBuffer, ssize_t& socketResult)
{
	struct sockaddr_in addr;
	sonar::socklen_t len = sizeof(addr); 

#if USE_WSARECVEX

    // WSARecvEx doc: http://msdn.microsoft.com/en-us/library/windows/desktop/ms741684(v=vs.85).aspx
    char* b = reinterpret_cast<char*>(buffer);
    int b_len = *cbBuffer;

    int flags;
    do {
        flags = 0;
        socketResult = WSARecvEx(m_prv->m_sockfd, b, b_len,  &flags);

        if ((flags & MSG_PARTIAL) != 0)
            printf("received partial message, %d, %d\n", socketResult, b_len);

        if (socketResult > 0) {
            b += b_len;
            b_len -= socketResult;
        }
    }
    while (socketResult >= 0 && ((flags & MSG_PARTIAL) != 0) && b_len > 0);

    if (b_len == 0) {
        printf("ran out of buffer space");
    }

#else
	socketResult = recvfrom(m_prv->m_sockfd, (char *) buffer, *cbBuffer, common::DEF_MSG_NOSIGNAL, (struct sockaddr *) &addr, &len);
#endif

	switch(socketResult)
	{
#ifdef _WIN32
	case SOCKET_ERROR:
#else
	case -1:
#endif
    {
#ifdef _WIN32
        const int err = WSAGetLastError();
        const int blockError = WSAEWOULDBLOCK;
#else
        const int err = errno;
        const int blockError = EWOULDBLOCK;
#endif
		if (err != blockError)
		{
			sonar::common::Log("sonar: recvfrom err=%d", err);
			return false;
		}

		*cbBuffer = 0;
		return true;
    }

	case 0:
		return false;

	default:

#if !USE_WSARECVEX
		if (addr.sin_addr.s_addr != m_prv->m_remoteAddr.sin_addr.s_addr ||
			addr.sin_port != m_prv->m_remoteAddr.sin_port)
		{
			return false;
		}
#endif

		if (m_rxPacketLoss > 0 && ((rand () % 100) + 1) < m_rxPacketLoss)
		{
			return false;
		}
		
#if USE_WSARECVEX
        *cbBuffer -= b_len;
#else
       *cbBuffer = (size_t) socketResult;
#endif
		return true;
	}
}
bool Udp4Transport::rxMessageWait(void)
{
	struct timeval tv;
	tv.tv_sec = 1;
	tv.tv_usec = 0;

	fd_set readSet;
	FD_ZERO(&readSet);
	FD_SET(m_prv->m_sockfd, &readSet);
	int result = select(m_prv->m_sockfd + 1, &readSet, NULL, NULL, &tv);
	return (result == 1);
}

bool Udp4Transport::txMessageSend(const void *buffer, size_t cbBuffer, ssize_t& socketResult)
{
	if (m_txPacketLoss > 0 && ((rand () % 100) + 1) < m_txPacketLoss)
	{
		return false;
	}

	socketResult = sendto(m_prv->m_sockfd, (char *) buffer, cbBuffer, common::DEF_MSG_NOSIGNAL, (sockaddr *) &m_prv->m_remoteAddr, sizeof(m_prv->m_remoteAddr));
	if (socketResult < 0) {
		const sockaddr_in addr = m_prv->m_remoteAddr;
#ifdef _WIN32
		int err = WSAGetLastError();
#else
		int err = errno;
#endif
		sonar::common::Log("sonar: udp4 sendto err=%d, addr={%04X, %04X, %08X}, len=%d", err, addr.sin_family, addr.sin_port, addr.sin_addr.s_addr, cbBuffer);
	}
	return (socketResult == (int) cbBuffer);
}

sockaddr_in Udp4Transport::getLocalAddress() const
{
    return m_addr_local;
}

sockaddr_in Udp4Transport::getRemoteAddress() const
{
    return m_prv->m_remoteAddr;
}

void StderrLogger::printf(const char *format, ...) const
{
	char buffer[4096 + 1];
	va_list ap;
	va_start (ap, format);
	vsnprintf (buffer, 4096, format, ap);
	va_end (ap);
    std::cerr << buffer;
}

sonar::AbstractTransport *Udp4NetworkProvider::connect(sonar::CString &address) const
{
	Udp4Transport *ret = new Udp4Transport(address);
	return ret;
}

}
