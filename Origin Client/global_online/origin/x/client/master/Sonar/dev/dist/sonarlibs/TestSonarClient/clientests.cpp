#include <jlib/Time.h>
#include <jlib/socketdefs.h>
#include <jlib/Thread.h>
#include <SonarClient/Client.h>
#include <SonarInput/Stroke.h>
#include <SonarIpcClient/IpcClient.h>
#include <SonarIpcHost/IpcHost.h>
#include <SonarIpcCommon/IpcUtil.h>
#include <assert.h>
#include <SonarVoiceServer/Server.h>

using namespace sonar;

static void WideAssert(const wchar_t *expr, const wchar_t *file, int line)
{
	wchar_t message[1024 + 1];

	_snwprintf (message, 1024, L"TEST FAILED at %s:%d> %s\n", file, line, expr);
	fwprintf (stderr, L"%s", message);
	fflush(stderr);
	exit(-1);
}

#define ASSERT(_Expression) (void)( (!!(_Expression)) || (WideAssert(_CRT_WIDE(#_Expression), _CRT_WIDE(__FILE__), __LINE__), 0) )

static CString TEST_CHANNEL_DESCRIPTION = "TestChannelDesc";
static CString TEST_CHANNEL_ID_PREFIX = "testchannel";

static CString TEST_REASON_TYPE = "LEAVING";

static int g_serverPort = 22990;

static void InjectKeyboard(int scanCode, bool state)
{
	UINT vk = MapVirtualKeyExW(scanCode, MAPVK_VSC_TO_VK, NULL);
	ASSERT(vk != 0);

	keybd_event( (BYTE) vk, (BYTE) scanCode, state ? 0 : KEYEVENTF_KEYUP, NULL);
}

static void *ServerContainerProc(void *arg);

class ServerContainer
{
public:
	ServerContainer(Server::Config *pConfig = NULL)
	{
		Server::Config config;
		if (pConfig)
		{
			config = *pConfig;
		}

		m_server = new Server("", "", "", "", 1000, 0, config);
		g_serverPort = ntohs(m_server->getVoipAddress().sin_port);
		m_isRunning = true;
		m_thread = jlib::Thread::createThread(ServerContainerProc, this);
	}

	~ServerContainer(void)
	{
		while (m_server->getClientCount() != 0)
		{
			sonar::common::sleepHalfFrame();
		}

		m_isRunning = false;
		m_thread.join();


		ASSERT (m_server->cleanup());
		delete m_server;
	}

	void threadProc(void)
	{
		srand(m_thread.getId() + (unsigned int) time(0));
		while (m_isRunning)
		{
			m_server->poll();
			sonar::common::sleepExact(1);
		}
	}

	const Server &getServer()
	{
		return *m_server;
	}

private:
	Server *m_server;
	bool m_isRunning;

	jlib::Thread m_thread;
};

static void *ServerContainerProc(void *arg)
{
	ServerContainer *sc = (ServerContainer *) arg;
	sc->threadProc();
	return NULL;
}


class Timeout
{
public:

	Timeout()
	{
	}

	Timeout(long long length)
	{
		m_expires = jlib::Time::getTimeAsMSEC() + length;
	}

	bool expired()
	{
		return (jlib::Time::getTimeAsMSEC() > m_expires);
	}

private:
	long long m_expires;

};

class Event
{
public:
	Event(CString &name) 
		: name(name)
	{
	}
	String name;
};

class DisconnectedEvent : public Event
{
public:
	DisconnectedEvent(sonar::CString &reasonType, sonar::CString &reasonDesc) 
		: Event("DisconnectedEvent")
		, reasonType(reasonType)
		, reasonDesc(reasonDesc)
	{}
	String reasonType;
	String reasonDesc;
};

class ConnectedEvent : public Event
{
public:
	ConnectedEvent(int _chid, CString &_channelId, CString _channelDesc) 
		: Event("ConnectedEvent")
		, chid (_chid)
		, channelId(_channelId)
		, channelDesc(_channelDesc)
	{}
	String channelId;
	String channelDesc;
	int chid;
};

class ClientPartedEvent : public Event
{
public:
	ClientPartedEvent(const sonar::PeerInfo &_client, sonar::CString &reasonType, sonar::CString &reasonDesc)
		: Event("ClientPartedEvent")
		, client (_client)
		, reasonType(reasonType)
		, reasonDesc(reasonDesc)
	{}
	sonar::PeerInfo client;
	sonar::String reasonType;
	sonar::String reasonDesc;
};

class ClientJoinedEvent : public Event
{
public:
	ClientJoinedEvent(const sonar::PeerInfo &_client) 
		: Event("ClientJoinedEvent")
		, client (_client)

	{}
	sonar::PeerInfo client;
};

