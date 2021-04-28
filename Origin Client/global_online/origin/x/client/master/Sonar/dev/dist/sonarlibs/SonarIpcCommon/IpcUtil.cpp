#include <SonarCommon/Common.h>
#include <SonarIpcCommon/IpcUtil.h>

namespace sonar
{

PeerInfo IpcUtil::readPeerInfo(const IpcMessage &msg, size_t *argIndex)
{
	PeerInfo pi;
	size_t iArg = *argIndex;

	pi.chid = msg.getInt(iArg + 0);
	pi.userId = msg.getString(iArg + 1);
	pi.userDesc = msg.getString(iArg + 2);
	pi.isTalking = msg.getBool(iArg + 3);
	pi.isMuted = msg.getBool(iArg + 4);
	*argIndex = iArg + 5;
	return pi;
}

void IpcUtil::writePeerInfo(IpcMessage &msg, const PeerInfo &pi)
{
	msg.writeInt(pi.chid);
	msg.writeString(pi.userId);
	msg.writeString(pi.userDesc);
	msg.writeBool(pi.isTalking);
	msg.writeBool(pi.isMuted);
}

	
int IpcUtil::bindSocket(SOCKET sockfd, int port)
{
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons( (u_short) port);
	memset (addr.sin_zero, 0, sizeof (addr.sin_zero));

	if (bind(sockfd, (struct sockaddr *) &addr, sizeof(sockaddr_in)) == -1)
	{
		return -1;
	}

	listen(sockfd, 5);

	int len = sizeof(addr);

	if (getsockname(sockfd, (struct sockaddr *) &addr, &len) == -1)
	{
		return -1;
	}

	return ntohs(addr.sin_port);
}
	
int IpcUtil::getAvailableIpcPort()
{
	for (int tries = 0; tries < 10; tries ++)
	{
		SOCKET sockfd = createIpcSocket();
		int port = bindSocket(sockfd, 0);
		common::SocketClose(sockfd);

		if (port > 0)
		{
			return port;
		}
	}

	return -1;
}

SOCKET IpcUtil::createIpcSocket()
{
	SOCKET sockfd = socket(AF_INET, SOCK_STREAM, 0);
	common::SocketSetNonBlock(sockfd, true);
	return sockfd;
}

bool IpcUtil::bindIpcSocket (SOCKET sockfd, int port)
{
	return bindSocket(sockfd, port) == port;
}

bool IpcUtil::connectIpcSocket (SOCKET sockfd, int port)
{
	struct sockaddr_in addr;

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
	addr.sin_port = htons( (unsigned short) port);
	memset (addr.sin_zero, 0, sizeof (addr.sin_zero));
	common::SocketSetNonBlock(sockfd, false);

	if (connect(sockfd, (struct sockaddr *) &addr, sizeof(sockaddr_in)) == -1)
	{
		return false;
	}

	common::SocketSetNonBlock(sockfd, true);
	return true;
}


}