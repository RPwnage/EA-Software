#pragma once

#include <SonarCommon/Common.h>
#include <SonarConnection/Protocol.h>
#include <jlib/Thread.h>

class SwarmConnection;
namespace sonar
{
	class TokenSigner;
}

class Swarm
{
public:
	class OverloadException
	{
	};

	struct Config
	{
		int connectsPerSec;
		int clientsPerChannel;
		int connectionLengthSec;
		const char *hostPrefix;
		int disconnectBackoff;
		int showDisconnectReason;
		int talkFrames;
        int blockedUserCount;
        int showClientConnections;
	};

	Swarm (int start, int end, int channelStart, const char *serverAddress, int serverPort, Config &config, sonar::TokenSigner *tokenSigner, long long executionTime);
	~Swarm (void);
	void poll();
	void run();
	
	void onConnect(SwarmConnection *sc);
	void onDisconnect(SwarmConnection *sc, sonar::CString &reasonType, sonar::CString &reasonDesc);
	long long getTime(void);
    long long startTime() const { return m_startTime; }

private:

	void frame();
	void printStatus();

    Config& m_config;
	long long m_nextFrame;
	long long m_frameCounter;
	long long m_time;
	long long m_startTime;
	long long m_allConnectedTime;
	long long m_executionTime;
	int m_overload;

	SonarVector<SwarmConnection *> m_conns;
};