class ClientTalkingEvent : public Event
{
public:
	ClientTalkingEvent(const sonar::PeerInfo &_client) 
		: Event("ClientTalkingEvent")
		, client (_client)

	{}
	sonar::PeerInfo client;
};


class ClientMutedEvent : public Event
{
public:
	ClientMutedEvent(const sonar::PeerInfo &_client) 
		: Event("ClientMutedEvent")
		, client (_client)

	{}
	sonar::PeerInfo client;
};


class CaptureModeChangedEvent : public Event
{
public:
	CaptureModeChangedEvent(const sonar::CaptureMode _mode) 
		: Event("CaptureModeChangedEvent")
		, mode (_mode)

	{}
	sonar::CaptureMode mode;
};

class StrokeRecordedEvent : public Event
{
public:
	StrokeRecordedEvent(CString &_stroke) 
		: Event("StrokeRecordedEvent")
		, stroke (_stroke)

	{}
	String stroke;
};

class BindChangedEvent : public Event
{
public:
	BindChangedEvent(CString &_stroke, bool _block) 
		: Event("BindChangedEvent")
		, stroke (_stroke)
		, block (_block)

	{}
	String stroke;
	bool block;
};

typedef BindChangedEvent PushToTalkKeyChangedEvent;
typedef BindChangedEvent PlaybackMuteKeyChanged;
typedef BindChangedEvent CaptureMuteKeyChanged;

class PushToTalkActiveChangedEvent : public Event
{
public:
    PushToTalkActiveChangedEvent(bool _active) 
        : Event("PushToTalkActiveChangedEvent")
        , active (_active)

    {}
    bool active;
};

class TalkTimeOverLimitEvent : public Event
{
public:
    TalkTimeOverLimitEvent(bool _over) 
        : Event("TalkTimeOverLimitEvent")
        , over (_over)

    {}
    bool over;
};

class ChannelInactivityEvent : public Event
{
public:
    ChannelInactivityEvent(unsigned long _interval) 
        : Event("ChannelInactivityEvent")
        , interval (_interval)

    {}
    unsigned long interval;
};

class CaptureAudioEvent : public Event
{
public:
	CaptureAudioEvent(bool _clip, float _avgAmp, float _peakAmp, bool _vaState, bool _xmit) 
		: Event("CaptureAudioEvent")
		, clip (_clip)
		, avgAmp (_avgAmp)
		, peakAmp (_peakAmp)
		, vaState (_vaState)
		, xmit (_xmit)

	{}
	bool clip;
	float avgAmp;
	float peakAmp;
	bool vaState;
	bool xmit;
};

class AudioGainEvent : public Event
{
public:
	AudioGainEvent(int _percent) 
		: Event("AudioGainEvent")
		, percent (_percent)

	{}
	int percent;
};

typedef AudioGainEvent PlaybackGainChangedEvent;
typedef AudioGainEvent CaptureGainChangedEvent;
typedef AudioGainEvent VoiceSensitivityChangedEvent;


class StateChangedEvent : public Event
{
public:
	StateChangedEvent(bool _state)
		: Event("StateChangedEvent")
		, state (_state)
	{}
	bool state;
};

typedef StateChangedEvent PlaybackMuteChangedEvent;
typedef StateChangedEvent CaptureMuteChangedEvent;
typedef StateChangedEvent LoopbackChangedEvent;
typedef StateChangedEvent AutoGainChangedEvent;

class ErrorEvent : public Event
{
public:
	ErrorEvent(CString &_reasonType, CString &_reasonDesc)
		: Event("ErrorEvent")
		, reasonType (_reasonType)
		, reasonDesc (_reasonDesc)
	{}
	String reasonType;
	String reasonDesc;
};

class AudioDeviceChangedEvent : public Event
{
public:
	AudioDeviceChangedEvent(CString &_id, CString &_name)
		: Event("AudioDeviceChangedEvent")
		, id (_id)
		, name (_name)
	{}
	
	String id;
	String name;
};

typedef AudioDeviceChangedEvent PlaybackDeviceChangedEvent;
typedef AudioDeviceChangedEvent CaptureDeviceChangedEvent;

#ifdef SONAR_CLIENT_TESTS
#include <TestSonarClient/TestClient.h>
#else
#ifdef SONAR_IPC_CLIENT_TESTS
#include <TestSonarIpc/IpcTestClient.h>
#endif
#endif

static String MakeToken(int id, CString &channelPrefix = TEST_CHANNEL_ID_PREFIX)
{
	sonar::StringStream ss;

	int cid = (int) jlib::Thread::getCurrentId();

	ss << "SONAR2|127.0.0.1:" << g_serverPort << "|127.0.0.1:" << g_serverPort << "|default|user";
	ss << id;
	ss << "|User";
	ss << id;
	ss << "|";
	ss << channelPrefix;
	ss << cid;
	ss << "|";
	ss << TEST_CHANNEL_DESCRIPTION;
	ss << "||";
	ss << time(0);
	ss << "|";

	return ss.str();
}

