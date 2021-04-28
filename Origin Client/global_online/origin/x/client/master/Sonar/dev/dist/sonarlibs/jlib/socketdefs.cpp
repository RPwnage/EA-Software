#include <jlib/socketdefs.h>


namespace jlib
{
	void SocketClose(jlib::SOCKET fd)
	{
		::closesocket(fd);
	}

	bool SocketWouldBlock(jlib::SOCKET fd)
	{
		fd = 0;
		return (WSAGetLastError () == WSAEWOULDBLOCK);
	}

	bool SocketEINPROGRESS(jlib::SOCKET fd)
	{
		fd = 0;
		return (WSAGetLastError () == WSAEINPROGRESS);

	}

	int SocketGetLastError(void)
	{
		return  (int) WSAGetLastError ();

	}
	
	void SocketSetNonBlock(jlib::SOCKET fd, bool state)
	{
		unsigned long flags = (state) ? 1 : 0;
		ioctlsocket(fd, FIONBIO, &flags);
	}

	int PortableGetCurrentThreadId()
	{
		return (int) GetCurrentThreadId();
	}

	void YieldThread()
	{
		SwitchToThread();
	}
}


