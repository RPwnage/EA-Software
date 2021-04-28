#include <SonarIpcHost/InboundConnection.h>
#include <SonarIpcCommon/IpcUtil.h>

namespace sonar
{

InboundConnection::MessageHandlerMap InboundConnection::s_messageHandlers;

InboundConnection::InboundConnection (IpcHost &host)
	: m_host(host)
{
	if (s_messageHandlers.empty())
	{
		s_messageHandlers["connect"] = &InboundConnection::connectHandler;
		s_messageHandlers["disconnect"] = &InboundConnection::disconnectHandler;
		s_messageHandlers["muteClient"] = &InboundConnection::muteClientHandler;
		s_messageHandlers["recordStroke"] = &InboundConnection::recordStrokeHandler;
		s_messageHandlers["bindPushToTalkKey"] = &InboundConnection::bindPushToTalkKeyHandler;
		s_messageHandlers["bindPlaybackMuteKey"] = &InboundConnection::bindPlaybackMuteKeyHandler;
		s_messageHandlers["bindCaptureMuteKey"] = &InboundConnection::bindCaptureMuteKeyHandler;
		s_messageHandlers["setCaptureMode"] = &InboundConnection::setCaptureModeHandler;
		s_messageHandlers["setPlaybackMute"] = &InboundConnection::setPlaybackMuteHandler;
		s_messageHandlers["setCaptureMute"] = &InboundConnection::setCaptureMuteHandler;
		s_messageHandlers["setCaptureDevice"] = &InboundConnection::setCaptureDeviceHandler;
		s_messageHandlers["setPlaybackDevice"] = &InboundConnection::setPlaybackDeviceHandler;
		s_messageHandlers["setLoopback"] = &InboundConnection::setLoopbackHandler;
		s_messageHandlers["setAutoGain"] = &InboundConnection::setAutoGainHandler;
		s_messageHandlers["setCaptureGain"] = &InboundConnection::setCaptureGainHandler;
		s_messageHandlers["setPlaybackGain"] = &InboundConnection::setPlaybackGainHandler;
		s_messageHandlers["setVoiceSensitivity"] = &InboundConnection::setVoiceSensitivityHandler;
		s_messageHandlers["getCaptureMode"] = &InboundConnection::getCaptureModeHandler;
		s_messageHandlers["getPushToTalkKeyStroke"] = &InboundConnection::getPushToTalkKeyStrokeHandler;
		s_messageHandlers["getPlaybackMuteKeyStroke"] = &InboundConnection::getPlaybackMuteKeyStrokeHandler;
		s_messageHandlers["getPushToTalkKeyBlock"] = &InboundConnection::getPushToTalkKeyBlockHandler;
		s_messageHandlers["getPlaybackMuteKeyBlock"] = &InboundConnection::getPlaybackMuteKeyBlockHandler;
		s_messageHandlers["getCaptureMuteKeyStroke"] = &InboundConnection::getCaptureMuteKeyStrokeHandler;
		s_messageHandlers["getCaptureMuteKeyBlock"] = &InboundConnection::getCaptureMuteKeyBlockHandler;
		s_messageHandlers["getPlaybackMute"] = &InboundConnection::getPlaybackMuteHandler;
		s_messageHandlers["getCaptureMute"] = &InboundConnection::getCaptureMuteHandler;
		s_messageHandlers["getClients"] = &InboundConnection::getClientsHandler;
		s_messageHandlers["getCaptureDevices"] = &InboundConnection::getCaptureDevicesHandler;
		s_messageHandlers["getPlaybackDevices"] = &InboundConnection::getPlaybackDevicesHandler;
		s_messageHandlers["getCaptureDevice"] = &InboundConnection::getCaptureDeviceHandler;
		s_messageHandlers["getPlaybackDevice"] = &InboundConnection::getPlaybackDeviceHandler;
		s_messageHandlers["getLoopback"] = &InboundConnection::getLoopbackHandler;
		s_messageHandlers["getAutoGain"] = &InboundConnection::getAutoGainHandler;
		s_messageHandlers["getCaptureGain"] = &InboundConnection::getCaptureGainHandler;
		s_messageHandlers["getPlaybackGain"] = &InboundConnection::getPlaybackGainHandler;
		s_messageHandlers["getVoiceSensitivity"] = &InboundConnection::getVoiceSensitivityHandler;
		s_messageHandlers["isConnected"] = &InboundConnection::isConnectedHandler;
		s_messageHandlers["isDisconnected"] = &InboundConnection::isDisconnectedHandler;
		s_messageHandlers["getUserId"] = &InboundConnection::getUserIdHandler;
		s_messageHandlers["getUserDesc"] = &InboundConnection::getUserDescHandler;
		s_messageHandlers["getChannelId"] = &InboundConnection::getChannelIdHandler;
		s_messageHandlers["getChannelDesc"] = &InboundConnection::getChannelDescHandler;
		s_messageHandlers["getChid"] = &InboundConnection::getChidHandler;
		s_messageHandlers["getToken"] = &InboundConnection::getTokenHandler;
		s_messageHandlers["echo"] = &InboundConnection::echoHandler;
		s_messageHandlers["setSoftPushToTalk"] = &InboundConnection::setSoftPushToTalkHandler;
		s_messageHandlers["getSoftPushToTalk"] = &InboundConnection::getSoftPushToTalkHandler;
	}
}

void InboundConnection::connectHandler(IpcMessage &msg)
{
	bool result = m_host.connect(msg.getString(0));
	IpcMessage reply;
	reply.writeBool(result);
	this->sendReply(msg, reply);
}
void InboundConnection::disconnectHandler(IpcMessage &msg)
{
	bool result = m_host.disconnect(msg.getString(0), msg.getString(1));
	IpcMessage reply;
	reply.writeBool(result);
	this->sendReply(msg, reply);
}

void InboundConnection::muteClientHandler(IpcMessage &msg)
{
	m_host.muteClient(msg.getString(0), msg.getBool(1));
	IpcMessage reply;
	this->sendReply(msg, reply);
}

void InboundConnection::recordStrokeHandler(IpcMessage &msg)
{
	m_host.recordStroke();
	IpcMessage reply;
	this->sendReply(msg, reply);
}

void InboundConnection::bindPushToTalkKeyHandler(IpcMessage &msg)
{
	m_host.bindPushToTalkKey(msg.getString(0), msg.getBool(1));
	IpcMessage reply;
	this->sendReply(msg,reply);
}

void InboundConnection::bindPlaybackMuteKeyHandler(IpcMessage &msg)
{
	m_host.bindPlaybackMuteKey(msg.getString(0), msg.getBool(1));
	IpcMessage reply;
	this->sendReply(msg,reply);
}

void InboundConnection::bindCaptureMuteKeyHandler(IpcMessage &msg)
{
	m_host.bindCaptureMuteKey(msg.getString(0), msg.getBool(1));
	IpcMessage reply;
	this->sendReply(msg,reply);
}


void InboundConnection::setSoftPushToTalkHandler(IpcMessage &msg)
{
	m_host.setSoftPushToTalk(msg.getBool(0));
	IpcMessage reply;
	this->sendReply(msg, reply);
}

void InboundConnection::setCaptureModeHandler(IpcMessage &msg)
{
	m_host.setCaptureMode( (CaptureMode) msg.getInt(0));
	IpcMessage reply;
	this->sendReply(msg, reply);
}
void InboundConnection::setPlaybackMuteHandler(IpcMessage &msg)
{
	m_host.setPlaybackMute(msg.getBool(0));
	IpcMessage reply;
	this->sendReply(msg, reply);
}

void InboundConnection::setCaptureMuteHandler(IpcMessage &msg)
{
	m_host.setCaptureMute(msg.getBool(0));
	IpcMessage reply;
	this->sendReply(msg, reply);
}

void InboundConnection::setCaptureDeviceHandler(IpcMessage &msg)
{
	m_host.setCaptureDevice(msg.getString(0));
	IpcMessage reply;
	this->sendReply(msg, reply);
}

void InboundConnection::setPlaybackDeviceHandler(IpcMessage &msg)
{
	m_host.setPlaybackDevice(msg.getString(0));
	IpcMessage reply;
	this->sendReply(msg, reply);
}

void InboundConnection::setLoopbackHandler(IpcMessage &msg)
{
	m_host.setLoopback(msg.getBool(0));
	IpcMessage reply;
	this->sendReply(msg, reply);
}

void InboundConnection::setAutoGainHandler(IpcMessage &msg)
{
	m_host.setAutoGain(msg.getBool(0));
	IpcMessage reply;
	this->sendReply(msg, reply);
}

void InboundConnection::setCaptureGainHandler(IpcMessage &msg)
{
	m_host.setCaptureGain(msg.getInt(0));
	IpcMessage reply;
	this->sendReply(msg, reply);
}

void InboundConnection::setPlaybackGainHandler(IpcMessage &msg)
{
	m_host.setPlaybackGain(msg.getInt(0));
	IpcMessage reply;
	this->sendReply(msg, reply);
}

void InboundConnection::setVoiceSensitivityHandler(IpcMessage &msg)
{
	m_host.setVoiceSensitivity(msg.getInt(0));
	IpcMessage reply;
	this->sendReply(msg, reply);
}

void InboundConnection::getCaptureModeHandler(IpcMessage &msg)
{
	CaptureMode cpm = m_host.getCaptureMode();
	IpcMessage reply;
	reply.writeInt( (int) cpm);
	this->sendReply(msg, reply);
}

void InboundConnection::getPushToTalkKeyStrokeHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeString(m_host.getPushToTalkKeyStroke());
	this->sendReply(msg, reply);
}