#define WHILE(_client, _timeout, _expression) \
	{ \
		Timeout _wnt((_timeout)); \
		while(!_wnt.expired() && (_expression)) \
		{ \
		(_client).update(); \
		} \
		ASSERT(!_wnt.expired()); \
		ASSERT(!(_expression)); \
	} \

void DisconnectClient(TestClient &client)
{
	Timeout t(10000);

	ASSERT(client.disconnect(TEST_REASON_TYPE, ""));

	while (!t.expired() && !client.isDisconnected())
	{
		client.update();
		sonar::common::sleepFrame();
	}

	ASSERT(!t.expired());
}

void ConnectClient(TestClient &client, int id = 0)
{
	Timeout t(10000);

	String token = MakeToken(jlib::Thread::getCurrentId() + id);

	ASSERT(client.connect(token));

	while (!t.expired() && !client.isConnected())
	{
		client.update();
		sonar::common::sleepFrame();
	}

	ASSERT(!t.expired());
}

void test_ConnectAndDisconnect(void)
{
	ServerContainer sc;

	TestClient client;

	String token = MakeToken(jlib::Thread::getCurrentId() + 0);

	ASSERT(client.connect(token));

	Timeout t(10000);

	while (!t.expired() && !client.isConnected())
	{
		client.update();
		sonar::common::sleepFrame();
	}

	ASSERT(!client.connect(token));
	DisconnectClient(client);

	WHILE(client, 1000, client.events.size() < 2);
	ConnectedEvent *evtConnected = (ConnectedEvent *) client.events[0];
	DisconnectedEvent *evtDisconnected = (DisconnectedEvent *) client.events[1];
	
	ASSERT(evtConnected->channelId == client.getToken().getChannelId());
	ASSERT(evtConnected->channelDesc == client.getToken().getChannelDesc());
	ASSERT(evtConnected->chid == 0);

	ASSERT(evtDisconnected->reasonType == TEST_REASON_TYPE);
}

void test_DoubleConnect(void)
{
	ServerContainer sc;

	TestClient client;

	ConnectClient(client);
	ASSERT(!client.connect( MakeToken(jlib::Thread::getCurrentId() + 0)));
	DisconnectClient(client);

}

void test_ConnectDisconnectTwoTimes(void)
{
	ServerContainer sc;

	TestClient client;
	ConnectClient(client);
	DisconnectClient(client);
	ConnectClient(client);
	DisconnectClient(client);
}

void test_ReplaceClient(void)
{
	ServerContainer sc;

	TestClient client1;
	TestClient client2;

	ConnectClient(client1);
	ConnectClient(client2);

	WHILE(client1, 1000, !client1.isDisconnected()); 
	DisconnectClient(client2);
}


void test_DisconnectReasonClientParted(void)
{
	ServerContainer sc;

	TestClient client1;
	TestClient client2;

	ConnectClient(client1, 0);
	ConnectClient(client2, 1);
	DisconnectClient(client1);
	WHILE(client2, 10000, client2.events.size() < 2);

	ClientPartedEvent *evt = (ClientPartedEvent *) client2.events[1];
	ASSERT(evt->reasonType == TEST_REASON_TYPE);

	DisconnectClient(client2);
}

void test_MuteClient(void)
{
	ServerContainer sc;

	TestClient client1;
	TestClient client2;

	ConnectClient(client1, 0);
	ConnectClient(client2, 1);

	WHILE(client1, 10000, client1.events.size() < 2);

	client1.muteClient(client2.getToken().getUserId(), true);

	WHILE(client1, 10000, client1.events.size() < 3);

	ConnectedEvent *evtConnected = (ConnectedEvent *) client1.events[0];
	ClientJoinedEvent *evtClientJoined = (ClientJoinedEvent *) client1.events[1];
	ClientMutedEvent *evtClientMuted = (ClientMutedEvent *) client1.events[2];
	
	ASSERT(evtClientJoined->client.isMuted == false);
	ASSERT(evtClientMuted->client.isMuted == true);
	ASSERT(evtClientMuted->client.userId == client2.getToken().getUserId());
	ASSERT(evtClientMuted->client.userDesc == client2.getToken().getUserDesc());

	DisconnectClient(client1);
	DisconnectClient(client2);
}

