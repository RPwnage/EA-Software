#pragma once

#include <SonarIpcCommon/IpcMessage.h>

namespace sonar
{
	enum EventType
	{
		CHANNEL_DESTROYED,
		CLIENT_REGISTERED,
		CLIENT_UNREGISTERED,
		CLIENT_STATE,
		KEEPALIVE,
	};

	class BackendEvent
	{
	public:
		BackendEvent(const IpcMessage &msg, EventType type);
		~BackendEvent();

		EventType type;
		IpcMessage msg;
	};
}