void InboundConnection::getPlaybackMuteKeyStrokeHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeString(m_host.getPlaybackMuteKeyStroke());
	this->sendReply(msg, reply);
}

void InboundConnection::getPushToTalkKeyBlockHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeBool(m_host.getPushToTalkKeyBlock());
	this->sendReply(msg, reply);
}

void InboundConnection::getPlaybackMuteKeyBlockHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeBool(m_host.getPlaybackMuteKeyBlock());
	this->sendReply(msg, reply);
}

void InboundConnection::getCaptureMuteKeyStrokeHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeString(m_host.getCaptureMuteKeyStroke());
	this->sendReply(msg, reply);
}

void InboundConnection::getCaptureMuteKeyBlockHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeBool(m_host.getCaptureMuteKeyBlock());
	this->sendReply(msg, reply);
}


void InboundConnection::getPlaybackMuteHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeBool(m_host.getPlaybackMute());
	this->sendReply(msg, reply);
}

void InboundConnection::getCaptureMuteHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeBool(m_host.getCaptureMute());
	this->sendReply(msg, reply);
}

void InboundConnection::getClientsHandler(IpcMessage &msg)
{
	PeerList peerList = m_host.getClients();

	IpcMessage reply;

	for (PeerList::iterator iter = peerList.begin(); iter != peerList.end(); iter ++)
	{
		IpcUtil::writePeerInfo(reply, *iter);
	}

	sendReply(msg, reply);
}