void test_ClientJoined(void)
{
	ServerContainer sc;

	TestClient client1;
	TestClient client2;

	ConnectClient(client1, 0);
	ConnectClient(client2, 1);

	WHILE(client1, 10000, client1.events.size() < 2);
	WHILE(client1, 10000, client1.events.size() < 2);

	ConnectedEvent *evtConnected = (ConnectedEvent *) client1.events[0];
	ClientJoinedEvent *evtClientJoined = (ClientJoinedEvent *) client1.events[1];
	
	ASSERT(evtClientJoined->client.isMuted == false);
	ASSERT(evtClientJoined->client.userId == client2.getToken().getUserId());
	ASSERT(evtClientJoined->client.userDesc == client2.getToken().getUserDesc());

	DisconnectClient(client1);
	DisconnectClient(client2);
}

void test_GetSetCaptureMode(void)
{
	ServerContainer sc;

	TestClient client;
	
	for (int index = 0; index < 3; index ++)
	{
		sonar::CaptureMode cpm = (sonar::CaptureMode) index;
		client.setCaptureMode(cpm);
		ASSERT(client.getCaptureMode() == cpm);

		WHILE(client, 10000, client.events.size() < 1);

		CaptureModeChangedEvent *evt = (CaptureModeChangedEvent *) client.events[0];
		ASSERT(evt->mode == cpm);

		client.clearEvents();
	}
}

void test_AsyncRecordStroke(void)
{
	TestClient client;
	
	client.recordStroke();

	InjectKeyboard(0x1d, true); // CTRL
	InjectKeyboard(0x2a, true); // SHIFT
	InjectKeyboard(0x31, true); // N
	InjectKeyboard(0x31, false); // N
	InjectKeyboard(0x2a, false); // SHIFT
	InjectKeyboard(0x1d, false); // CTRL
		
	WHILE(client, 10000, client.events.size() < 1);

	StrokeRecordedEvent *evtStrokeRecorded = (StrokeRecordedEvent *) client.events[0];

	ASSERT(evtStrokeRecorded->stroke == "Ctrl + Shift + N");
}

void test_bindPushToTalkKey(void)
{
	ServerContainer sc;

	TestClient client;

	CString stroke = "Ctrl + Shift + N";

	client.bindPushToTalkKey(stroke, true);

	WHILE(client, 10000, client.events.size() < 1);

	PushToTalkKeyChangedEvent *evt = (PushToTalkKeyChangedEvent *) client.events[0];

	ASSERT(evt->stroke == stroke);
	ASSERT(client.getPushToTalkKeyStroke() == stroke);
	ASSERT(client.getPushToTalkKeyBlock() == true);

	client.setCaptureMode(sonar::PushToTalk);
	client.clearEvents();
	client.enableCaptureAudio(true);

	WHILE(client, 1000, client.events.size() < 10);
	client.clearEvents();
	WHILE(client, 1000, client.events.size() < 10);

	for (int index = 0; index < 10; index ++)
	{
		CaptureAudioEvent *evt = (CaptureAudioEvent *) client.events[index];
		ASSERT(evt->xmit == false);
	}

	client.clearEvents();

	InjectKeyboard(0x1d, true); // CTRL
	InjectKeyboard(0x2a, true); // SHIFT
	InjectKeyboard(0x31, true); // N

	WHILE(client, 1000, client.events.size() < 10);

	for (int index = 0; index < 10; index ++)
	{
		CaptureAudioEvent *evt = (CaptureAudioEvent *) client.events[index];
		ASSERT(evt->xmit == true);
	}

	InjectKeyboard(0x31, false); // N
	InjectKeyboard(0x2a, false); // SHIFT
	InjectKeyboard(0x1d, false); // CTRL

	WHILE(client, 3000, client.events.size() < 100);
	client.clearEvents();

	WHILE(client, 1000, client.events.size() < 10);

	for (int index = 0; index < 10; index ++)
	{
		CaptureAudioEvent *evt = (CaptureAudioEvent *) client.events[index];
		ASSERT(evt->xmit == false);
	}
}

void test_bindStrokeTrim(void)
{
	CString strokeKeys = " Ctrl   + A    ";
	sonar::Stroke stroke(strokeKeys, false);

	CString result = stroke.toString();

	ASSERT(result == "Ctrl + A");
}

