#pragma once

#include <jlib/socketdefs.h>
#include <SonarClient/Client.h>
#include <SonarIpcHost/InboundConnection.h>

namespace sonar
{
	class InboundConnection;

class IpcHost : public Client, public IClientEvents
{
public:
	IpcHost(void);
	virtual ~IpcHost(void);
	void update();

	void setSocket(jlib::SOCKET sockfd);

protected:
	void onConnect(InboundConnection *connection);

	/* IClient */
	virtual void evtStrokeRecorded(CString &stroke);
	virtual void evtCaptureModeChanged(CaptureMode mode);
	virtual void evtPlaybackMuteKeyChanged(CString &stroke, bool block);
	virtual void evtCaptureMuteKeyChanged(CString &stroke, bool block);
	virtual void evtPushToTalkKeyChanged(CString &stroke, bool block);
    virtual void evtPushToTalkActiveChanged(bool active);
    virtual void evtTalkTimeOverLimit(bool over);
	virtual void evtConnected(int chid, CString &channelId, CString channelDesc);
	virtual void evtDisconnected(sonar::CString &reasonType, sonar::CString &reasonDesc);
	virtual void evtShutdown(void);
	virtual void evtClientJoined (const PeerInfo &client);
	virtual void evtClientParted (const PeerInfo &client, sonar::CString &reasonType, sonar::CString &reasonDesc);
	virtual void evtClientMuted (const PeerInfo &client);
	virtual void evtClientTalking (const PeerInfo &client);
	virtual void evtCaptureAudio (bool clip, float avgAmp, float peakAmp, bool vaState, bool xmit);
	virtual void evtPlaybackMuteChanged (bool state);
	virtual void evtCaptureMuteChanged (bool state);
	virtual void evtCaptureDeviceChanged(CString &deviceId, CString &deviceName);
	virtual void evtPlaybackDeviceChanged(CString &deviceId, CString &deviceName);
	virtual void evtLoopbackChanged (bool state);
	virtual void evtAutoGainChanged(bool state);
	virtual void evtCaptureGainChanged (int percent);
	virtual void evtPlaybackGainChanged (int percent);
	virtual void evtVoiceSensitivityChanged (int percent);
    virtual void evtChannelInactivity(unsigned long interval);

	void sendEvent(const IpcMessage &msg);

	typedef SonarMap<int, InboundConnection *> ConnectionMap;
	ConnectionMap m_connections;
	
	SOCKET m_sockfd;

};

}