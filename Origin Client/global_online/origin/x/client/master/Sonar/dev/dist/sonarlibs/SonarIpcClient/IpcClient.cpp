#include <SonarIpcClient/IpcClient.h>
#include <SonarIpcCommon/IpcUtil.h>
#include <jlib/Time.h>

namespace sonar
{

IpcClient::MessageHandlerMap IpcClient::s_messageHandlers;

IpcClient::IpcClient(void)
{
	if (s_messageHandlers.empty())
	{
		s_messageHandlers["evtStrokeRecorded"] = &IpcClient::evtStrokeRecordedHandler;
		s_messageHandlers["evtCaptureModeChanged"] = &IpcClient::evtCaptureModeChangedHandler;
		s_messageHandlers["evtPlaybackMuteKeyChanged"] = &IpcClient::evtPlaybackMuteKeyChangedHandler;
		s_messageHandlers["evtCaptureMuteKeyChanged"] = &IpcClient::evtCaptureMuteKeyChangedHandler;
		s_messageHandlers["evtPushToTalkKeyChanged"] = &IpcClient::evtPushToTalkKeyChangedHandler;
		s_messageHandlers["evtConnected"] = &IpcClient::evtConnectedHandler;
		s_messageHandlers["evtDisconnected"] = &IpcClient::evtDisconnectedHandler;
		s_messageHandlers["evtShutdown"] = &IpcClient::evtShutdownHandler;
		s_messageHandlers["evtClientJoined"] = &IpcClient::evtClientJoinedHandler;
		s_messageHandlers["evtClientParted"] = &IpcClient::evtClientPartedHandler;
		s_messageHandlers["evtClientMuted"] = &IpcClient::evtClientMutedHandler;
		s_messageHandlers["evtClientTalking"] = &IpcClient::evtClientTalkingHandler;
		s_messageHandlers["evtCaptureAudio"] = &IpcClient::evtCaptureAudioHandler;
		s_messageHandlers["evtPlaybackMuteChanged"] = &IpcClient::evtPlaybackMuteChangedHandler;
		s_messageHandlers["evtCaptureMuteChanged"] = &IpcClient::evtCaptureMuteChangedHandler;
		s_messageHandlers["evtCaptureDeviceChanged"] = &IpcClient::evtCaptureDeviceChangedHandler;
		s_messageHandlers["evtPlaybackDeviceChanged"] = &IpcClient::evtPlaybackDeviceChangedHandler;
		s_messageHandlers["evtLoopbackChanged"] = &IpcClient::evtLoopbackChangedHandler;
		s_messageHandlers["evtAutoGainChanged"] = &IpcClient::evtAutoGainChangedHandler;
		s_messageHandlers["evtCaptureGainChanged"] = &IpcClient::evtCaptureGainChangedHandler;
		s_messageHandlers["evtPlaybackGainChanged"] = &IpcClient::evtPlaybackGainChangedHandler;
		s_messageHandlers["evtVoiceSensitivityChanged"] = &IpcClient::evtVoiceSensitivityChangedHandler;
	}
	m_isRunning = true;
	m_thread = jlib::Thread::createThread(StaticThreadProc, this);
}

IpcClient::~IpcClient(void)
{
	m_isRunning = false;
	jlib::SocketClose(getSocket());
	m_thread.join();
}


void *IpcClient::StaticThreadProc(void *arg)
{
	((IpcClient *) arg)->threadProc();
	return NULL;
}

void IpcClient::threadProc(void)
{
	while (m_isRunning)
	{
		sonar::common::sleepFrame();

		if (getSocket() == -1)
		{
			continue;
		}

		{
			jlib::Spinlocker locker(m_lock);

			if (!SocketConnection::update())
			{
				return;
			}
		}

	}
}


CString IpcClient::postMessage(IpcMessage &msg)
{
	CString id = generateId();
	msg.setId(id);

	{
		jlib::Spinlocker locker(m_lock);
		this->send(msg);
	}

	return id;
}

IpcMessage IpcClient::waitForReply(CString &expId)
{
	for (;;)
	{
		m_replyCond.WaitAndLock();

		if (m_replyMsg.getId() == expId)
		{
			IpcMessage ret = m_replyMsg;
			m_replyCond.Unlock();
			return ret;
		}

		m_replyCond.WaitForCondition();

		if (m_replyMsg.getId() == expId)
		{
			m_replyCond.Unlock();
			return m_replyMsg;
		}

		m_replyCond.Unlock();
	}


}

IpcMessage IpcClient::postAndWait(IpcMessage &msg)
{
	CString id = postMessage(msg);
	return waitForReply(id);
}

void IpcClient::update()
{
	{
		jlib::Spinlocker locker(m_lock);

		while (!m_inQueue.empty())
		{
			IpcMessage msg = m_inQueue.front();
			m_inQueue.pop_front();

			MessageHandlerMap::iterator iter = s_messageHandlers.find(msg.getName());
			
			if (iter != s_messageHandlers.end())
			{
				MessageHandler func = iter->second;
				(this->*func)(msg);
			}

		}
	}

}


void IpcClient::onEvent(const IpcMessage &msg)
{
	m_inQueue.push_back(msg);
}

void IpcClient::onReply(const IpcMessage &msg)
{
		m_replyCond.WaitAndLock();
		m_replyMsg = msg;
		m_replyCond.Signal();
		m_replyCond.Unlock();
}

bool IpcClient::connect(CString &token)
{
	IpcMessage msg("connect");
	msg.writeString(token);

	IpcMessage reply = postAndWait(msg);
	return reply.getBool(0);
}

bool IpcClient::disconnect(sonar::CString &reasonType, sonar::CString &reasonDesc)
{
	IpcMessage msg("disconnect");
	msg.writeString(reasonType);
	msg.writeString(reasonDesc);

	IpcMessage reply = postAndWait(msg);
	return reply.getBool(0);
}

void IpcClient::muteClient(CString &userId, bool state)
{
	IpcMessage msg("muteClient");
	msg.writeString(userId);
	msg.writeBool(state);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::recordStroke()
{
	IpcMessage msg("recordStroke");
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::bindPushToTalkKey(CString &stroke, bool block)
{
	IpcMessage msg("bindPushToTalkKey");
	msg.writeString(stroke);
	msg.writeBool(block);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::bindPlaybackMuteKey(CString &stroke, bool block)
{
	IpcMessage msg("bindPlaybackMuteKey");
	msg.writeString(stroke);
	msg.writeBool(block);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::bindCaptureMuteKey(CString &stroke, bool block)
{
	IpcMessage msg("bindCaptureMuteKey");
	msg.writeString(stroke);
	msg.writeBool(block);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::setSoftPushToTalk(bool state)
{
	IpcMessage msg("setSoftPushToTalk");
	msg.writeBool(state);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::setCaptureMode(CaptureMode mode)
{
	IpcMessage msg("setCaptureMode");
	msg.writeInt( (int) mode);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::setPlaybackMute(bool state)
{
	IpcMessage msg("setPlaybackMute");
	msg.writeBool(state);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::setCaptureMute(bool state)
{
	IpcMessage msg("setCaptureMute");
	msg.writeBool(state);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::setCaptureDevice (CString &deviceId)
{
	IpcMessage msg("setCaptureDevice");
	msg.writeString(deviceId);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::setPlaybackDevice (CString &deviceId)
{
	IpcMessage msg("setPlaybackDevice");
	msg.writeString(deviceId);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::setLoopback(bool state)
{
	IpcMessage msg("setLoopback");
	msg.writeBool(state);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::setAutoGain(bool state)
{
	IpcMessage msg("setAutoGain");
	msg.writeBool(state);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::setCaptureGain(int percent)
{
	IpcMessage msg("setCaptureGain");
	msg.writeInt(percent);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::setPlaybackGain(int percent)
{
	IpcMessage msg("setPlaybackGain");
	msg.writeInt(percent);
	IpcMessage reply = postAndWait(msg);
}

void IpcClient::setVoiceSensitivity(int percent)
{
	IpcMessage msg("setVoiceSensitivity");
	msg.writeInt(percent);
	IpcMessage reply = postAndWait(msg);
}

CaptureMode IpcClient::getCaptureMode()
{
	IpcMessage msg("getCaptureMode");
	IpcMessage reply = postAndWait(msg);

	return (CaptureMode) reply.getInt(0);
}

bool IpcClient::getSoftPushToTalk(void)
{
	IpcMessage msg("getSoftPushToTalk");
	IpcMessage reply = postAndWait(msg);

	return reply.getBool(0);
}

CString IpcClient::getPushToTalkKeyStroke()
{
	IpcMessage msg("getPushToTalkKeyStroke");
	IpcMessage reply = postAndWait(msg);
	return reply.getString(0);
}

CString IpcClient::getPlaybackMuteKeyStroke()
{
	IpcMessage msg("getPlaybackMuteKeyStroke");
	IpcMessage reply = postAndWait(msg);
	return reply.getString(0);
}

bool IpcClient::getPushToTalkKeyBlock()
{
	IpcMessage msg("getPushToTalkKeyBlock");
	IpcMessage reply = postAndWait(msg);
	return reply.getBool(0);
}

bool IpcClient::getPlaybackMuteKeyBlock()
{
	IpcMessage msg("getPlaybackMuteKeyBlock");
	IpcMessage reply = postAndWait(msg);
	return reply.getBool(0);
}

CString IpcClient::getCaptureMuteKeyStroke()
{
	IpcMessage msg("getCaptureMuteKeyStroke");
	IpcMessage reply = postAndWait(msg);
	return reply.getString(0);
}

bool IpcClient::getCaptureMuteKeyBlock()
{
	IpcMessage msg("getCaptureMuteKeyBlock");
	IpcMessage reply = postAndWait(msg);
	return reply.getBool(0);
}

bool IpcClient::getPlaybackMute(void)
{
	IpcMessage msg("getPlaybackMute");
	IpcMessage reply = postAndWait(msg);
	return reply.getBool(0);
}

bool IpcClient::getCaptureMute(void)
{
	IpcMessage msg("getCaptureMute");
	IpcMessage reply = postAndWait(msg);
	return reply.getBool(0);
}

PeerList IpcClient::getClients()
{
	IpcMessage msg("getClients");
	IpcMessage reply = postAndWait(msg);

	size_t length = reply.getArguments().size();
	PeerList ret;

	for (size_t index = 0; index < length; )
	{
		ret.push_back(IpcUtil::readPeerInfo(reply, &index));
	}

	return ret;
}

AudioDeviceList IpcClient::getCaptureDevices()
{
	IpcMessage msg("getCaptureDevices");
	IpcMessage reply = postAndWait(msg);
	
	AudioDeviceList ret;
	size_t length = reply.getArguments().size();

	for (size_t index = 0; index < length; index += 2)
	{
		AudioDeviceId device;
		device.id = reply.getString(index + 0);
		device.name = reply.getString(index + 1);

		ret.push_back(device);
	}

	return ret;

}

AudioDeviceList IpcClient::getPlaybackDevices()
{
	IpcMessage msg("getPlaybackDevices");
	IpcMessage reply = postAndWait(msg);
	
	AudioDeviceList ret;
	size_t length = reply.getArguments().size();

	for (size_t index = 0; index < length; index += 2)
	{
		AudioDeviceId device;
		device.id = reply.getString(index + 0);
		device.name = reply.getString(index + 1);

		ret.push_back(device);
	}

	return ret;
}

AudioDeviceId IpcClient::getCaptureDevice()
{
	IpcMessage msg("getCaptureDevice");
	IpcMessage reply = postAndWait(msg);
	AudioDeviceId device;
	device.id = reply.getString(0);
	device.name = reply.getString(1);
	return device;
}

AudioDeviceId IpcClient::getPlaybackDevice()
{
	IpcMessage msg("getPlaybackDevice");
	IpcMessage reply = postAndWait(msg);
	AudioDeviceId device;
	device.id = reply.getString(0);
	device.name = reply.getString(1);
	return device;
}

bool IpcClient::getLoopback(void)
{
	IpcMessage msg("getLoopback");
	IpcMessage reply = postAndWait(msg);
	return reply.getBool(0);
}

bool IpcClient::getAutoGain(void)
{
	IpcMessage msg("getAutoGain");
	IpcMessage reply = postAndWait(msg);
	return reply.getBool(0);
}

int IpcClient::getCaptureGain(void)
{
	IpcMessage msg("getCaptureGain");
	IpcMessage reply = postAndWait(msg);
	return reply.getInt(0);
}

int IpcClient::getPlaybackGain(void)
{
	IpcMessage msg("getPlaybackGain");
	IpcMessage reply = postAndWait(msg);
	return reply.getInt(0);
}

int IpcClient::getVoiceSensitivity(void)
{
	IpcMessage msg("getVoiceSensitivity");
	IpcMessage reply = postAndWait(msg);
	return reply.getInt(0);
}

bool IpcClient::isConnected(void)
{
	IpcMessage msg("isConnected");
	IpcMessage reply = postAndWait(msg);
	return reply.getBool(0);
}

bool IpcClient::isDisconnected(void)
{
	IpcMessage msg("isDisconnected");
	IpcMessage reply = postAndWait(msg);
	return reply.getBool(0);
}

CString IpcClient::getUserId()
{
	IpcMessage msg("getUserId");
	IpcMessage reply = postAndWait(msg);
	return reply.getString(0);
}

CString IpcClient::getUserDesc()
{
	IpcMessage msg("getUserDesc");
	IpcMessage reply = postAndWait(msg);
	return reply.getString(0);
}

CString IpcClient::getChannelId()
{
	IpcMessage msg("getChannelId");
	IpcMessage reply = postAndWait(msg);
	return reply.getString(0);
}

CString IpcClient::getChannelDesc()
{
	IpcMessage msg("getChannelDesc");
	IpcMessage reply = postAndWait(msg);
	return reply.getString(0);
}

int IpcClient::getChid(void)
{
	IpcMessage msg("getChid");
	IpcMessage reply = postAndWait(msg);
	return reply.getInt(0);
}

Token IpcClient::getToken(void)
{
	IpcMessage msg("getToken");
	IpcMessage reply = postAndWait(msg);
	
	Token t;
	t.parse(reply.getString(0));
	return t;
}


void IpcClient::evtStrokeRecordedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtStrokeRecorded(msg.getString(0));
	}
}

void IpcClient::evtCaptureModeChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtCaptureModeChanged( (CaptureMode) msg.getInt(0));
	}

}

void IpcClient::evtPlaybackMuteKeyChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtPlaybackMuteKeyChanged(msg.getString(0), msg.getBool(1));
	}
}

void IpcClient::evtCaptureMuteKeyChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtCaptureMuteKeyChanged(msg.getString(0), msg.getBool(1));
	}
}

void IpcClient::evtPushToTalkKeyChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtPushToTalkKeyChanged(msg.getString(0), msg.getBool(1));
	}
}

void IpcClient::evtConnectedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtConnected(msg.getInt(0), msg.getString(1), msg.getString(2));
	}
}

void IpcClient::evtDisconnectedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtDisconnected(msg.getString(0), msg.getString(1));
	}
}

void IpcClient::evtShutdownHandler(IpcMessage &msg){}

void IpcClient::evtClientJoinedHandler(IpcMessage &msg)
{
	size_t index = 0;
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtClientJoined(IpcUtil::readPeerInfo(msg, &index));
	}
}