void test_bindPlaybackMuteKey(void)
{
	ServerContainer sc;

	TestClient client;
	PlaybackMuteChangedEvent *evtMute;
	CString stroke = "Ctrl";

	client.bindPlaybackMuteKey(stroke, true);

	WHILE(client, 10000, client.events.size() < 1);

	PlaybackMuteKeyChanged *evt = (PlaybackMuteKeyChanged *) client.events[0];

	ASSERT(evt->stroke == stroke);
	ASSERT(client.getPlaybackMuteKeyStroke() == stroke);
	ASSERT(client.getPlaybackMuteKeyBlock() == true);
	ASSERT(client.getPlaybackMute() == false);

	client.clearEvents();
	InjectKeyboard(0x1d, true); // CTRL
	WHILE(client, 1000, client.events.size() < 1);
	InjectKeyboard(0x1d, false); // CTRL

	evtMute = (PlaybackMuteChangedEvent *) client.events[0];
	ASSERT(evtMute->state == true);
	ASSERT(client.getPlaybackMute() == true);

	client.clearEvents();
	sonar::common::sleepExact(100);
	client.update();
	InjectKeyboard(0x1d, true); // CTRL
	WHILE(client, 1000, client.events.size() < 1);
	InjectKeyboard(0x1d, false); // CTRL

	evtMute = (PlaybackMuteChangedEvent *) client.events[0];
	ASSERT(evtMute->state == false);
	ASSERT(client.getPlaybackMute() == false);
}

void test_bindCaptureMuteKey(void)
{
	ServerContainer sc;

	TestClient client;
	PlaybackMuteChangedEvent *evtMute;
	CString stroke = "Ctrl";

	client.bindPlaybackMuteKey(stroke, true);

	WHILE(client, 10000, client.events.size() < 1);

	PlaybackMuteKeyChanged *evt = (PlaybackMuteKeyChanged *) client.events[0];

	ASSERT(evt->stroke == stroke);
	ASSERT(client.getPlaybackMuteKeyStroke() == stroke);
	ASSERT(client.getPlaybackMuteKeyBlock() == true);
	ASSERT(client.getPlaybackMute() == false);

	client.clearEvents();
	InjectKeyboard(0x1d, true); // CTRL
	WHILE(client, 1000, client.events.size() < 1);
	InjectKeyboard(0x1d, false); // CTRL

	evtMute = (PlaybackMuteChangedEvent *) client.events[0];
	ASSERT(evtMute->state == true);
	ASSERT(client.getPlaybackMute() == true);

	client.clearEvents();
	sonar::common::sleepExact(100);
	client.update();
	InjectKeyboard(0x1d, true); // CTRL
	WHILE(client, 1000, client.events.size() < 1);
	InjectKeyboard(0x1d, false); // CTRL

	evtMute = (PlaybackMuteChangedEvent *) client.events[0];
	ASSERT(evtMute->state == false);
	ASSERT(client.getPlaybackMute() == false);
}
void test_GetClients(void)
{
	ServerContainer sc;

	TestClient clients[4];

	for (int index = 0; index < 4; index ++)
	{
		ConnectClient(clients[index], index);
	}

	TestClient &client = clients[3];

	WHILE(client, 10000, client.getClients().size() < 4);
	
	for (int index = 0; index < 4; index ++)
	{
		DisconnectClient(clients[index]);
	}
}

void test_GetSetPlaybackDevice(void)
{
	ServerContainer sc;

	TestClient client;

	sonar::AudioDeviceId oldDevice = client.getPlaybackDevice();

	ASSERT(!oldDevice.id.empty());

	sonar::AudioDeviceList deviceList = client.getPlaybackDevices();

	ASSERT(deviceList.size() > 1);

	sonar::AudioDeviceId newDevice;
	
	for (sonar::AudioDeviceList::iterator iter = deviceList.begin(); iter != deviceList.end(); iter ++)
	{
		if (iter->id != oldDevice.id)
		{
			newDevice = *iter;
			break;
		}
	}

	client.setPlaybackDevice(newDevice.id);
	WHILE(client, 10000, client.events.size() != 1);

	PlaybackDeviceChangedEvent *evt = (PlaybackDeviceChangedEvent *) client.events[0];
	
	ASSERT(evt->id == newDevice.id);
	ASSERT(evt->name == newDevice.name);

	sonar::AudioDeviceId currDevice = client.getPlaybackDevice();

	ASSERT(currDevice.id == newDevice.id);
	ASSERT(currDevice.name == newDevice.name);
}

void test_GetSetCaptureDevice(void)
{
	ServerContainer sc;

	TestClient client;

	sonar::AudioDeviceId oldDevice = client.getCaptureDevice();

	ASSERT(!oldDevice.id.empty());

	sonar::AudioDeviceList deviceList = client.getCaptureDevices();

	ASSERT(deviceList.size() > 1);

	sonar::AudioDeviceId newDevice;
	
	for (sonar::AudioDeviceList::iterator iter = deviceList.begin(); iter != deviceList.end(); iter ++)
	{
		if (iter->id != oldDevice.id)
		{
			newDevice = *iter;
			break;
		}
	}

	client.setCaptureDevice(newDevice.id);
	WHILE(client, 10000, client.events.size() != 1);

	CaptureDeviceChangedEvent *evt = (CaptureDeviceChangedEvent *) client.events[0];
	
	ASSERT(evt->id == newDevice.id);
	ASSERT(evt->name == newDevice.name);

	sonar::AudioDeviceId currDevice = client.getCaptureDevice();

	ASSERT(currDevice.id == newDevice.id);
	ASSERT(currDevice.name == newDevice.name);
}

