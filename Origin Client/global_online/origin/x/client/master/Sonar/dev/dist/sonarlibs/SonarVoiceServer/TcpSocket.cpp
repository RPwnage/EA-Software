#include "TcpSocket.h"
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>

namespace sonar
{

#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0
#endif

TcpSocket::TcpSocket (SOCKET _inSocket, struct sockaddr_in *_pSockaddr)
{
	m_sockfd = _inSocket;
	memcpy (&m_sockaddr, _pSockaddr, sizeof (struct sockaddr_in));
	setupQueue ();
}

int TcpSocket::sendf (const char *format, ...)
{
  char buffer[4096 + 1];
	int length;

  va_list valist;
  va_start (valist, format);
    
  if ( (length = vsnprintf ( (char *) buffer, 4096, format, valist)) == -1)
  {
    return -1;
  }

  va_end (valist);


	int bytesToSend = 0;
	int bytesLeft = length;
	int bytesSent = 0;

	while (bytesLeft > 0)
	{
		if (bytesLeft > 4096)
			bytesToSend = 4096;
		else
			bytesToSend = bytesLeft;

		int sendResult = ::send (m_sockfd, buffer + bytesSent, bytesToSend, MSG_NOSIGNAL);

		if (sendResult < 1)
		{
			return -1;
		}

		bytesSent += sendResult;
		bytesLeft -= sendResult;
	}

	return 0;
}


TcpSocket::TcpSocket (void)
{
	setupQueue ();
	if ( (m_sockfd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
	{
		//FIXME: Handle error here 
		throw -1;
	}
}

TcpSocket::TcpSocket (SOCKET _inSocket)
{
	m_sockfd = _inSocket;
	setupQueue ();
}


TcpSocket::~TcpSocket (void)
{
	this->close ();
	this->destroySendQueue ();
}


int TcpSocket::send (void *_pBuffer, int _cbBuffer)
{
	return ::send (this->m_sockfd, (char *) _pBuffer, _cbBuffer, MSG_NOSIGNAL);
}

int TcpSocket::recv (void *_pBuffer, int _cbMaxBuffer)
{
	return ::recv (m_sockfd, (char *) _pBuffer, _cbMaxBuffer, MSG_NOSIGNAL);
}

int TcpSocket::bind (const char *_addrString, int _port)
{
	struct sockaddr_in m_sockaddr;
 
	// Bind socket to port
	if (strcmp (_addrString, "ANY") != 0)
		m_sockaddr.sin_addr.s_addr = INADDR_ANY;
	else
		m_sockaddr.sin_addr.s_addr = INADDR_ANY;

	m_sockaddr.sin_family = AF_INET;
	m_sockaddr.sin_port = htons ( (unsigned short) _port);
	memset (m_sockaddr.sin_zero, 0, 8);

	if (::bind (m_sockfd, (struct sockaddr *) &m_sockaddr, sizeof (struct sockaddr)) == -1)
	{
        SYSLOGX(LOG_ERR, "err=%d", errno);
        return -1;
    }

    if (listen (m_sockfd, 1024) == -1)
    {
        SYSLOGX(LOG_ERR, "err=%d", errno);
		return -1;
	}

	return 0;
}

TcpSocket *TcpSocket::accept (void)
{
	TcpSocket *pReturnSocket;
	SOCKET tempSocket;

	struct sockaddr_in remoteAddr;

	sonar::socklen_t sinSize = sizeof (struct sockaddr_in);

	if ( (tempSocket = ::accept (m_sockfd, (sockaddr*)&remoteAddr,
		&sinSize)) == -1) 
	{
        SYSLOGX(LOG_ERR, "err=%d", errno);
		return NULL;
	}

	pReturnSocket = new TcpSocket (tempSocket, &remoteAddr);
  
	return (TcpSocket *) pReturnSocket;
}

int TcpSocket::connect (const sockaddr_in &_addr)
{
	m_sockaddr = _addr;

	int result = ::connect (m_sockfd, (struct sockaddr *) &m_sockaddr, sizeof (struct sockaddr));
    if (result == -1) {
        SYSLOGX(LOG_ERR, "err=%d", errno);
    }
    return result;
}


int TcpSocket::connect (const char *_addrString, int _port)
{
	struct sockaddr_in addr;

	if (strcmp (_addrString, "ALL") == 0) 
	{
		addr.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		addr.sin_addr.s_addr = inet_addr (_addrString);
	}

	addr.sin_family = AF_INET;
	addr.sin_port = htons ( (unsigned short) _port);
	memset (addr.sin_zero, 0, 8);

	return connect (addr);
}	


int TcpSocket::close (void)
{
#ifdef _WIN32
	closesocket (m_sockfd);	
#else

	::close (m_sockfd);	
#endif
	return 0;
}

int TcpSocket::sendQueue (void *_pData, int _cbLength)
{
	memcpy (m_pBuffer + m_iBuffer, _pData, _cbLength);
	m_iBuffer += _cbLength;	

	for (;;)
	{
		int sendLength = m_iBuffer;

		if (sendLength == 0)
		{
			return 0;
		}
	
		if (sendLength > 4096)
		{
			sendLength = 4096;
		}

		int sendResult = ::send (m_sockfd, m_pBuffer, sendLength, MSG_NOSIGNAL);

		if (sendResult < 1)
		{
			return -1;
		}

		memmove (m_pBuffer, m_pBuffer + sendResult, (m_iBuffer - sendLength));
		m_iBuffer -= sendLength;
	}
}

bool TcpSocket::wouldBlock (void)
{
	#ifndef _WIN32	
	if (errno == EWOULDBLOCK)
	#else	
	if (WSAGetLastError () == WSAEWOULDBLOCK)
	#endif
	
	{
		return true;
	}	

	return false;
}

int TcpSocket::setNonBlocking (bool _state)
{
#ifdef _WIN32
	unsigned long flags = _state ? 1 : 0;
	int result;

	result = ioctlsocket (m_sockfd, FIONBIO, &flags);

	if (result != 0)
	{
		return -1;
	}

	return 0;

#else
	if (_state)
	{
		if (fcntl (m_sockfd, F_SETFL, O_NONBLOCK) == -1)
		{
			return -1;
		}
	}
	else
	{
		if (fcntl (m_sockfd, F_SETFL, 0) == -1)
		{
			return -1;
		}

	}

	return 0;
#endif
}

SOCKET TcpSocket::getSocket (void)
{
	return m_sockfd;
}

struct sockaddr_in *TcpSocket::getSockaddr (void)
{
	return &m_sockaddr;
}

void TcpSocket::setupQueue (void)
{
	m_cbBuffer = 65536;
	m_pBuffer = new char[m_cbBuffer];
	m_iBuffer = 0;
}

void TcpSocket::destroySendQueue (void)
{
	delete m_pBuffer;
}

}
