#pragma once

#include <SonarCommon/Common.h>
#include <SonarIpcCommon/IpcMessage.h>
#include <SonarClient/ClientTypes.h>

namespace sonar
{
	class IpcUtil
	{
	public:
		static int getAvailableIpcPort();
		static SOCKET createIpcSocket();
		static bool bindIpcSocket (SOCKET sockfd, int port);
		static bool connectIpcSocket (SOCKET sockfd, int port);
		

		static PeerInfo IpcUtil::readPeerInfo(const IpcMessage &msg, size_t *argIndex);
		static void writePeerInfo(IpcMessage &msg, const PeerInfo &pi);


	private:
		static int bindSocket(SOCKET sockfd, int port);
	};

}