void InboundConnection::getCaptureDevicesHandler(IpcMessage &msg)
{
	AudioDeviceList list = m_host.getCaptureDevices();
	
	IpcMessage reply;

	for (AudioDeviceList::iterator iter = list.begin(); iter != list.end(); iter ++)
	{
		reply.writeString(iter->id);
		reply.writeString(iter->name);
	}

	sendReply(msg, reply);
}

void InboundConnection::getPlaybackDevicesHandler(IpcMessage &msg)
{
	AudioDeviceList list = m_host.getPlaybackDevices();
	
	IpcMessage reply;

	for (AudioDeviceList::iterator iter = list.begin(); iter != list.end(); iter ++)
	{
		reply.writeString(iter->id);
		reply.writeString(iter->name);
	}

	sendReply(msg, reply);
}
void InboundConnection::getCaptureDeviceHandler(IpcMessage &msg)
{
	AudioDeviceId device = m_host.getCaptureDevice();
	IpcMessage reply;
	reply.writeString(device.id);
	reply.writeString(device.name);
	this->sendReply(msg, reply);
}

void InboundConnection::getPlaybackDeviceHandler(IpcMessage &msg)
{
	AudioDeviceId device = m_host.getPlaybackDevice();
	IpcMessage reply;
	reply.writeString(device.id);
	reply.writeString(device.name);
	this->sendReply(msg, reply);
}
void InboundConnection::getLoopbackHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeBool(m_host.getLoopback());
	this->sendReply(msg, reply);
}
void InboundConnection::getAutoGainHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeBool(m_host.getAutoGain());
	this->sendReply(msg, reply);
}

void InboundConnection::getCaptureGainHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeInt(m_host.getCaptureGain());
	this->sendReply(msg, reply);
}

void InboundConnection::getPlaybackGainHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeInt(m_host.getPlaybackGain());
	this->sendReply(msg, reply);
}

void InboundConnection::getVoiceSensitivityHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeInt(m_host.getVoiceSensitivity());
	this->sendReply(msg, reply);
}

void InboundConnection::isConnectedHandler(IpcMessage &msg)
{
	bool result = m_host.isConnected();
	IpcMessage reply;
	reply.writeBool(result);
	this->sendReply(msg, reply);
}

void InboundConnection::isDisconnectedHandler(IpcMessage &msg)
{
	bool result = m_host.isDisconnected();
	IpcMessage reply;
	reply.writeBool(result);
	this->sendReply(msg, reply);
}

void InboundConnection::getUserIdHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeString(m_host.getUserId());
	this->sendReply(msg, reply);
}

void InboundConnection::getUserDescHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeString(m_host.getUserDesc());
	this->sendReply(msg, reply);
}

void InboundConnection::getChannelIdHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeString(m_host.getChannelId());
	this->sendReply(msg, reply);
}

void InboundConnection::getChannelDescHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeString(m_host.getChannelDesc());
	this->sendReply(msg, reply);
}

void InboundConnection::getChidHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeInt(m_host.getChid());
	this->sendReply(msg, reply);
}

void InboundConnection::getTokenHandler(IpcMessage &msg)
{
	Token t = m_host.getToken();
	IpcMessage reply;
	reply.writeString(t.toString());
	this->sendReply(msg, reply);
}


void InboundConnection::getSoftPushToTalkHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeBool(m_host.getSoftPushToTalk());
	this->sendReply(msg, reply);
}

void InboundConnection::echoHandler(IpcMessage &msg)
{
	IpcMessage reply;
	reply.writeString(msg.getString(0));
	this->sendReply(msg, reply);
}


void InboundConnection::onMessage(IpcMessage &msg)
{
	MessageHandlerMap::iterator iter = s_messageHandlers.find(msg.getName());

	if (iter == s_messageHandlers.end())
	{
		return;
	}

	MessageHandler func =	iter->second;
	(this->*func)(msg);
}




}