void test_GetSetLoopback(void)
{
	ServerContainer sc;

	LoopbackChangedEvent *evt;

	TestClient client;
	bool oldState = client.getLoopback();

	ASSERT(!oldState);

	client.setLoopback(true);
	ASSERT(client.getLoopback() == true);
	WHILE(client, 10000, client.events.size() != 1);
	evt = (LoopbackChangedEvent *) client.events[0];
	ASSERT(evt->state == true);
	client.clearEvents();

	client.setLoopback(false);
	ASSERT(client.getLoopback() == false);
	WHILE(client, 10000, client.events.size() != 1);
	evt = (LoopbackChangedEvent *) client.events[0];
	ASSERT(evt->state == false);
}

void test_GetSetAutoGain(void)
{
	ServerContainer sc;

	AutoGainChangedEvent *evt;

	TestClient client;
	bool oldState = client.getAutoGain();
	bool newState = !oldState;

	client.setAutoGain(newState);
	ASSERT(client.getAutoGain() == newState);
	WHILE(client, 10000, client.events.size() != 1);
	evt = (AutoGainChangedEvent *) client.events[0];
	ASSERT(evt->state == newState);
	client.clearEvents();

	client.setAutoGain(oldState);
	ASSERT(client.getAutoGain() == oldState);
	WHILE(client, 10000, client.events.size() != 1);
	evt = (AutoGainChangedEvent *) client.events[0];
	ASSERT(evt->state == oldState);
}

void test_GetSetPlaybackGain(void)
{
	ServerContainer sc;

	PlaybackGainChangedEvent *evt;

	TestClient client;

	int oldGain = client.getPlaybackGain();
	int newGain = 43;

	client.setPlaybackGain(newGain);
	ASSERT(client.getPlaybackGain() == newGain);
	WHILE(client, 10000, client.events.size() != 1);
	evt = (PlaybackGainChangedEvent *) client.events[0];
	ASSERT(evt->percent == newGain);
	client.clearEvents();

	client.setPlaybackGain(oldGain);
	ASSERT(client.getPlaybackGain() == oldGain);
	WHILE(client, 10000, client.events.size() != 1);
	evt = (PlaybackGainChangedEvent *) client.events[0];
	ASSERT(evt->percent == oldGain);
}

void test_GetSetCaptureGain(void)
{
	ServerContainer sc;

	CaptureGainChangedEvent *evt;

	TestClient client;

	int oldGain = client.getCaptureGain();
	int newGain = 43;

	client.setCaptureGain(newGain);
	ASSERT(client.getCaptureGain() == newGain);
	WHILE(client, 10000, client.events.size() != 1);
	evt = (CaptureGainChangedEvent *) client.events[0];
	ASSERT(evt->percent == newGain);
	client.clearEvents();

	client.setCaptureGain(oldGain);
	ASSERT(client.getCaptureGain() == oldGain);
	WHILE(client, 10000, client.events.size() != 1);
	evt = (CaptureGainChangedEvent *) client.events[0];
	ASSERT(evt->percent == oldGain);
}

void test_GetSetVoiceSensitivity(void)
{
	ServerContainer sc;

	VoiceSensitivityChangedEvent *evt;

	TestClient client;

	int oldSensitivity = client.getVoiceSensitivity();
	int newSensitivity = 43;

	client.setVoiceSensitivity(newSensitivity);
	ASSERT(client.getVoiceSensitivity() == newSensitivity);
	WHILE(client, 10000, client.events.size() != 1);
	evt = (VoiceSensitivityChangedEvent *) client.events[0];
	ASSERT(evt->percent == newSensitivity);
	client.clearEvents();

	client.setVoiceSensitivity(oldSensitivity);
	ASSERT(client.getVoiceSensitivity() == oldSensitivity);
	WHILE(client, 10000, client.events.size() != 1);
	evt = (VoiceSensitivityChangedEvent *) client.events[0];
	ASSERT(evt->percent == oldSensitivity);
}

void test_TokenArguments(void)
{
	ServerContainer sc;

	TestClient client;

	ASSERT(client.getUserId() == "");
	ASSERT(client.getUserDesc() == "");
	ASSERT(client.getChannelId() == "");
	ASSERT(client.getChannelDesc() == "");
	ASSERT(client.getChid() == -1);

	ConnectClient(client);

	ASSERT(client.getUserId() == client.getToken().getUserId());
	ASSERT(client.getUserDesc() == client.getToken().getUserDesc());
	ASSERT(client.getChannelId() == client.getToken().getChannelId());
	ASSERT(client.getChannelDesc() == client.getToken().getChannelDesc());
	ASSERT(client.getChid() >= 0);

	DisconnectClient(client);

	ASSERT(client.getChannelId() == "");
	ASSERT(client.getChannelDesc() == "");
	ASSERT(client.getChid() == -1);
}

