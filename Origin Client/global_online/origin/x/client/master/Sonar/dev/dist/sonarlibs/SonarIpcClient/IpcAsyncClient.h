#pragma once

#include <SonarIpcCommon/SocketConnection.h>
#include <SonarIpcCommon/IpcMessage.h>

namespace sonar
{

	class IpcAsyncClient : public SocketConnection
	{
	public:
		virtual CString postMessage(IpcMessage &msg);

	private:
		void onMessage(IpcMessage &msg);

	protected:
		virtual void onEvent(const IpcMessage &msg);
		virtual void onReply(const IpcMessage &msg);
	};

}