void IpcClient::evtClientPartedHandler(IpcMessage &msg)
{
	size_t index = 0;
	PeerInfo pi = IpcUtil::readPeerInfo(msg, &index);

	CString &reasonType = msg.getString(index + 0);
	CString &reasonDesc = msg.getString(index + 1);


	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtClientParted(pi, reasonType, reasonDesc);
	}
}

void IpcClient::evtClientMutedHandler(IpcMessage &msg)
{
	size_t index = 0;
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtClientMuted(IpcUtil::readPeerInfo(msg, &index));
	}
}

void IpcClient::evtClientTalkingHandler(IpcMessage &msg)
{
	size_t index = 0;
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtClientTalking(IpcUtil::readPeerInfo(msg, &index));
	}
}

void IpcClient::evtCaptureAudioHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtCaptureAudio(msg.getBool(0), msg.getFloat(1), msg.getFloat(2), msg.getBool(3), msg.getBool(4));
	}
}

void IpcClient::evtPlaybackMuteChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtPlaybackMuteChanged(msg.getBool(0));
	}
}

void IpcClient::evtCaptureMuteChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtCaptureMuteChanged(msg.getBool(0));
	}
}

void IpcClient::evtCaptureDeviceChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtCaptureDeviceChanged(msg.getString(0), msg.getString(1));
	}
}

void IpcClient::evtPlaybackDeviceChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtPlaybackDeviceChanged(msg.getString(0), msg.getString(1));
	}
}

void IpcClient::evtLoopbackChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtLoopbackChanged(msg.getBool(0));
	}
}

void IpcClient::evtAutoGainChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtAutoGainChanged(msg.getBool(0));
	}
}

void IpcClient::evtCaptureGainChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtCaptureGainChanged(msg.getInt(0));
	}
}

void IpcClient::evtPlaybackGainChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtPlaybackGainChanged(msg.getInt(0));
	}
}

void IpcClient::evtVoiceSensitivityChangedHandler(IpcMessage &msg)
{
	for (EventListeners::iterator iter = m_listeners.begin(); iter != m_listeners.end(); iter ++)
	{
		(*iter)->evtVoiceSensitivityChanged(msg.getInt(0));
	}
}

void IpcClient::addEventListener(IClientEvents *listener)
{
	m_listeners.push_back(listener);
}

}