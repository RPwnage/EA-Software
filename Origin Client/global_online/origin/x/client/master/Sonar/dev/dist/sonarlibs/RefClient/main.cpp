#include <SonarClient\Client.h>
#include <jlib/Thread.h>
#include <conio.h>
#include <time.h>
#include <assert.h>
#include <TestUtils/TokenSigner.h>

#define USE_VOICE_ACTIVATION 1

static sonar::String MakeToken(int id, int port, const char *channelPrefix)
{
	sonar::StringStream ss;

	//ss << "SONAR2|127.0.0.1:22990|127.0.0.1:22990|default|user";
    ss << "SONAR2|127.0.0.1:";
    ss << port;
    ss << "|127.0.0.1:";
    ss << port;
    ss << "|default|user";
	ss << id;
	ss << "|User";
	ss << id;
	ss << "|";
	ss << getenv("COMPUTERNAME");
	ss << "-";
	ss << channelPrefix;
	ss << "|";
	ss << "";
	ss << "||";
	ss << time(0);
	ss << "|";

	return ss.str();
}

static sonar::String MakeTokenEx(const char *serverAddress, int id, const char *channelPrefix)
{
    sonar::StringStream ss;

    ss << "SONAR2||";
    ss << serverAddress;
    ss << "|default|user";
    ss << id;
    ss << "|User";
    ss << id;
    ss << "|";
    ss << "SGIENG-SERVER";
    ss << "-";
    ss << channelPrefix;
    ss << "|";
    ss << "";
    ss << "||";
    ss << time(0);
    ss << "|";

    return ss.str();
}

class EventListener : public sonar::IClientEvents
{
public:

	EventListener(sonar::Client *client)
	{
		m_client = client;
	}

	virtual void evtStrokeRecorded(sonar::CString &stroke){}
	virtual void evtCaptureModeChanged(sonar::CaptureMode mode){}
	virtual void evtPlaybackMuteKeyChanged(sonar::CString &stroke, bool block){}
	virtual void evtCaptureMuteKeyChanged(sonar::CString &stroke, bool block){}
	virtual void evtPushToTalkKeyChanged(sonar::CString &stroke, bool block){}
    virtual void evtPushToTalkActiveChanged(bool active){}
    virtual void evtTalkTimeOverLimit(bool over){}
	virtual void evtConnected(int chid, sonar::CString &channelId, sonar::CString channelDesc)
	{
		fprintf (stderr, ">> Connected as %s to channel %s\n", m_client->getUserId().c_str(), channelId.c_str());
	}

	virtual void evtDisconnected(sonar::CString &reasonType, sonar::CString &reasonDesc)
	{
		fprintf (stderr, ">> Disconnected, %s/%s\n", reasonType.c_str(), reasonDesc.c_str());
	}

	virtual void evtShutdown(void){}
	virtual void evtClientJoined (const sonar::PeerInfo &client)
	{
		fprintf (stderr, ">> %s joined the channel\n", client.userId.c_str());
	}
	virtual void evtClientParted (const sonar::PeerInfo &client, sonar::CString &reasonType, sonar::CString &reasonDesc)
	{
		fprintf (stderr, ">> %s left the channel (%s/%s)\n", client.userId.c_str(), reasonType.c_str(), reasonDesc.c_str());
	}
	virtual void evtClientMuted (const sonar::PeerInfo &client){}

	virtual void evtClientTalking (const sonar::PeerInfo &client)
	{
		fprintf (stderr, ">> %s is %s\n", client.userId.c_str(), client.isTalking ? "talking" : "not talking");
	}
	virtual void evtCaptureAudio (bool clip, float avgAmp, float peakAmp, bool vaState, bool xmit){}
	virtual void evtPlaybackMuteChanged (bool state){}
	virtual void evtCaptureMuteChanged (bool state){}
	virtual void evtCaptureDeviceChanged(sonar::CString &deviceId, sonar::CString &deviceName){}
	virtual void evtPlaybackDeviceChanged(sonar::CString &deviceId, sonar::CString &deviceName){}
	virtual void evtLoopbackChanged (bool state){}
	virtual void evtAutoGainChanged(bool state){}
	virtual void evtCaptureGainChanged (int percent){}
	virtual void evtPlaybackGainChanged (int percent){}
	virtual void evtVoiceSensitivityChanged (int percent){}
    virtual void evtChannelInactivity(unsigned long interval){}

private:

	sonar::Client *m_client;
};

