#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <mswsock.h>
#include <Ws2tcpip.h>

namespace jlib
{
	const int MSG_NOSIGNAL = 0;

	typedef int socklen_t;
	typedef ::SOCKET SOCKET;

	void SocketClose(jlib::SOCKET fd);
	bool SocketWouldBlock(jlib::SOCKET fd);
	bool SocketEINPROGRESS(jlib::SOCKET fd);
	int SocketGetLastError(void);
	void SocketSetNonBlock(jlib::SOCKET fd, bool state);
	int PortableGetCurrentThreadId();
	void YieldThread();
}
#else
#error "jlib socketdefs needs to be ported, should be easy though"
#endif