void test_SoftPushToTalk(void)
{
	ServerContainer sc;

	Timeout t;
	TestClient client;
	ConnectClient(client);
	
	client.setCaptureMode(sonar::PushToTalk);

	WHILE(client, 1000, client.events.size() < 2);
	client.clearEvents();

	ASSERT(client.events.empty());

	client.setSoftPushToTalk(true);
	t = Timeout(500);
	while (!t.expired())
	{
		client.update();
		sonar::common::sleepFrame();
	}

	client.setSoftPushToTalk(false);
	t = Timeout(3000);
	while (!t.expired())
	{
		client.update();
		sonar::common::sleepFrame();
	}

	ASSERT(client.events.size() == 2);

	ClientTalkingEvent *t1 = (ClientTalkingEvent *) client.events[0];
	ClientTalkingEvent *t2 = (ClientTalkingEvent *) client.events[1];

	ASSERT(t1->client.isTalking);
	ASSERT(!t2->client.isTalking);

	DisconnectClient(client);
}

#ifdef SONAR_CLIENT_TESTS

void test_TalkBalanceClient(void)
{
	ServerContainer sc;

	Timeout t;
	TestClient client;
	
	ClientConfig config = client.getConfig();
	config.set("talkBalanceMax", (1000 / protocol::FRAME_TIMESLICE_MSEC) / 2);
	config.set("talkBalancePenalty", (1000 / protocol::FRAME_TIMESLICE_MSEC) / 2);
	config.set("talkBalanceMin", (1000 / protocol::FRAME_TIMESLICE_MSEC) / 4);
	client.setConfig(config);
	
	ConnectClient(client);
	
	client.setCaptureMode(sonar::PushToTalk);
	WHILE(client, 1000, client.events.size() < 2);
	client.clearEvents();
	ASSERT(client.events.empty());
	client.setSoftPushToTalk(true);

	t = Timeout(1000);

	while (!t.expired())
	{
		client.update();
		sonar::common::sleepFrame();
	}

	ASSERT(((ClientTalkingEvent *) client.events[0])->client.isTalking);
	ASSERT(!((ClientTalkingEvent *) client.events[1])->client.isTalking);
	
	int minCount = config.get("talkBalanceMin", -1);

  for (int x = 0; x < minCount; x ++)
		sonar::common::sleepFrame();

	t = Timeout(1000);

	while (!t.expired())
	{
		client.update();
		sonar::common::sleepFrame();
	}

	ASSERT(((ClientTalkingEvent *) client.events[0])->client.isTalking);
	ASSERT(!((ClientTalkingEvent *) client.events[1])->client.isTalking);

	DisconnectClient(client);
}

#endif

void test_PushToTalking(void)
{
	ServerContainer sc;

	TestClient clients[4];

	for (int index = 0; index < 4; index ++)
	{
		clients[index].bindPushToTalkKey("Ctrl", false);
		clients[index].setCaptureMode(sonar::PushToTalk);
		clients[index].setCaptureMute(true);
		ConnectClient(clients[index], index);
	}
	

	for (int talker = 0; talker < 4; talker ++)
	{
		clients[talker].setCaptureMute(false);

		InjectKeyboard(0x1d, true);

    for (int frame = 0; frame < protocol::FRAMES_PER_SECOND; frame ++)
		{
			for (int index = 0; index < 4; index ++)
			{
				clients[index].update();
			}

			sonar::common::sleepFrame();
		}

		InjectKeyboard(0x1d, false);
		clients[talker].setCaptureMute(true);
	}

	for (int frame = 0; frame < protocol::FRAMES_PER_SECOND * 2; frame ++)
	{
		for (int index = 0; index < 4; index ++)
			clients[index].update();

		sonar::common::sleepFrame();
	}

	for (int index = 0; index < 4; index ++)
	{
		TestClient &client = clients[index];
		DisconnectClient(client);
				
		int talkBegin = 0;
		int talkEnd = 0;
		
		for (size_t index2 = 0; index2 < client.events.size(); index2++)
		{
			if (client.events[index2]->name == "ClientTalkingEvent")
			{
				ClientTalkingEvent *evt = (ClientTalkingEvent *) client.events[index2];

				if (evt->client.isTalking)
					talkBegin ++;
				else
					talkEnd ++;
			}
		}

		ASSERT (talkBegin == talkEnd && talkEnd == 4);
	}


}


