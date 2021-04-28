#pragma once

#include <SonarCommon/Common.h>
#include <string.h>

namespace sonar
{

class TcpSocket
{
private:
	SOCKET m_sockfd;
	struct sockaddr_in m_sockaddr;

	char *m_pBuffer;
	int m_iBuffer;
	int m_cbBuffer;

private:
	void setupQueue (void);
	void destroySendQueue (void);

public:
	TcpSocket (void);
	TcpSocket (SOCKET _inSocket);
	TcpSocket (SOCKET _inSocket, struct sockaddr_in *_pSockaddr);
	~TcpSocket (void);

	int sendf (const char *format, ...);
	int recvUntil (char *_poutBuffer, int _cbLength, int _c);
	int recv (void *_pBuffer, int _cbMaxBuffer);
	int send (void *_pBuffer, int _cbBuffer);

	SOCKET getSocket (void);
	struct sockaddr_in *getSockaddr (void);

	int bind (const char *_addrString, int _port);
	TcpSocket *accept (void);
	int close (void);

	int connect (const sockaddr_in &_addr);
	int connect (const char *_addrString, int _port);
	int recreate (void);

	int sendQueue (void *_pData, int _cbLength);
	int setNonBlocking (bool _state);

	bool wouldBlock (void);
};

}