static void usage(void)
{
	fprintf (stderr, "Usage: <channeltoken>\n"
		"<channelToken> - A channel token.\n"
        "Use [--local <port> | --serveraddress <address:port>]\n"
        "  local: to generate and use a 127.0.0.1 valid token with specified port\n"
        "  serveraddress: to generate and use <address:port> valid token\n"
		"Use --loss <int> to generate percent packet loss\n"
		"Use --remoteecho to hear your own audio through the server\n"
        "Use --useprivatekey to use 'sonarmaster.prv' in user home to sign tokens\n"
		"--help shows this information\n");
}

#if !USE_VOICE_ACTIVATION
volatile bool g_pttState = false;

static void *inputThread(void *arg)
{
	for(;;)
	{
		getchar();
		g_pttState = !g_pttState;
	}
}
#endif

int main(int argc, char **argv)
{
	fprintf (stderr, "Sonar Reference Client\n*** FOR INTERNAL USE ONLY ***\n\n");

#ifdef _WIN32
	::CoInitialize(NULL);
	WSADATA wsaData;
	::WSAStartup(MAKEWORD(2,2), &wsaData);
	::LoadKeyboardLayout("00000409", KLF_ACTIVATE | KLF_SETFORPROCESS);
#endif

	argc --;
	argv ++;

	bool remoteEcho = false;
	int loss = 0;
	sonar::String channelToken;
    bool usePrivateKey = false;

	for (int index = 0; index < argc; index ++)
	{
		if (strcmp(argv[index], "--local") == 0)
		{
            int port = atoi(argv[index + 1]);
			channelToken = MakeToken( (int) ::GetCurrentProcessId(), port, "LocalChannel");
		}
        else if(strcmp(argv[index], "--serveraddress") == 0)
        {
            channelToken = MakeTokenEx( argv[index + 1], (int) ::GetCurrentProcessId(), "LocalChannel");
        }
		else
		if (strcmp(argv[index], "--loss") == 0)
		{
			loss = atoi(argv[index + 1]);
		}
		else
		if (strcmp(argv[index], "--remoteecho") == 0)
		{
			remoteEcho = true;
		}
		else
		if (strncmp(argv[index], "SONAR2|", 7) == 0)
		{
			channelToken = argv[index];
		}
        else
        if (strcmp(argv[index], "--useprivatekey") == 0)
        {
            usePrivateKey = true;
        }
		else
		if (strcmp(argv[index], "--help") == 0)
		{
			usage();
			return 0;
		}
	}

	if (channelToken.empty())
	{
		usage();
		return -1;
	}

    if (usePrivateKey)
    {
        sonar::TokenSigner *tokenSigner = new sonar::TokenSigner();
        assert(tokenSigner);

        sonar::Token token;
        token.parse(channelToken);
        channelToken = tokenSigner->sign(token);

        delete tokenSigner;
    }

	sonar::Client client;

	sonar::ClientConfig config = client.getConfig();
	config.set("remoteEcho", remoteEcho);
	client.setConfig(config);

	fprintf(stderr, "Using token: %s\n", channelToken.c_str());
	fprintf(stderr, "Remote echo: %s\n", remoteEcho ? "Enabled": "Disabled");
	if (loss > 0)
	{
		fprintf(stderr, "Enabling duplex packet loss of %d %%\n", loss);

		sonar::Udp4Transport::m_rxPacketLoss = loss;
		sonar::Udp4Transport::m_txPacketLoss = loss;
	}
	
	client.addEventListener(new EventListener(&client));
#if USE_VOICE_ACTIVATION
    client.setCaptureMode(sonar::VoiceActivation);
#else
    client.setCaptureMode(sonar::PushToTalk);
#endif

	fprintf (stderr, "Capture device: %s/%s\n", 
		client.getCaptureDevice().id.c_str(),
		client.getCaptureDevice().name.c_str());

	fprintf (stderr, "Playback device: %s/%s\n", 
		client.getPlaybackDevice().id.c_str(),
		client.getPlaybackDevice().name.c_str());



#if USE_VOICE_ACTIVATION
    fprintf (stderr, "Using Voice Activation\n");
#else
	fprintf (stderr, "Press any key to talk\n");
#endif

	fprintf (stderr, ">> Connecting...\n");
	if (!client.connect(channelToken))
	{
		fprintf (stderr, "Connect failed\n");
	}

#if !USE_VOICE_ACTIVATION
	jlib::Thread t = jlib::Thread::createThread(inputThread, NULL);
    client.setSoftPushToTalk(true);
#endif

	for (;;)
	{
		client.update();
		sonar::common::sleepHalfFrame();

#if !USE_VOICE_ACTIVATION
		if (client.getSoftPushToTalk() != g_pttState)
		{
			fprintf(stderr, "%s soft push-to-talk\n", g_pttState ? "Enabling" : "Disabling");
			client.setSoftPushToTalk(g_pttState);
		}
#endif
	}
}