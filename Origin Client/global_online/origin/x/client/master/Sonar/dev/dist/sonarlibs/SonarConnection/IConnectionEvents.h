#pragma once

#include <SonarConnection/Message.h>
#include <SonarConnection/Protocol.h>
#include <SonarCommon/Common.h>

namespace sonar
{
	class IConnectionEvents
	{
	public:
		virtual void onConnect(CString &channelId, CString &channelDesc) = 0;
		virtual void onDisconnect(CString &reasonType, CString &reasonDesc) = 0;
		virtual void onClientJoined(int chid, CString &userId, CString &userDesc) = 0;
		virtual void onClientParted(int chid, CString &userId, CString &userDesc, CString &reasonType, CString &reasonDesc) = 0;
		virtual void onTakeBegin() = 0;
		virtual void onFrameBegin(long long timestamp) = 0;
		virtual void onFrameClientPayload(int chid, const void *payload, size_t cbPayload) = 0;
		virtual void onFrameEnd(long long timestamp = 0) = 0;
		virtual void onTakeEnd() = 0;
		virtual void onEchoMessage(const Message &message) = 0;
        virtual void onChannelInactivity(unsigned long interval) = 0;


	};

}