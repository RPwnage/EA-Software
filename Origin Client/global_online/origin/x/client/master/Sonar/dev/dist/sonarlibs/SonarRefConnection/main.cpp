#include <SonarCommon/Common.h>
#include <SonarConnection/Connection.h>
#include <SonarClient/DefaultBuildConfig.h>
#include <stdio.h>
#include <jlib/Thread.h>

namespace sonar
{


class FileEventSink : public IConnectionEvents
{
public:
	FileEventSink(FILE *file) 
	{
		m_file = file;
	}

	//IConnectionEvents
	virtual void onConnect(CString &channelId, CString &channelDesc)
	{
		fprintf (stderr, "%s: %s %s\n", "onConnect", channelId.c_str(), channelDesc.c_str());
	}

	virtual void onDisconnect(CString &reasonType, CString &reasonDesc)
	{
		fprintf (stderr, "%s: %s %s\n", "onDisconnect", reasonType.c_str(), reasonDesc.c_str());
	}

	virtual void onClientJoined(int chid, CString &userId, CString &userDesc)
	{
		fprintf (stderr, "%s: %d %s %s\n", "onClientJoined", chid, userId.c_str(), userDesc.c_str());
	}

	virtual void onClientParted(int chid, CString &userId, CString &userDesc, CString &reasonType, CString &reasonDesc)
	{
		fprintf (stderr, "%s: %d %s %s %s %s\n", "onClientParted", chid, userId.c_str(), userDesc.c_str(), reasonType.c_str(), reasonDesc.c_str());
	}

	virtual void onTakeBegin() 
	{
		fprintf (stderr, "%s:\n", "onTakeBegin");
	}

	virtual void onFrameBegin(long long timestamp)
	{
		fprintf (stderr, "%s:\n", "onFrameBegin");
	}

	virtual void onFrameClientPayload(int chid, const void *payload, size_t cbPayload)
	{
		fprintf (stderr, "%s: %d %d\n", "onFrameClientPayload", chid, (int) cbPayload);
	}

	virtual void onFrameEnd(long long timestamp = 0)
	{
		fprintf (stderr, "%s:\n", "onFrameEnd");
	}

	virtual void onTakeEnd() 
	{
		fprintf (stderr, "%s\n", "onTakeEnd");
	}

	virtual void onEchoMessage(const Message &message)
	{
		fprintf (stderr, "%s: %d\n", "onEchoMessage", (int) message.getBytesLeft());
	}
	
    virtual void onChannelInactivity(unsigned long interval)
    {
        fprintf (stderr, "%s: %lu\n", "onChannelInactivity", interval);
    }

private:
	FILE *m_file;
};

static volatile bool g_isRunning = true;

void *KeyboardProc(void *arg)
{
	getchar();
	g_isRunning = false;
	return NULL;
}

int main(int argc, char **argv)
{
	jlib::Thread thread;

	setvbuf(stderr, NULL, _IONBF, 0);

#ifdef _WIN32
	::CoInitialize(NULL);
	WSADATA wsaData;
	::WSAStartup(MAKEWORD(2,2), &wsaData);
#endif
	argc --;
	argv ++;
	if (argc < 1)
	{
		fprintf (stderr, "Usage:\n<token>\n\n");
		return -1;
	}

	FileEventSink sink(stderr);

	TimeProvider time;
	NetworkProvider network;
	DebugLogger logger;

	sonar::Connection conn(time, network, logger, ConnectionOptions());
	sonar::CString token = argv[0];

	conn.setEventSink(&sink);

	conn.connect(token);

	while (!conn.isConnected() && !conn.isDisconnected())
	{
		conn.poll();
    common::sleepFrame();
	}

	if (conn.isDisconnected())
	{
		return 0;
	}

	thread = jlib::Thread::createThread(KeyboardProc, NULL);

	while (conn.isConnected() && g_isRunning)
	{
		conn.poll();
		common::sleepFrame();
	}

	if (conn.isConnected())
	{
		conn.disconnect("LEAVING", "");
	}
	thread.join();

	return 0;
}

}

int main(int argc, char **argv)
{
	return sonar::main(argc, argv);
}