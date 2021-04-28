#include <SonarIpcHost/IpcHost.h>
#include <SonarIpcCommon/IpcUtil.h>
#include <jlib/String.h>

namespace sonar
{

IpcHost::IpcHost(void)
{
	m_sockfd = (jlib::SOCKET) -1;

	addEventListener(this);
}

IpcHost::~IpcHost(void)
{
}

void IpcHost::setSocket(jlib::SOCKET sockfd)
{
	m_sockfd = sockfd;
}

void IpcHost::onConnect(InboundConnection *connection)
{
	
}


void IpcHost::update()
{
	Client::update();

	if (m_sockfd == -1)
	{
		return;
	}

	// accept

	for (;;)
	{
		struct sockaddr_in remoteAddr;

		int len = sizeof(remoteAddr);
		int result = accept(m_sockfd, (struct sockaddr *) &remoteAddr, &len);

		if (result == -1)
		{
			if (!jlib::SocketWouldBlock(m_sockfd))
			{
				return;
			}

			break;
		}
			
		InboundConnection *connection = new InboundConnection(*this);
		connection->setSocket((jlib::SOCKET) result);
		m_connections[connection->getSocket()] = connection;

		onConnect(connection);
	}

	SonarList<InboundConnection *> deleteList;

	for (ConnectionMap::iterator iter = m_connections.begin(); iter != m_connections.end(); iter ++)
	{
		InboundConnection *connection = iter->second;

		if (!connection->update())
		{
			deleteList.push_back(connection);
		}
	}

	while (!deleteList.empty())
	{
		InboundConnection *connection = deleteList.front();
		deleteList.pop_front();

		m_connections.erase(connection->getSocket());
		delete connection;
	}
}

void IpcHost::sendEvent(const IpcMessage &msg)
{
	for (ConnectionMap::iterator iter = m_connections.begin(); iter != m_connections.end(); iter ++)
	{
		InboundConnection *connection = iter->second;
		connection->send(msg);
	}	
}


void IpcHost::evtStrokeRecorded(CString &stroke)
{
	IpcMessage msg("evtStrokeRecorded");
	msg.writeString(stroke);
	sendEvent(msg);
}

void IpcHost::evtCaptureModeChanged(CaptureMode mode)
{
	IpcMessage msg("evtCaptureModeChanged");
	msg.writeInt( (int) mode);
	sendEvent(msg);
}

void IpcHost::evtPlaybackMuteKeyChanged(CString &stroke, bool block)
{
	IpcMessage msg("evtPlaybackMuteKeyChanged");
	msg.writeString(stroke);
	msg.writeBool(block);
	sendEvent(msg);
}

void IpcHost::evtCaptureMuteKeyChanged(CString &stroke, bool block)
{
	IpcMessage msg("evtCaptureMuteKeyChanged");
	msg.writeString(stroke);
	msg.writeBool(block);
	sendEvent(msg);
}

void IpcHost::evtPushToTalkKeyChanged(CString &stroke, bool block)
{
	IpcMessage msg("evtPushToTalkKeyChanged");
	msg.writeString(stroke);
	msg.writeBool(block);
	sendEvent(msg);
}

void IpcHost::evtPushToTalkActiveChanged(bool active)
{
    IpcMessage msg("evtPushToTalkActiveChanged");
    msg.writeBool(active);
    sendEvent(msg);
}

void IpcHost::evtTalkTimeOverLimit(bool over)
{
    IpcMessage msg("evtTalkTimeOverLimit");
    msg.writeBool(over);
    sendEvent(msg);
}

void IpcHost::evtConnected(int chid, CString &channelId, CString channelDesc)
{
	IpcMessage msg("evtConnected");
	msg.writeInt(chid);
	msg.writeString(channelId);
	msg.writeString(channelDesc);
	sendEvent(msg);
}

void IpcHost::evtDisconnected(sonar::CString &reasonType, sonar::CString &reasonDesc)
{
	IpcMessage msg("evtDisconnected");
	msg.writeString(reasonType);
	msg.writeString(reasonDesc);
	sendEvent(msg);
}

void IpcHost::evtShutdown(void)
{
	IpcMessage msg("evtShutdown");
	sendEvent(msg);
}

void IpcHost::evtClientJoined (const PeerInfo &client)
{
	IpcMessage msg("evtClientJoined");
	IpcUtil::writePeerInfo(msg, client);
	sendEvent(msg);
}

void IpcHost::evtClientParted (const PeerInfo &client, sonar::CString &reasonType, sonar::CString &reasonDesc)
{
	IpcMessage msg("evtClientParted");
	IpcUtil::writePeerInfo(msg, client);
	msg.writeString(reasonType);
	msg.writeString(reasonDesc);
	sendEvent(msg);
}

void IpcHost::evtClientMuted (const PeerInfo &client)
{
	IpcMessage msg("evtClientMuted");
	IpcUtil::writePeerInfo(msg, client);
	sendEvent(msg);
}

void IpcHost::evtClientTalking (const PeerInfo &client)
{
	IpcMessage msg("evtClientTalking");
	IpcUtil::writePeerInfo(msg, client);
	sendEvent(msg);
}

void IpcHost::evtCaptureAudio (bool clip, float avgAmp, float peakAmp, bool vaState, bool xmit)
{
	IpcMessage msg("evtCaptureAudio");
	msg.writeBool(clip);
	msg.writeFloat(avgAmp);
	msg.writeFloat(peakAmp);
	msg.writeBool(vaState);
	msg.writeBool(xmit);
	sendEvent(msg);
}

void IpcHost::evtPlaybackMuteChanged (bool state)
{
	IpcMessage msg("evtPlaybackMuteChanged");
	msg.writeBool(state);
	sendEvent(msg);
}

void IpcHost::evtCaptureMuteChanged (bool state)
{
	IpcMessage msg("evtCaptureMuteChanged");
	msg.writeBool(state);
	sendEvent(msg);
}

void IpcHost::evtCaptureDeviceChanged(CString &deviceId, CString &deviceName)
{
	IpcMessage msg("evtCaptureDeviceChanged");
	msg.writeString(deviceId);
	msg.writeString(deviceName);
	sendEvent(msg);
}

void IpcHost::evtPlaybackDeviceChanged(CString &deviceId, CString &deviceName)
{
	IpcMessage msg("evtPlaybackDeviceChanged");
	msg.writeString(deviceId);
	msg.writeString(deviceName);
	sendEvent(msg);
}

void IpcHost::evtLoopbackChanged (bool state)
{
	IpcMessage msg("evtLoopbackChanged");
	msg.writeBool(state);
	sendEvent(msg);
}

void IpcHost::evtAutoGainChanged(bool state)
{
	IpcMessage msg("evtAutoGainChanged");
	msg.writeBool(state);
	sendEvent(msg);
}

void IpcHost::evtCaptureGainChanged (int percent)
{
	IpcMessage msg("evtCaptureGainChanged");
	msg.writeInt(percent);
	sendEvent(msg);
}

void IpcHost::evtPlaybackGainChanged (int percent)
{
	IpcMessage msg("evtPlaybackGainChanged");
	msg.writeInt(percent);
	sendEvent(msg);
}

void IpcHost::evtVoiceSensitivityChanged (int percent)
{
	IpcMessage msg("evtVoiceSensitivityChanged");
	msg.writeInt(percent);
	sendEvent(msg);
}

void IpcHost::evtChannelInactivity(unsigned long interval)
{
    IpcMessage msg("evtChannelInactivity");
    msg.writeInt(interval);
    sendEvent(msg);
}

}