void test_Config(void)
{
#ifdef SONAR_CLIENT_TESTS
	TestClient client;

	client.setAutoGain(false);
	client.setCaptureDevice("31337");
	client.setCaptureGain(13);
	client.setCaptureMode(sonar::PushToTalk);
	client.setPlaybackDevice("31338");
	client.setPlaybackGain(false);
	client.setVoiceSensitivity(14);
	
	client.getConfig().save("./test.cfg");
	
	sonar::ClientConfig config2;
	config2.load("./test.cfg");

	TestClient client2;
	client2.setConfig(config2);

	ASSERT(client.getAutoGain() == client2.getAutoGain());
	ASSERT(client.getCaptureDevice().id == client2.getCaptureDevice().id);
	ASSERT(client.getCaptureGain() == client2.getCaptureGain());
	ASSERT(client.getCaptureMode() == client2.getCaptureMode());
	ASSERT(client.getPlaybackDevice().id == client2.getPlaybackDevice().id);
	ASSERT(client.getPlaybackGain() == client2.getPlaybackGain());
	ASSERT(client.getVoiceSensitivity() == client2.getVoiceSensitivity());

	client.setAutoGain(true);
	client.setCaptureDevice("DEFAULT");
	client.setCaptureGain(19);
	client.setCaptureMode(sonar::VoiceActivation);
	client.setPlaybackDevice("DEFAULT");
	client.setPlaybackGain(true);
	client.setVoiceSensitivity(15);

	client.getConfig().save("./test.cfg");

	sonar::ClientConfig config3;
	config3.load("./test.cfg");

	TestClient client3;
	client3.setConfig(config3);

	ASSERT(client.getAutoGain() == client3.getAutoGain());
	ASSERT(client.getCaptureDevice().id == client3.getCaptureDevice().id);
	ASSERT(client.getCaptureGain() == client3.getCaptureGain());
	ASSERT(client.getCaptureMode() == client3.getCaptureMode());
	ASSERT(client.getPlaybackDevice().id == client3.getPlaybackDevice().id);
	ASSERT(client.getPlaybackGain() == client3.getPlaybackGain());
	ASSERT(client.getVoiceSensitivity() == client3.getVoiceSensitivity());
#endif
}

#define RUN_TEST(_func) \
	fprintf (stderr, "%s is running\n", #_func);\
	_func();\
	fprintf (stderr, "%s is done\n", #_func);

int wmain(int argc, wchar_t **argv)
{
	//_set_abort_behavior(0, _WRITE_ABORT_MSG );

#ifdef _WIN32
	WSADATA wsaData;
	::WSAStartup(MAKEWORD(2,2), &wsaData);
	::CoInitialize(NULL);
	ASSERT(::LoadKeyboardLayoutW(L"00000409", KLF_ACTIVATE | KLF_SETFORPROCESS));
#endif

	argv ++;
	argc --;

	SetEnvironmentVariableA("SONAR_IGNORE_REAL_INPUT", "1");

	if (getenv("SONAR_TEST_VOICE_PORT"))
	{
		g_serverPort = atoi(getenv("SONAR_TEST_VOICE_PORT"));
	}

	fprintf (stderr, "SONAR_TEST_VOICE_PORT: %d\n", g_serverPort);

#ifdef SONAR_CLIENT_TESTS
	RUN_TEST(test_TalkBalanceClient);
#endif

	RUN_TEST(test_bindPlaybackMuteKey);
	RUN_TEST(test_TokenArguments);
	RUN_TEST(test_ConnectAndDisconnect);
	RUN_TEST(test_Config);
	RUN_TEST(test_bindStrokeTrim);
	RUN_TEST(test_ReplaceClient);
	RUN_TEST(test_SoftPushToTalk);
	RUN_TEST(test_ConnectDisconnectTwoTimes);
	RUN_TEST(test_DoubleConnect);
	RUN_TEST(test_DisconnectReasonClientParted);
	RUN_TEST(test_MuteClient);
	RUN_TEST(test_ClientJoined);
	RUN_TEST(test_GetSetCaptureMode);
	RUN_TEST(test_AsyncRecordStroke);
	RUN_TEST(test_bindPushToTalkKey);
	RUN_TEST(test_bindCaptureMuteKey);
	RUN_TEST(test_GetClients);
	RUN_TEST(test_GetSetPlaybackDevice);
	RUN_TEST(test_GetSetCaptureDevice);
	RUN_TEST(test_GetSetLoopback);
	RUN_TEST(test_GetSetAutoGain);
	RUN_TEST(test_GetSetPlaybackGain);
	RUN_TEST(test_GetSetCaptureGain);
	RUN_TEST(test_GetSetVoiceSensitivity)
	RUN_TEST(test_PushToTalking);
	
	fprintf (stderr, "DONE!\n");

	return 0;
}