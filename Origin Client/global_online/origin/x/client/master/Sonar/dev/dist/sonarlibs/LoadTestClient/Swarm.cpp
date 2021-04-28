#include "Swarm.h"
#include "SwarmConnection.h"
#include <TestUtils/TokenSigner.h>
#include <jlib/Time.h>
#include <iomanip>
#include <iostream>

Swarm::Swarm (int start, int end, int channelStart, const char *serverAddress, int serverPort, Config &config, sonar::TokenSigner *tokenSigner, long long executionTime) 
	: m_config(config)
    , m_nextFrame(0LL)
	, m_frameCounter(0LL)
	, m_overload(0)
	, m_allConnectedTime(-1)
	, m_executionTime(executionTime)
{
	size_t connections = (size_t) end - start + 1;

	m_conns.resize(connections);
	int id = start;

	double connectInterval = 1000.0 / (double) config.connectsPerSec;
	double connectDelay = 0.0;

	int idChannel = channelStart;
	int channelUsers = config.clientsPerChannel;

#ifndef wgetenv
#define wgetenv _wgetenv
#endif
	
    int firstUserIdInChannel = 0;
    int lastUserIdInChannel = 0;
    int blockedUserCount = config.blockedUserCount;
    if( blockedUserCount > config.clientsPerChannel )
        blockedUserCount = config.clientsPerChannel;

	for (size_t index = 0; index < connections; index ++)
	{
		m_conns[index] = new SwarmConnection(id, idChannel, serverAddress, serverPort, (long long) connectDelay, config, this, tokenSigner, index % 1000);
		connectDelay += connectInterval;		

        if( blockedUserCount > 0 )
        {
            // create blocked list for connection
            lastUserIdInChannel = firstUserIdInChannel + blockedUserCount;
            if( lastUserIdInChannel > connections )
                lastUserIdInChannel = connections;
            for( int i = firstUserIdInChannel; i < lastUserIdInChannel; ++i )
            {
		char userIdString[128];
		sprintf(userIdString, "u%s-%d", config.hostPrefix, i);
                m_conns[index]->addBlockedUser(userIdString);
            }
        }

		id ++;
		channelUsers --;

		if (channelUsers == 0)
		{
			channelUsers = config.clientsPerChannel;
			idChannel ++;
            firstUserIdInChannel = id;
		}
    }
}

Swarm::~Swarm (void)
{
	for (size_t index = 0; index < m_conns.size(); index ++)
	{
		delete m_conns[index];
	}
}

void Swarm::poll()
{
	long long diff = (sonar::common::getTimeAsMSEC() - m_startTime) - m_nextFrame;

	while (diff >= sonar::protocol::FRAME_TIMESLICE_MSEC)
	{
		frame();

		diff -= sonar::protocol::FRAME_TIMESLICE_MSEC;
		m_nextFrame += sonar::protocol::FRAME_TIMESLICE_MSEC;
	}
}

void Swarm::onConnect(SwarmConnection *sc)
{
    if (m_config.showClientConnections) {
        std::cerr 
            << std::setw(8) << sonar::common::getTimeAsMSEC()
            << " Connected cid=" << ((sc) ? sc->getCID() : -1)
            << ", disconnect time=" << ((sc) ? sc->disconnectTime() + m_startTime : -1LL)
            << std::endl;
    }
}

void Swarm::onDisconnect(SwarmConnection *sc, sonar::CString &reasonType, sonar::CString &reasonDesc)
{
    if (m_config.showClientConnections) {
        std::cerr
            << std::setw(8) << sonar::common::getTimeAsMSEC()
            << " Disconnected cid=" << ((sc) ? sc->getCID() : -1)
            << ", rtype=" << reasonType.c_str()
            << ", rdesc=" << reasonDesc.c_str()
            << std::endl;
    }
}

void Swarm::run()
{
	m_startTime = sonar::common::getTimeAsMSEC();
	
	for (;;)
	{
		poll();
		sonar::common::sleepExact(1);

		if (m_time > m_executionTime && m_executionTime != -1)
		{
			fprintf (stderr, "SHUTTING DOWN >>> ");
			printStatus();
			break;
		}
	}
}

void Swarm::frame()
{
	++m_frameCounter;
	m_time = sonar::common::getTimeAsMSEC() - m_startTime;
	
	long long time = m_time;

	int connected = 0;

	for (size_t index = 0; index < m_conns.size(); index ++)
	{
		SwarmConnection *conn = m_conns[index];
  		conn->poll(time, m_frameCounter);

		connected += conn->isConnected() ? 1 : 0;
	}

	if (connected == m_conns.size() && m_allConnectedTime == -1)
	{
		m_allConnectedTime = sonar::common::getTimeAsMSEC();
		fprintf (stderr, "All clients connected in %lld msec\n", m_allConnectedTime - m_startTime);
	}


	if (m_frameCounter % 50 == 0)
	{
		printStatus();
	}
}

long long Swarm::getTime(void)
{
	return m_time;
}

#ifdef _WIN32
static int getpid(void)
{
	return (int) GetCurrentProcessId();
}
#endif

void Swarm::printStatus(void)
{
	int connectCount = 0;
	int connectingCount = 0;
	int disconnectCount = 0;

	for (size_t index = 0; index < m_conns.size(); index ++)
	{
		SwarmConnection *conn = m_conns[index];
		connectCount += conn->isConnected() ? 1 : 0;
		connectingCount += conn->isConnecting() ? 1 : 0;
		disconnectCount += conn->isDisconnected() ? 1 : 0;
	}

    fprintf (stderr, "%05d:%08lld:%08lld> %08d:%08d:%08d\n", getpid(), m_time, m_frameCounter, connectCount, connectingCount, disconnectCount);
	fflush(stderr);
}
