#pragma once 

#include <jlib/socketdefs.h>
#include <jlib/ByteBuffer.h>
#include <SonarIpcCommon/IpcMessage.h>

namespace sonar
{
	class SocketConnection
	{
	public:
		static const int BUFFER_SIZE = (1024 * 1024);
		static const int COMMAND_SIZE = 65536;

		SocketConnection(void);
		virtual ~SocketConnection(void);
		bool update();
		jlib::SOCKET getSocket(void);
		bool hasTxBytes(void);
		void send(const IpcMessage &msg);
		void sendReply(const IpcMessage &request, IpcMessage &reply);
		void setSocket(jlib::SOCKET sockfd);

	protected:

		virtual void onMessage(IpcMessage &msg) = 0;
		CString generateId();

	private:

		void processData(const char *buffer, size_t length);

		jlib::ByteBuffer m_rxBuffer;
		jlib::ByteBuffer m_txBuffer;
		jlib::SOCKET m_sockfd;
		unsigned long m_idCounter;
	};


}