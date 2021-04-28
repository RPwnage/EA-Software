#include <crtdbg.h>

class TestClient : public sonar::Client, sonar::IClientEvents
{
public:

	TestClient(void)
	{
		m_enableCaptureAudio = false;
		addEventListener(this);
	}

	~TestClient(void)
	{
		clearEvents();
		_ASSERTE( _CrtCheckMemory( ) );
	}

	void enableCaptureAudio(bool state)
	{
		m_enableCaptureAudio = state;
	}


	/*[ ]*/ 
	virtual void evtStrokeRecorded(CString &stroke)
	{
		events.push_back(new StrokeRecordedEvent(stroke));
	}
	/*[ ]*/ 
	virtual void evtCaptureModeChanged(sonar::CaptureMode mode)
	{
		events.push_back(new CaptureModeChangedEvent(mode));
	}
	/*[ ]*/ 
	virtual void evtPlaybackMuteKeyChanged(CString &stroke, bool block)
	{
		events.push_back(new PlaybackMuteKeyChanged(stroke, block));
	}
	/*[ ]*/ 
	virtual void evtCaptureMuteKeyChanged(CString &stroke, bool block)
	{
		events.push_back(new CaptureMuteKeyChanged(stroke, block));
	}
	/*[ ]*/ 
	virtual void evtPushToTalkKeyChanged(CString &stroke, bool block)
	{
		events.push_back(new PushToTalkKeyChangedEvent(stroke, block));
	}
    /*[ ]*/ 
    virtual void evtPushToTalkActiveChanged(bool active)
    {
        events.push_back(new PushToTalkActiveChangedEvent(active));
    }
    /*[ ]*/ 
    virtual void evtTalkTimeOverLimit(bool over)
    {
        events.push_back(new TalkTimeOverLimitEvent(over));
    }
    /*[ ]*/ 
	virtual void evtConnected(int chid, CString &channelId, CString channelDesc)
	{
		events.push_back(new ConnectedEvent(chid, channelId, channelDesc));
	}
	/*[ ]*/ 
	virtual void evtDisconnected(sonar::CString &reasonType, sonar::CString &reasonDesc)
	{
		events.push_back(new DisconnectedEvent(reasonType, reasonDesc));
	}
	/*[ ]*/ 
	virtual void evtShutdown(void)
	{
	}
	/*[ ]*/ 
	virtual void evtClientJoined (const sonar::PeerInfo &client)
	{
		events.push_back(new ClientJoinedEvent(client));
	}
	/*[ ]*/ 
	virtual void evtClientParted (const sonar::PeerInfo &client, sonar::CString &reasonType, sonar::CString &reasonDesc)
	{
		events.push_back(new ClientPartedEvent(client, reasonType, reasonDesc));
	}
	/*[ ]*/ 
	virtual void evtClientMuted (const sonar::PeerInfo &client)
	{
		events.push_back(new ClientMutedEvent(client));
	}
	/*[ ]*/ 
	virtual void evtClientTalking (const sonar::PeerInfo &client)
	{
		events.push_back(new ClientTalkingEvent(client));
	}
	/*[ ]*/ 
	virtual void evtCaptureAudio (bool clip, float avgAmp, float peakAmp, bool vaState, bool xmit)
	{
		if (!m_enableCaptureAudio)
			return;

		events.push_back(new CaptureAudioEvent(clip, avgAmp, peakAmp, vaState, xmit));
	}
	/*[ ]*/ 
	virtual void evtPlaybackMuteChanged (bool state)
	{
		events.push_back(new PlaybackMuteChangedEvent(state));
	}
	/*[ ]*/ 
	virtual void evtCaptureMuteChanged (bool state)
	{
		events.push_back(new CaptureMuteChangedEvent(state));
	}
	/*[ ]*/ 
	virtual void evtCaptureDeviceChanged(CString &deviceId, CString &deviceName)
	{
		events.push_back(new CaptureDeviceChangedEvent(deviceId, deviceName));
	}
	/*[ ]*/ 
	virtual void evtPlaybackDeviceChanged(CString &deviceId, CString &deviceName)
	{
		events.push_back(new PlaybackDeviceChangedEvent(deviceId, deviceName));
	}
	/*[ ]*/ 
	virtual void evtLoopbackChanged (bool state)
	{
		events.push_back(new LoopbackChangedEvent(state));
	}
	/*[ ]*/ 
	virtual void evtAutoGainChanged(bool state)
	{
		events.push_back(new AutoGainChangedEvent(state));
	}
	/*[ ]*/ 
	virtual void evtCaptureGainChanged (int percent)
	{
		events.push_back(new CaptureGainChangedEvent(percent));
	}
	/*[ ]*/ 
	virtual void evtPlaybackGainChanged (int percent)
	{
		events.push_back(new PlaybackGainChangedEvent(percent));
	}
	/*[ ]*/ 
	virtual void evtVoiceSensitivityChanged (int percent)
	{
		events.push_back(new VoiceSensitivityChangedEvent(percent));
	}
    /*[ ]*/ 
    virtual void evtChannelInactivity(unsigned long interval)
    {
        events.push_back(new ChannelInactivityEvent(interval));
    }

	void clearEvents()
	{
		for each (Event *evt in events)
		{
			delete evt;
		}

		events.clear();
	}

private:
	String m_token;
	bool m_enableCaptureAudio;

public:
	typedef SonarVector<Event *> EventList;
	EventList events;

};


