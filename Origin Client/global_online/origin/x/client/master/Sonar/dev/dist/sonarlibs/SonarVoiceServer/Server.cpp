#define NOMINMAX 1

#include "Server.h"
#include <SonarConnection/Message.h>
#include <SonarCommon/Token.h>
#include "Operator.h"

// C headers
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <conio.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/times.h>
#include <limits.h>
#include <netdb.h>
#include <unistd.h>
#endif

// C++ headers
#include <algorithm>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>

#if defined(_WIN32)
#define PRId64 "I64d"
#else
#define __STDC_FORMAT_MACROS 
#include <inttypes.h>
#endif

namespace sonar
{

static int GetPID(void)
{
#ifdef _WIN32
	return (int) GetCurrentProcessId();
#else
	return (int) getpid();
#endif
}

Server *Server::s_instance = NULL;
const char *Server::VERSION_NAME = "XFactor";

struct ServerStats
{
    ServerStats()
    : currentUserCount(0)
    , peakUserCount(0)
    , peakUsersPerChannel(0)
    , currentChannelCount(0)
    , peakChannelCount(0)
    , minChannelDurationInSecs(UINT_MAX)
    , maxChannelDurationInSecs(0)
    , avgChannelDurationInSecs(0)
    , totalChannelDurationInSecs(0)
    , numCompletedChannels(0)
    , avgTalkersPerChannel(0.0f)
    {
    }

    size_t currentUserCount;
    size_t peakUserCount;
    size_t peakUsersPerChannel;
    size_t currentChannelCount;
    size_t peakChannelCount;
    size_t minChannelDurationInSecs;
    size_t maxChannelDurationInSecs;
    size_t avgChannelDurationInSecs;
    size_t totalChannelDurationInSecs;
    size_t numCompletedChannels;
    float  avgTalkersPerChannel;
}
serverStats;

void* Server::AllocMessage::operator new (size_t size)
{
	return Server::getInstance().m_messageAllocator.alloc();
}

void Server::AllocMessage::operator delete (void *p)
{
	Server::getInstance().m_messageAllocator.free( (Server::AllocMessage *) p);
}

Server::Server(CString &_hostname, CString &_voipAddress, CString &_controlAddress, CString &_location, int _maxClients, int _voipPort, const Config &_config)
	: m_chanStatsLog(NULL)
    , m_connectedClientIds()
    , m_clients(sonar::protocol::SERVER_MAX_CLIENTS, NULL)
	, m_spread(0)
	, m_nextBackendFrame(getTime() + Server::BACKEND_FRAME_INTERVAL_MSEC)
	, m_nextBackendEvent(getTime() + _config.backendEventInterval)
	, m_nextMetricsFileWrite(getTime() + _config.metricsFileInterval)
	, m_nextClientStatsFileWrite(getTime() + _config.clientStatsFileInterval)
	, m_nextPrintout(getTime() + _config.printoutInterval)
	, m_updateTime(common::getTimeAsMSEC())
	, m_lastChallenge(-(sonar::protocol::CHALLENGE_REPLY_MIN_INTERVAL * 2))
	, m_shutdownTime(0)
	, m_config(_config)
	, m_clientCount(0)
{
    OPENLOG (NULL, LOG_NDELAY, LOG_LOCAL7);
    SYSLOG (LOG_INFO, "*** Server initiated\n");

	common::setVoipPort(_voipPort);

	String hostname;

	unsigned int seed = (unsigned int) common::getTimeAsMSEC() + (unsigned int) GetPID() + (unsigned int) time(0);

	srand(seed);

	/* Resolve or parse public hostname */

	char strPublicIP[256 + 1];

	if (!_hostname.empty())
	{
		struct addrinfo hai;
		memset (&hai, 0, sizeof (hai));
		hai.ai_family = AF_INET;
		hai.ai_socktype = SOCK_DGRAM;
		hai.ai_protocol = IPPROTO_UDP;

		struct addrinfo *pai;

		if (getaddrinfo (_hostname.c_str(), NULL, &hai, &pai) != 0)
		{
			throw StartupException("Unable to resolve -public argument");
		}

		in_addr inAddr = ((sockaddr_in *) pai->ai_addr)->sin_addr;

		sprintf (strPublicIP, "%s", inet_ntoa (inAddr));
		hostname = strPublicIP;
	} else
	{
		hostname = _hostname;
	}
	
	/* Setup initial UDP socket */
	struct sockaddr_in hostAddr;
	m_sockfd = socket (AF_INET, SOCK_DGRAM, 0);
	int size = 65536;

	while (setsockopt (m_sockfd, SOL_SOCKET, SO_SNDBUF, (char*) &size, sizeof(size)) != -1 && size < 16777216)
	{
		size *= 2;
	}

	size = 65536;

	while (setsockopt (m_sockfd, SOL_SOCKET, SO_RCVBUF, (char*) &size, sizeof(size)) != -1 && size < 16777216)
	{
		size *= 2;
	}

	int flag = 0;
#ifndef _WIN32
	setsockopt (m_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*) &flag, sizeof(flag));
#endif

#ifdef _WIN32
	setsockopt (m_sockfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *) &flag, sizeof (flag));
#endif

#ifdef _WIN32
	unsigned long flags = 1;
	ioctlsocket (m_sockfd, FIONBIO, &flags);
#else
	fcntl (m_sockfd, F_SETFL, O_NONBLOCK);
	fcntl (m_sockfd, F_SETFL, O_NDELAY);
#endif

#ifdef _WIN32
	DWORD dwBytesReturned = 0;
	BOOL bState = FALSE;
	WSAIoctl (m_sockfd, SIO_UDP_CONNRESET, &bState, sizeof(bState), NULL, 0, &dwBytesReturned, NULL, NULL);
#endif

	hostAddr.sin_family = AF_INET;

	if (_voipAddress == "ANY" || _voipAddress == "")
	{
		hostAddr.sin_addr.s_addr = INADDR_ANY;
	}
	else
	{
		hostAddr.sin_addr.s_addr = inet_addr(_voipAddress.c_str());
	}

	hostAddr.sin_port = htons ( (u_short) _voipPort);
	memset (hostAddr.sin_zero, 0, 8);


	if (bind (m_sockfd, (struct sockaddr *) &hostAddr, sizeof (struct sockaddr_in)) == -1)
	{
		StringStream ss;
		ss << "ERROR: Could not bind initial socket on " << inet_ntoa(hostAddr.sin_addr) << ":" << ntohs(hostAddr.sin_port);
		throw StartupException(ss.str().c_str());
	}

	socklen_t len = sizeof (m_voipAddress);
	if (getsockname (m_sockfd, (struct sockaddr *) &m_voipAddress, &len) != -1)
	{
		_voipPort = ntohs(m_voipAddress.sin_port);
		common::setVoipPort(_voipPort);
		common::setVoipAddress(m_voipAddress);
	}
			
	s_instance = this;


	for (int index = 0; index < _maxClients + 1; index ++)
	{
		m_cids.pushEnd(index);
	}

	ClientConnection *client = new ClientConnection(0, "", "", 0, 0, NULL, NULL, NULL, getTime());

	int cid = allocateCid();
	assert (cid == 0);
    // NB: don't record this sentinel client connection in the list of connected client ids
    //m_connectedClientIds.push_back(cid);
	m_clients[cid] = client;
	
    std::ostringstream msg;
    msg
		<< "Starting server in Backend mode:"
		<< " public VoIP address: " << (hostname.empty() ? "0.0.0.0" : hostname) << ':' << _voipPort
		<< ", bound VoIP IP: " << _voipAddress
		<< ", backend address: " << _controlAddress
		<< ", max clients: " << _maxClients
		<< ", shutdown delay: " << _config.shutdownDelay
        << std::endl;
    std::cerr << msg.str();
	std::cerr.flush();
    SYSLOG(LOG_INFO, msg.str().c_str());

	m_backendClient = new BackendClient(hostname, _controlAddress, _location, 10, 10, 30, _maxClients, _voipPort);
	m_backendClient->setState(AbstractOutboundClient::CS_CONNECTION_PENDING);

#if ENABLE_HEALTH_CHECK
	HealthCheck::Config config;
	config.healthcheckinterval = m_config.healthCheckInterval;
	config.backendeventinterval = m_config.backendEventInterval;
	config.backendhealthport = m_config.backendHealthPort;
	config.metricsfileinterval = m_config.metricsFileInterval;
	config.clientstatsfileinterval = m_config.clientStatsFileInterval;
	config.badchannelpercent = m_config.badChannelPercent;
    config.badclientpercent = m_config.badClientPercent;
    config.jitterbuffersize = m_config.jitterBufferSize;
    config.jitterscalefactor = m_config.jitterScaleFactor;
	config.publicip = _hostname.c_str();
	config.voipaddress = _voipAddress.c_str();
	config.voipport = _voipPort;
	config.maxclients = _maxClients;
	m_healthCheck = new HealthCheck(config, m_backendClient);
#endif

	memset(&m_metrics, 0, sizeof(m_metrics));
	memset(&m_lastMetrics, 0, sizeof(m_lastMetrics));
	memset(&m_lastEventMetrics, 0, sizeof(m_lastEventMetrics));
	memset(&m_lastJsonMetrics, 0, sizeof(m_lastJsonMetrics));

	if (m_config.shutdownDelay > 0)
	{
		m_shutdownTime = common::getTimeAsMSEC() + (1000 * m_config.shutdownDelay);
	}

	if(m_config.channelStatsLogging)
	{
		char filename[256];

		SYSLOG (LOG_INFO, "Enabling channel statistics logging\n");

#ifdef _WIN32
		sprintf_s(filename, 256, "channel.%d.log", _voipPort);
#else
		snprintf(filename, 256, "channel.%d.log", _voipPort);
#endif
		m_chanStatsLog = fopen(filename, "a");
	}
}

Server::~Server(void)
{
	cleanup();

	delete m_clients[0];
	freeCid(0);

	delete m_backendClient;
	common::SocketClose(m_sockfd);

#if ENABLE_HEALTH_CHECK
	delete m_healthCheck;
#endif

    SYSLOG(LOG_INFO, "*** Server terminated\n");
}


Server &Server::getInstance(void)
{
	return *s_instance;
}

const sockaddr_in &Server::getVoipAddress() const
{
	return m_voipAddress;
}

void Server::handleChallenge(Message &message)
{
	SYSLOG (LOG_DEBUG, "Server::handleChallenge()\n");
	if (message.getBytesLeft() < 2)
	{
		getMetrics().rejectedChallenges ++;
		return;
	}

	if (getTime() - m_lastChallenge < protocol::CHALLENGE_REPLY_MIN_INTERVAL)
	{
		getMetrics().rejectedChallenges ++;
		return;
	}

	m_lastChallenge = getTime();

    // NB: The Edge server communicates with a Message using a 16-bit sequence instead of the 32-bit sequence
    // used in communication with the client so we need to do some ugly "magic" to make it work.
    unsigned long hi16 = message.getSequence();
	unsigned short lo16 = message.readUnsafeShort();

    SYSLOG(LOG_INFO, "challenge, hi16=0x%08x, lo16=0x%04x", hi16, lo16);

	Message reply(protocol::MSG_CHALLENGE, 0, false);
	reply.updateSequence(hi16);
	reply.writeUnsafeShort(lo16);

	ClientConnection::socketSendMessage(reply, message.getAddress());
}

void Server::processMessage(ClientConnection::Message &message)
{
	getMetrics().readPacket ++;

	if (message.getBytesLeft() < sizeof(sonar::protocol::MessageHeader))
	{
		getMetrics().dropBadHeader ++;
		return;
	}

	int source = (int) message.getHeader().source;
		
	ClientConnection *client = m_clients[source];
	
	if (client == NULL)
	{
		getMetrics().dropBadCid ++;
		return;
	}
	
	if (client->isDetached())
	{
		getMetrics().dropBadState ++;
		return;
	}

	if (client->getCID() != 0)
	{
		const sockaddr_in &exp = client->getRemoteAddress();
		const sockaddr_in &ref = message.getAddress();

        if (exp.sin_addr.s_addr != ref.sin_addr.s_addr ||
			exp.sin_port != ref.sin_port)
		{
			getMetrics().dropBadAddress ++;
			return;
		}
	}

	return client->postMessage(message);
}

long long Server::getTime() const
{
	return m_updateTime;
}

SOCKET Server::getSocket(void)
{
	return m_sockfd;
}

void Server::frame()
{

}

// loadTest
static int updateCount = 0;

void Server::update()
{
	m_updateTime = common::getTimeAsMSEC();

	for (ChannelList::iterator iter = m_channels.begin(); iter != m_channels.end(); iter ++)
	{
		(*iter)->update(m_updateTime);
	}

    for (ConnectedClientIds::iterator i(m_connectedClientIds.begin()), last(m_connectedClientIds.end());
        i != last;
        ++i) {
        ClientConnection* client = m_clients[*i];
        if (!client)
			continue;
		client->update(m_updateTime);
	}

	if (getTime() > m_nextBackendFrame)
	{
		m_backendClient->update();
		m_nextBackendFrame = getTime() + Server::BACKEND_FRAME_INTERVAL_MSEC;
	}

#if ENABLE_HEALTH_CHECK
	//
	// health check
	//
	m_healthCheck->update();
#endif

	//
	// metrics
	//
	long long int timeMs = getTime();
	bool printMetrics = (timeMs > m_nextPrintout);
	bool sendBackendEvents = (timeMs > m_nextBackendEvent);
	bool writeMetricsFile = (timeMs > m_nextMetricsFileWrite);
	bool writeClientStatsFile = (timeMs > m_nextClientStatsFileWrite);

	if( printMetrics || sendBackendEvents || writeMetricsFile || writeClientStatsFile )
	{
		// common
		std::stringstream metrics;
		std::stringstream metrics_system;

#ifdef _WIN32
		char TIME_FORMAT[] = "%Y-%m-%d %H:%M:%S UTC";
#else
		char TIME_FORMAT[] = "%F %T UTC";
#endif
		char timeStr[100];
		time_t rawtime = time(NULL);
		struct tm* gmt = gmtime(&rawtime);
		size_t timeLen = strftime (timeStr, sizeof(timeStr), TIME_FORMAT, gmt);
		if (timeLen == 0)
		{
			timeStr[0] = 0;
		}

		updateServerStats();
		updatePercentageStats(); // avg all clients over all channels
		updateLatencyStats(); // avg all clients over all channels
        updateJitterStats(); // avg all clients over all channels
        updateNetworkStats(); // avg all clients over all channels
        updateChannelRanking();

		// compute system usage
		float memoryUsage = computeMemoryUsage();

#if 1 // send metrics to Backend Server
		if( printMetrics )
		{
			float cpuUsage = computeCPUUsage(m_lastMetrics.cpuTime, m_config.printoutInterval);
			metrics_system.fill('0');
			metrics_system
				<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2)
				<< cpuUsage << ':'
				<< memoryUsage;

			metrics.fill('0');
			metrics
				<< std::setw(2) << m_metrics.currOverload << ':'
				<< std::setw(6) << serverStats.currentUserCount << ':'
				<< std::setw(6) << serverStats.peakUserCount << ':'
				<< std::setw(6) << serverStats.peakUsersPerChannel << ':'
				<< std::setw(5) << serverStats.currentChannelCount << ':'
				<< std::setw(5) << serverStats.peakChannelCount << ':'
				<< std::setw(4) << serverStats.numCompletedChannels << ':'
				<< std::setw(6) << serverStats.minChannelDurationInSecs << ':'
				<< std::setw(6) << serverStats.maxChannelDurationInSecs << ':'
				<< std::setw(6) << serverStats.avgChannelDurationInSecs << ':'
				<< std::setw(9) << serverStats.totalChannelDurationInSecs << ':'
				<< std::setw(6) << m_metrics.readPacket - m_lastMetrics.readPacket << ':'
				<< std::setw(6) << m_metrics.sendPacket - m_lastMetrics.sendPacket << ':'
				<< std::setw(1)
				<< m_metrics.resends - m_lastMetrics.resends << ':'
				<< m_metrics.dropBadCid - m_lastMetrics.dropBadCid << ':'
				<< m_metrics.dropBadHeader - m_lastMetrics.dropBadHeader << ':'
				<< m_metrics.dropBadSeq - m_lastMetrics.dropBadSeq << ':'
				<< m_metrics.dropBadState - m_lastMetrics.dropBadState << ':'
				<< m_metrics.dropUnexpectedMsg - m_lastMetrics.dropUnexpectedMsg << ':'
				<< m_metrics.dropUnexpectedSeq - m_lastMetrics.dropUnexpectedSeq << ':'
				<< m_metrics.dropBadAddress - m_lastMetrics.dropBadAddress << ':'
				<< m_metrics.dropPayloadSize - m_lastMetrics.dropPayloadSize << ':'
				<< m_metrics.dropOverflow - m_lastMetrics.dropOverflow << ':'
                << m_metrics.dropLatePacketNotStarted - m_lastMetrics.dropLatePacketNotStarted << ':'
                << m_metrics.dropLatePacket - m_lastMetrics.dropLatePacket << ':'
				<< m_metrics.dropEmptyPayload - m_lastMetrics.dropEmptyPayload << ':'
				<< m_metrics.dropStopFrame - m_lastMetrics.dropStopFrame << ':'
				<< m_metrics.dropTalkBalance - m_lastMetrics.dropTalkBalance << ':'
				<< m_metrics.dropTruncatedMessages - m_lastMetrics.dropTruncatedMessages << ':'
				<< m_metrics.audioNullFrames - m_lastMetrics.audioNullFrames << ':'
				<< m_metrics.rejectedChallenges - m_lastMetrics.rejectedChallenges << ':'
				<< m_metrics.tooManyTalkers - m_lastMetrics.tooManyTalkers << ':'
				<< m_metrics.tokenInvalid - m_lastMetrics.tokenInvalid << ':'
				<< m_metrics.tokenExpired - m_lastMetrics.tokenExpired << ':'
				<< m_metrics.serverFull - m_lastMetrics.serverFull << ':'
				<< m_metrics.connections - m_lastMetrics.connections << ':'
				<< m_metrics.recvErrors - m_lastMetrics.recvErrors << ':'
				<< m_metrics.sendErrors - m_lastMetrics.sendErrors << ':'
				<< m_metrics.maxAudioLoss - m_lastMetrics.maxAudioLoss << ':'
				<< std::setw(6) << m_metrics.inboundPayloads - m_lastMetrics.inboundPayloads << ':'
				<< std::setw(6) << m_metrics.outboundPayloads - m_lastMetrics.outboundPayloads << ':'
				<< std::setw(2) << m_metrics.sendReliable - m_lastMetrics.sendReliable << ':'
				<< std::setw(6) << m_metrics.audioIncomingMessagesLost - m_lastMetrics.audioIncomingMessagesLost << ':'
				<< std::setw(6) << m_metrics.audioIncomingMessagesBadSequence - m_lastMetrics.audioIncomingMessagesBadSequence << ':'
                << std::setw(3) << m_jitterStats.jitterMean << ':'
                << std::setw(3) << m_jitterStats.jitterMax << ':'
                << std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_networkStats.loss << ':'
                << std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_networkStats.jitter << ':'
                << std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_networkStats.ping << ':'
                << std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_networkStats.received << ':'
                << std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(4) << m_networkStats.quality;
				
			if(m_config.loadTest)
			{
				updateCount++;
				printMetricsForLoadTest(timeStr, metrics_system);
			}
			else
			{
#ifdef _WIN32
				std::cerr << timeStr << ' ' << metrics.str() << ' ' << metrics_system.str() << std::endl;
#else
				SYSLOG(LOG_INFO, "%s %s\n", metrics.str().c_str(), metrics_system.str().c_str());
#endif
			}

			std::cerr.flush();

			memcpy(&m_lastMetrics, &m_metrics, sizeof(Metrics));
			m_nextPrintout = getTime() + m_config.printoutInterval;
			
			if (m_config.gnuPlot)
			{
				std::cout
					<< "0:" << std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << cpuUsage << std::endl
					<< "1:" << std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << memoryUsage << std::endl
					<< "2:" << getClientCount() << std::endl
					<< "3:" << m_channels.size() << std::endl;

				std::cout.flush();
			}

            resetJitterStats();
		} // if( printMetrics )

		if( sendBackendEvents )
		{
			float cpuUsage = computeCPUUsage(m_lastEventMetrics.cpuTime, m_config.backendEventInterval);
			metrics_system.str("");
			metrics_system.clear();
			metrics_system.fill('0');
			metrics_system
				<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2)
				<< cpuUsage << ':'
				<< memoryUsage;

			std::stringstream metrics_state; // sent to backend server
			metrics_state.fill('0');
			metrics_state
				// << m_metrics.currOverload,
				<< std::setw(4) << getClientCount() << ':'
				<< std::setw(4) << m_channels.size();

			// health stats
			computeHealthStats(m_healthStats);
			std::stringstream metrics_health; // sent to backend server
			metrics_health.fill('0');
			metrics_health
				<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_healthStats.dropConnectionPercentage << ':'
				<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_healthStats.dropPacketsPercentage << ':'
				<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_healthStats.resendPercentage << ':'
				<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_healthStats.nullFramePercentage << ':'
				<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_healthStats.lostMessagePercentage << ':'
				<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_healthStats.outOfSequenceMessagePercentage;
			
			std::stringstream metrics_counter; // sent to backend server
			metrics_counter.fill('0');
			metrics_counter
				<< std::setw(6) << m_metrics.readPacket - m_lastEventMetrics.readPacket << ':'
				<< std::setw(6) << m_metrics.sendPacket - m_lastEventMetrics.sendPacket << ':'
				<< std::setw(1)
				<< m_metrics.resends - m_lastEventMetrics.resends << ':'
				<< m_metrics.dropBadCid - m_lastEventMetrics.dropBadCid << ':'
				<< m_metrics.dropBadHeader - m_lastEventMetrics.dropBadHeader << ':'
				//<< m_metrics.dropBadSeq - m_lastEventMetrics.dropBadSeq << ':'
				<< m_metrics.dropBadState - m_lastEventMetrics.dropBadState << ':'
				<< m_metrics.dropUnexpectedMsg - m_lastEventMetrics.dropUnexpectedMsg << ':'
				<< m_metrics.dropUnexpectedSeq - m_lastEventMetrics.dropUnexpectedSeq << ':'
				<< m_metrics.dropBadAddress - m_lastEventMetrics.dropBadAddress << ':'
				<< m_metrics.dropPayloadSize - m_lastEventMetrics.dropPayloadSize << ':'
				<< m_metrics.dropOverflow - m_lastEventMetrics.dropOverflow << ':'
                << m_metrics.dropLatePacketNotStarted - m_lastEventMetrics.dropLatePacketNotStarted << ':'
                << m_metrics.dropLatePacket - m_lastEventMetrics.dropLatePacket << ':'
				<< m_metrics.dropEmptyPayload - m_lastEventMetrics.dropEmptyPayload << ':'
				<< m_metrics.dropStopFrame - m_lastEventMetrics.dropStopFrame << ':'
				<< m_metrics.dropTalkBalance - m_lastEventMetrics.dropTalkBalance << ':'
				<< m_metrics.audioNullFrames - m_lastEventMetrics.audioNullFrames << ':'
				<< m_metrics.rejectedChallenges - m_lastEventMetrics.rejectedChallenges << ':'
				<< m_metrics.tooManyTalkers - m_lastEventMetrics.tooManyTalkers << ':'
				//<< m_metrics.tokenInvalid - m_lastEventMetrics.tokenInvalid << ':'
				//<< m_metrics.tokenExpired - m_lastEventMetrics.tokenExpired << ':'
				//<< m_metrics.serverFull - m_lastEventMetrics.serverFull << ':'
				<< std::setw(6) << m_metrics.inboundPayloads - m_lastEventMetrics.inboundPayloads << ':'
				<< std::setw(6) << m_metrics.outboundPayloads - m_lastEventMetrics.outboundPayloads << ':'
				<< std::setw(2) << m_metrics.sendReliable - m_lastEventMetrics.sendReliable;
				//<< std::setw(1) << m_metrics.dropConnection - m_lastEventMetrics.dropConnection;
				//<< std::setw(6) << m_metrics.audioIncomingMessagesLost - m_lastEventMetrics.audioIncomingMessagesLost << ':'
				//<< std::setw(6) << m_metrics.audioIncomingMessagesBadSequence - m_lastEventMetrics.audioIncomingMessagesBadSequence << ':';

            std::stringstream metrics_network; // sent to backend server
            metrics_network.fill('0');
            metrics_network
                << std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_networkStats.loss << ':'
                << std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_networkStats.jitter << ':'
                << std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_networkStats.ping << ':'
                << std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << m_networkStats.received << ':'
                << std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(4) << m_networkStats.quality;

		#if 1 // Send metrics events to the Voice Edge
			m_backendClient->eventVoiceServerMetrics_State (metrics_state.str().c_str());
			//m_backendClient->eventVoiceServerMetrics_Counter (metrics_counter.str().c_str());
			m_backendClient->eventVoiceServerMetrics_Health (metrics_health.str().c_str());
			m_backendClient->eventVoiceServerMetrics_System (metrics_system.str().c_str());
			m_backendClient->eventVoiceServerMetrics_Network (metrics_network.str().c_str());
			//std::cerr << "send backend event: " << metrics_state.str() << std::endl;
			//std::cerr << "send backend event: " << metrics_counter.str() << std::endl;
			//std::cerr << "send backend event: " << metrics_system.str() << std::endl;
			//std::cerr.flush();
		#endif
			memcpy(&m_lastEventMetrics, &m_metrics, sizeof(Metrics));
			m_nextBackendEvent = getTime() + m_config.backendEventInterval;
		} // if( sendBackendEvents )
		
		if( writeMetricsFile )
		{
			writeMetricsJson();
			memcpy(&m_lastJsonMetrics, &m_metrics, sizeof(Metrics));
			m_nextMetricsFileWrite = getTime() + m_config.metricsFileInterval;			
		} // if( writeMetricsFile)
		
		if( writeClientStatsFile )
		{
			// worst_channel_n.json
			updateChannelRanking();
			for( int i = 0; i < 10; ++i )
			{
				writeChannelJson(i);
			}

			// worst_client_n.json
			writeClientJson();

			m_nextClientStatsFileWrite = getTime() + m_config.clientStatsFileInterval;
		}
#else
	std::stringstream metrics;
	metrics.fill('0');
	metrics
		<< std::setw(2) << m_metrics.currOverload << ':'
		<< std::setw(4) << getClientCount() << ':'
		<< std::setw(4) << m_channels.size() << ':'
		<< std::setw(6) << m_metrics.readPacket - m_lastMetrics.readPacket << ':'
		<< std::setw(6) << m_metrics.sendPacket - m_lastMetrics.sendPacket << ':'
		<< std::setw(1)
		<< m_metrics.resends - m_lastMetrics.resends << ':'
		<< m_metrics.dropBadCid - m_lastMetrics.dropBadCid << ':'
		<< m_metrics.dropBadHeader - m_lastMetrics.dropBadHeader << ':'
		<< m_metrics.dropBadSeq - m_lastMetrics.dropBadSeq << ':'
		<< m_metrics.dropBadState - m_lastMetrics.dropBadState << ':'
		<< m_metrics.dropUnexpectedMsg - m_lastMetrics.dropUnexpectedMsg << ':'
		<< m_metrics.dropUnexpectedSeq - m_lastMetrics.dropUnexpectedSeq << ':'
		<< m_metrics.dropBadAddress - m_lastMetrics.dropBadAddress << ':'
		<< m_metrics.dropPayloadSize - m_lastMetrics.dropPayloadSize << ':'
		<< m_metrics.dropOverflow - m_lastMetrics.dropOverflow << ':'
        << m_metrics.dropLatePacketNotStarted - m_lastMetrics.dropLatePacketNotStarted << ':'
        << m_metrics.dropLatePacket - m_lastMetrics.dropLatePacket << ':'
		<< m_metrics.dropEmptyPayload - m_lastMetrics.dropEmptyPayload << ':'
		<< m_metrics.dropStopFrame - m_lastMetrics.dropStopFrame << ':'
		<< m_metrics.dropTalkBalance - m_lastMetrics.dropTalkBalance << ':'
		<< m_metrics.audioNullFrames - m_lastMetrics.audioNullFrames << ':'
		<< m_metrics.rejectedChallenges - m_lastMetrics.rejectedChallenges << ':'
		<< m_metrics.tooManyTalkers - m_lastMetrics.tooManyTalkers << ':'
		<< m_metrics.tokenInvalid - m_lastMetrics.tokenInvalid << ':'
		<< m_metrics.tokenExpired - m_lastMetrics.tokenExpired << ':'
		<< m_metrics.serverFull - m_lastMetrics.serverFull << ':'
        << m_metrics.maxAudioLoss - m_lastMetrics.maxAudioLoss << ':'
		<< std::setw(6) << m_metrics.inboundPayloads - m_lastMetrics.inboundPayloads << ':'
		<< std::setw(6) << m_metrics.outboundPayloads - m_lastMetrics.outboundPayloads << ':'
		<< std::setw(2) << m_metrics.sendReliable - m_lastMetrics.sendReliable << ':'
		<< std::setw(6) << m_metrics.audioIncomingMessagesLost - m_lastMetrics.audioIncomingMessagesLost << ':'
		<< std::setw(6) << m_metrics.audioIncomingMessagesBadSequence - m_lastMetrics.audioIncomingMessagesBadSequence;
        std::cerr << timeStr << ' ' << metrics.str() << std:endl;
	std::cerr.flush();
#endif
	}
}

float Server::computeCPUUsage(long long int lastCpuTime, int intervalMsec)
{
    float percent = 0.f;

#ifdef _WIN32
#pragma message("computeCPUUsage() not defined for WIN32")
#else
    // NOTE: sysconf (_SC_CLK_TICK) returns ticks/second
    //       So, divide by 1000 to get ticks/millisecond
    //       Then, multiply by interval (milliseconds) to get number of ticks for the interval
    long long cpuIntervalTicks = sysconf (_SC_CLK_TCK) * intervalMsec / 1000.f;
    struct tms tms0;

    if (times (&tms0) != -1)
    {
        // CPU time in ticks
        m_metrics.cpuTime = tms0.tms_utime + tms0.tms_stime;
        long long diffTicks = m_metrics.cpuTime - lastCpuTime;
        percent = 100.f * diffTicks / cpuIntervalTicks;
    }
#endif

    return percent;
}

typedef struct {
    unsigned long size, resident, share, text, lib, data, dt;
} statm_t;

int read_memory_status( statm_t *result)
{
    const char* statm_path = "/proc/self/statm";

    // open the file for reading memory status
    FILE *f = fopen(statm_path, "r");
    if (!f) {
        return -1;
    }

    // read the file
    int count = fscanf(f, "%ld %ld %ld %ld %ld %ld %ld", &result->size, &result->resident, &result->share, &result->text, &result->lib, &result->data, &result->dt);
    fclose(f);

    if (7 != count) {
        return -1;
    }

    return 0; // all good
}

size_t get_available_memory_bytes()
{
#if defined(_SC_PHYS_PAGES) && defined(_SC_PAGE_SIZE)
    return (size_t)sysconf( _SC_PHYS_PAGES ) * (size_t)sysconf( _SC_PAGE_SIZE );
#else
    return 0L;
#endif
}

size_t get_available_memory_pages()
{
#if defined(_SC_PHYS_PAGES)
    return (size_t)sysconf( _SC_PHYS_PAGES );
#else
    return 0L;
#endif
}

float Server::computeMemoryUsage()
{
    float percent = 0.f;

#ifdef _WIN32
#pragma message("computeMemoryUsage() not defined for WIN32")
#else
    static long available_memory_pages = get_available_memory_pages();
    statm_t statm;
    if (read_memory_status(&statm) == 0) {
        if (available_memory_pages > 0) {
            long total_used_pages = statm.resident + statm.text + statm.data;
            //fprintf(stderr, "size(%ld) resident(%ld) share(%ld) text(%ld) lib(%ld) data(%ld) dt(%ld)\n", statm.size, statm.resident, statm.share, statm.text, statm.lib, statm.data, statm.dt);
            //fprintf(stderr, "total = %ld, available = %ld\n", total_used_pages, available_memory_pages);
            percent = float(total_used_pages) / float(available_memory_pages);
            percent *= 100.f;
        }
    }
#endif

    return percent;
}

void Server::writeMetricsJson()
{
#ifndef _WIN32
	const char *metricsJsonLocation = "Node/sonar-voiceserver/";
	char filename[256];
	snprintf(filename, 256, "%smetrics.json", metricsJsonLocation);

	FILE *f = fopen(filename, "w");
	if( f != NULL )
	{
		CString worstChannelList = getWorstChannelList();
		CString worstClientList = (m_rankedChannels.size() > 0) ? m_rankedChannels[0]->getWorstClientList() : "no clients";

		fprintf(f,
			"{\"metrics\": {\n"
			"  \"state\": {\n"
			"    \"clients\": %lu,\n"
			"    \"channels\": %lu,\n"
			"    \"avgTalkersPerChannel\": %f\n"
			"  },\n"
			"  \"counter\": {\n"
			"    \"connections\": %d,\n"
			"    \"inboundPackets\": %d,\n"
			"    \"outboundPackets\": %d,\n"
			"    \"reliablePacketResends\": %d,\n"
			"    \"dropBadCid\": %d,\n"
			"    \"dropBadHeader\": %d,\n"
			//"    \"dropBadSequence\": %d,\n"
			"    \"dropBadState\": %d,\n"
			"    \"dropUnexpectedMsg\": %d,\n"
			"    \"dropUnexpectedSeq\": %d,\n"
			"    \"dropBadAddress\": %d,\n"
			"    \"dropPayloadSize\": %d,\n"
			"    \"dropOverflow\": %d,\n"
            "    \"dropLatePacketNotStarted\": %d,\n"
            "    \"dropLatePacket\": %d,\n"
			"    \"dropEmptyPayload\": %d,\n"
			"    \"dropStopFrame\": %d,\n"
			"    \"dropTalkBalance\": %d,\n"
			"    \"dropConnection\": %d,\n"
			"    \"dropTruncatedMessages\": %d,\n"
			"    \"audioNullFrames\": %d,\n"
			"    \"rejectedChallenges\": %d,\n"
			"    \"tooManyTalkers\": %d,\n"
			"    \"tokenInvalid\": %d,\n"
			"    \"tokenExpired\": %d,\n"
			"    \"serverFull\": %d,\n"
			"    \"inboundPayloads\": %d,\n"
			"    \"outboundPayloads\": %d,\n"
			"    \"sendReliable\": %d,\n"
			//"    \"audioIncomingMessagesLost\": %u,\n"
			"    \"audioIncomingMessagesBadSequence\": %u,\n"
            "    \"maxAudioLoss\": %d,\n"
			"    \"badChannels\": %d\n"
			"  },\n"
			"  \"percentage\": {\n"
			"    \"clientLostMessages\": %0.2f,\n"
			"    \"clientOutOfSequenceMessages\": %0.2f\n"
			"  },\n"
			"  \"latency\": {\n"
			"    \"deltaPlaybackMean\": %u,\n"
			"    \"deltaPlaybackMax\": %u,\n"
			"    \"receiveToPlaybackMean\": %u,\n"
			"    \"receiveToPlaybackMax\": %u\n"
			"  },\n"
            "  \"list\": {\n"
            "    \"worst_channels\": \"%s\",\n"
            "    \"worst_clients\": \"%s\"\n"
            "  },\n"
            "  \"jitter\": {\n"
            "    \"jitterMean\": %.2f,\n"
            "    \"jitterMax\": %d\n"
            "  },\n"
            "  \"network\": {\n"
            "    \"networkLoss\": %0.2f,\n"
            "    \"networkJitter\": %0.2f,\n"
            "    \"networkPing\": %0.2f,\n"
            "    \"networkReceived\": %0.2f,\n"
            "    \"networkQuality\": %0.4f\n"
            "  }\n"
			"}}",
			getClientCount(),
			m_channels.size(),
			serverStats.avgTalkersPerChannel,
			m_metrics.connections - m_lastJsonMetrics.connections,
			m_metrics.readPacket - m_lastJsonMetrics.readPacket,
			m_metrics.sendPacket - m_lastJsonMetrics.sendPacket,
			m_metrics.resends - m_lastJsonMetrics.resends,
			m_metrics.dropBadCid - m_lastJsonMetrics.dropBadCid,
			m_metrics.dropBadHeader - m_lastJsonMetrics.dropBadHeader,
			//m_metrics.dropBadSeq - m_lastJsonMetrics.dropBadSeq,
			m_metrics.dropBadState - m_lastJsonMetrics.dropBadState,
			m_metrics.dropUnexpectedMsg - m_lastJsonMetrics.dropUnexpectedMsg,
			m_metrics.dropUnexpectedSeq - m_lastJsonMetrics.dropUnexpectedSeq,
			m_metrics.dropBadAddress - m_lastJsonMetrics.dropBadAddress,
			m_metrics.dropPayloadSize - m_lastJsonMetrics.dropPayloadSize,
			m_metrics.dropOverflow - m_lastJsonMetrics.dropOverflow,
            m_metrics.dropLatePacketNotStarted - m_lastJsonMetrics.dropLatePacketNotStarted,
            m_metrics.dropLatePacket - m_lastJsonMetrics.dropLatePacket,
			m_metrics.dropEmptyPayload - m_lastJsonMetrics.dropEmptyPayload,
			m_metrics.dropStopFrame - m_lastJsonMetrics.dropStopFrame,
			m_metrics.dropTalkBalance - m_lastJsonMetrics.dropTalkBalance,
			m_metrics.dropConnection - m_lastJsonMetrics.dropConnection,
			m_metrics.dropTruncatedMessages - m_lastJsonMetrics.dropTruncatedMessages,
			m_metrics.audioNullFrames - m_lastJsonMetrics.audioNullFrames,
			m_metrics.rejectedChallenges - m_lastJsonMetrics.rejectedChallenges,
			m_metrics.tooManyTalkers - m_lastJsonMetrics.tooManyTalkers,
			m_metrics.tokenInvalid - m_lastJsonMetrics.tokenInvalid,
			m_metrics.tokenExpired - m_lastJsonMetrics.tokenExpired,
			m_metrics.serverFull - m_lastJsonMetrics.serverFull,
			m_metrics.inboundPayloads - m_lastJsonMetrics.inboundPayloads,
			m_metrics.outboundPayloads - m_lastJsonMetrics.outboundPayloads,
			m_metrics.sendReliable - m_lastJsonMetrics.sendReliable,
			//m_metrics.audioIncomingMessagesLost - m_lastJsonMetrics.audioIncomingMessagesLost,
			m_metrics.audioIncomingMessagesBadSequence - m_lastJsonMetrics.audioIncomingMessagesBadSequence,
            m_metrics.maxAudioLoss - m_lastJsonMetrics.maxAudioLoss,
			badChannelCount(),
			m_healthStats.lostMessagePercentage,
			m_healthStats.outOfSequenceMessagePercentage,
			m_latencyStats.deltaPlaybackMean,
			m_latencyStats.deltaPlaybackMax,
			m_latencyStats.receiveToPlaybackMean,
			m_latencyStats.receiveToPlaybackMax,
			worstChannelList.c_str(),
			worstClientList.c_str(),
            m_jitterStats.jitterMean,
            m_jitterStats.jitterMax,
            m_networkStats.loss,
            m_networkStats.jitter,
            m_networkStats.ping,
            m_networkStats.received,
            m_networkStats.quality
		);

		fclose(f);
	}
#endif
}

void Server::computeHealthStats(HealthStats& healthStats)
{
	//
	// null frame percentage
	//
	int audioNullFrames = m_metrics.audioNullFrames - m_lastMetrics.audioNullFrames;
	int inboundPayloads = m_metrics.inboundPayloads - m_lastMetrics.inboundPayloads;
	int totalAudioPayloads = audioNullFrames + inboundPayloads;
	float nullframe_percentage = (totalAudioPayloads > 0) ? float(audioNullFrames) / float(totalAudioPayloads) : 0.f;

	//
	// track all dropped packets
	//
	int dropBadCid = m_metrics.dropBadCid - m_lastMetrics.dropBadCid;
	int dropBadHeader = m_metrics.dropBadHeader - m_lastMetrics.dropBadHeader;
	//m_metrics.dropBadSeq - m_lastMetrics.dropBadSeq,
	int dropBadState = m_metrics.dropBadState - m_lastMetrics.dropBadState;
	int dropUnexpectedMsg = m_metrics.dropUnexpectedMsg - m_lastMetrics.dropUnexpectedMsg;
	int dropUnexpectedSeq = m_metrics.dropUnexpectedSeq - m_lastMetrics.dropUnexpectedSeq;
	int dropBadAddress = m_metrics.dropBadAddress - m_lastMetrics.dropBadAddress;
	int dropPayloadSize = m_metrics.dropPayloadSize - m_lastMetrics.dropPayloadSize;
	int dropOverflow = m_metrics.dropOverflow - m_lastMetrics.dropOverflow;
    int dropLatePacketNotStarted = m_metrics.dropLatePacketNotStarted - m_lastMetrics.dropLatePacketNotStarted;
    int dropLatePacket = m_metrics.dropLatePacket - m_lastMetrics.dropLatePacket;
	int dropEmptyPayload = m_metrics.dropEmptyPayload - m_lastMetrics.dropEmptyPayload;
	int dropStopFrame = m_metrics.dropStopFrame - m_lastMetrics.dropStopFrame;
	int dropTalkBalance = m_metrics.dropTalkBalance - m_lastMetrics.dropTalkBalance;
	int dropTruncatedMessages = m_metrics.dropTruncatedMessages - m_lastMetrics.dropTruncatedMessages;
	int tooManyTalkers = m_metrics.tooManyTalkers - m_lastMetrics.tooManyTalkers;
	int dropForced = m_metrics.dropForced - m_lastMetrics.dropForced;

	int dropTotal = dropForced + dropBadCid + dropBadHeader + dropBadState +
			dropUnexpectedMsg + dropUnexpectedSeq + dropBadAddress +
			dropPayloadSize + dropOverflow + dropLatePacketNotStarted + dropLatePacket +
			dropEmptyPayload + dropStopFrame + dropTalkBalance + dropTruncatedMessages + tooManyTalkers;

	//
	// dropped packet percentage
	//
	int receivedPackets = m_metrics.readPacket - m_lastMetrics.readPacket;
	float drop_percentage = (receivedPackets > 0) ? float(dropTotal) / float(receivedPackets) : 0.f;

	//
	// resend percentage
	//
	int sendReliable = m_metrics.sendReliable - m_lastMetrics.sendReliable;
	int resends = m_metrics.resends - m_lastMetrics.resends;
	static float ONE_OVER_RESEND_COUNT = 1.0f / float(protocol::RESEND_COUNT);
	float resend_percentage = (sendReliable > 0) ? (ONE_OVER_RESEND_COUNT * float(resends) / float(sendReliable)) : 0.f;

	//
	// dropped connection percentage
	//
	int connections = getClientCount();
	float drop_connection_percentage = 0.0f;
	if( connections > 0 )
	{
		int dropConnection = m_metrics.dropConnection - m_lastMetrics.dropConnection;
		drop_connection_percentage = (float)dropConnection / ((float)dropConnection + (float)connections);
	}

	healthStats.nullFramePercentage = nullframe_percentage * 100.f;
	healthStats.resendPercentage = resend_percentage * 100.f;
	healthStats.dropPacketsPercentage = drop_percentage * 100.f;
	healthStats.dropConnectionPercentage = drop_connection_percentage * 100.f;
	healthStats.lostMessagePercentage = m_percentageStats.loss * 100.f;
	healthStats.outOfSequenceMessagePercentage = m_percentageStats.oos * 100.f;
}

void Server::updateChannelRanking()
{
	// rank channels by voice quality performance based on client stats
	m_rankedChannels.clear();
	for (ChannelList::iterator iter = m_channels.begin(); iter != m_channels.end(); iter ++)
	{
		Channel *channel = *iter;
		channel->updateRankScore();
		m_rankedChannels.push_back(channel);
	}

	// sort 'm_rankedChannels'
	std::sort(m_rankedChannels.begin(), m_rankedChannels.end());
}

void Server::updatePercentageStats()
{
	m_percentageStats.loss = 0.0f;
    m_percentageStats.oos = 0.0f;
    m_percentageStats.jitter = 0.0f;

	// average percentage stats from all channels
	for (ChannelList::iterator iter = m_channels.begin(); iter != m_channels.end(); iter ++)
	{
		Channel *channel = *iter;
		m_percentageStats.loss += channel->getPercentageStats().loss;
        m_percentageStats.oos += channel->getPercentageStats().oos;
        m_percentageStats.jitter += (channel->getJitterStats().arrivalJitterMean / 2000.0);
	}

	if( m_channels.size() > 0 )
	{
		m_percentageStats.loss /= (float)m_channels.size();
        m_percentageStats.oos /= (float)m_channels.size();
        m_percentageStats.jitter /= (float)m_channels.size();
	}

    SYSLOG(LOG_DEBUG, "server stats: %.2f lost + %.2f oos + %.2f jitter\n"
        , m_percentageStats.loss
        , m_percentageStats.oos
        , m_percentageStats.jitter
    );
}

void Server::updateLatencyStats()
{
	m_latencyStats.deltaPlaybackMean = 0;
	m_latencyStats.deltaPlaybackMax = 0;
	m_latencyStats.receiveToPlaybackMean = 0;
	m_latencyStats.receiveToPlaybackMax = 0;
	
	// average latency stats from all channels
	for (ChannelList::iterator iter = m_channels.begin(); iter != m_channels.end(); iter ++)
	{
		Channel *channel = *iter;
		channel->updateLatencyStats();

		const Channel::LatencyStats &latencyStats = channel->getLatencyStats();

		m_latencyStats.deltaPlaybackMean += latencyStats.deltaPlaybackMean;
		m_latencyStats.deltaPlaybackMax += latencyStats.deltaPlaybackMax;
		m_latencyStats.receiveToPlaybackMean += latencyStats.receiveToPlaybackMean;
		m_latencyStats.receiveToPlaybackMax += latencyStats.receiveToPlaybackMax;
	}

	if( m_channels.size() > 0 )
	{
		m_latencyStats.deltaPlaybackMean = (unsigned int)((float)m_latencyStats.deltaPlaybackMean / (float)m_channels.size());
		m_latencyStats.deltaPlaybackMax = (unsigned int)((float)m_latencyStats.deltaPlaybackMax / (float)m_channels.size());
		m_latencyStats.receiveToPlaybackMean = (unsigned int)((float)m_latencyStats.receiveToPlaybackMean / (float)m_channels.size());
		m_latencyStats.receiveToPlaybackMax = (unsigned int)((float)m_latencyStats.receiveToPlaybackMax / (float)m_channels.size());
	}
}

void Server::updateJitterStats()
{
    m_jitterStats.jitterMean = 0;
    m_jitterStats.jitterMax = 0;

    // average jitter stats from all channels
    for (ChannelList::iterator iter = m_channels.begin(); iter != m_channels.end(); iter ++)
    {
        Channel *channel = *iter;
        if (!channel)
            continue;

        const Channel::JitterStats &jitterStats = channel->updateJitterStats();

        m_jitterStats.jitterMean += jitterStats.playbackJitterMean;
        m_jitterStats.jitterMax = std::max(jitterStats.playbackJitterMax, m_jitterStats.jitterMax);
    }

    if( m_channels.size() > 0 )
    {
        m_jitterStats.jitterMean = (m_jitterStats.jitterMean / m_channels.size());
        m_jitterStats.jitterMax = m_jitterStats.jitterMax;
    }
}

void Server::updateNetworkStats()
{
    m_networkStats.reset();

    int channelsWithNetworkStats = 0;

    // average network stats from all channels
    for (ChannelList::iterator iter = m_channels.begin(); iter != m_channels.end(); iter ++)
    {
        Channel *channel = *iter;
        if (!channel)
            continue;

        const Channel::NetworkStats &networkStats = channel->updateNetworkStats();
        if (networkStats.valid)
        {
            channelsWithNetworkStats++;

            m_networkStats.loss += networkStats.loss;
            m_networkStats.jitter += networkStats.jitter;
            m_networkStats.ping += networkStats.ping;
            m_networkStats.received += networkStats.received;
            m_networkStats.quality += networkStats.quality;
        }
    }

    if (channelsWithNetworkStats > 0)
    {
        m_networkStats.loss = m_networkStats.loss / (float)channelsWithNetworkStats;
        m_networkStats.jitter = m_networkStats.jitter / (float)channelsWithNetworkStats;
        m_networkStats.ping = m_networkStats.ping / (float)channelsWithNetworkStats;
        m_networkStats.received = m_networkStats.received / (float)channelsWithNetworkStats;
        m_networkStats.quality = m_networkStats.quality / (float)channelsWithNetworkStats;
    }
}

CString Server::getWorstChannelList()
{
	String channelList;

	if( m_rankedChannels.empty() )
	{
		channelList = "no channels";
	}
	else
	{
		RankedChannelList::iterator iter = m_rankedChannels.begin();
		for( int i = 0; i < 10 && iter != m_rankedChannels.end(); ++i, ++iter )
		{
			Channel* channel = *iter;
			if (channel == NULL)
				continue;

			if( i > 0 )
				channelList += ", ";

			char number[5];
			sprintf(number, "%d", i);

			channelList += number;
			channelList += ") ";
			channelList += channel->getName();
		}
	}

	return channelList;
}

int Server::badChannelCount()
{
	int count = 0;

	// count number of bad channels
	for (ChannelList::iterator iter = m_channels.begin(); iter != m_channels.end(); iter ++)
	{
		Channel *channel = *iter;
		if (channel == NULL)
			continue;

		if( channel->isBad(m_config.badChannelPercent, m_config.badClientPercent) )
			count++;
	}

	return count;
}

void Server::writeChannelJson(unsigned int index)
{
#ifndef _WIN32
	Channel* channel = NULL;
	if( index < m_rankedChannels.size() )
	{
		channel = m_rankedChannels[index];
	}

   	const char *channelJsonLocation = "Node/sonar-voiceserver/";
	char filename[256];
	snprintf(filename, 256, "%sworst_channel_%d.json", channelJsonLocation, index);

    if( channel == NULL )
    {
        Channel::writeMetricsJsonAllZero(filename);
    }
    else
    {
	    channel->updateStats();
	    channel->writeMetricsJson(filename);
	    channel->resetTalkers();
    }
#endif
}

void Server::writeClientJson()
{
  	if( m_rankedChannels.size() > 0 )
	{
	  	// update client ranking of worst performing channel
		m_rankedChannels[0]->updateClientRanking();
		// write client json of worst performing channel
		m_rankedChannels[0]->writeClientJson();
	}
}

void Server::resetClientStats()
{
	// reset all client stats
	for (ChannelList::iterator iter = m_channels.begin(); iter != m_channels.end(); iter ++)
	{
		Channel *channel = *iter;
		if (channel == NULL)
			continue;

		channel->resetClientStats();
	}
}

void Server::resetJitterStats()
{
    // reset all client jitter stats
    for (ChannelList::iterator iter = m_channels.begin(); iter != m_channels.end(); iter ++)
    {
        Channel *channel = *iter;
        if (channel == NULL)
            continue;

        channel->resetJitterStats();
    }
}

// loadtest
void Server::printMetricsForLoadTest(const char * timeStr, const std::stringstream & metrics_system)
{
	std::stringstream metrics;
	metrics.fill('0');
	metrics
		<< std::setw(2) << m_metrics.currOverload << ':'
		<< std::setw(4) << serverStats.currentUserCount << ':'
		<< std::setw(4) << serverStats.currentChannelCount << ':'
		<< std::setw(6) << m_metrics.readPacket - m_lastMetrics.readPacket << ':'
		<< std::setw(6) << m_metrics.sendPacket - m_lastMetrics.sendPacket << ':'
		<< std::setw(1)
		<< m_metrics.resends - m_lastMetrics.resends << ':'
		<< m_metrics.dropBadCid - m_lastMetrics.dropBadCid << ':'
		<< m_metrics.dropBadHeader - m_lastMetrics.dropBadHeader << ':'
		//<< m_metrics.dropBadSeq - m_lastMetrics.dropBadSeq << ':'
		<< m_metrics.dropBadState - m_lastMetrics.dropBadState << ':'
		<< m_metrics.dropUnexpectedMsg - m_lastMetrics.dropUnexpectedMsg << ':'
		<< m_metrics.dropUnexpectedSeq - m_lastMetrics.dropUnexpectedSeq << ':'
		<< m_metrics.dropBadAddress - m_lastMetrics.dropBadAddress << ':'
		<< m_metrics.dropPayloadSize - m_lastMetrics.dropPayloadSize << ':'
        << m_metrics.dropLatePacketNotStarted - m_lastMetrics.dropLatePacketNotStarted << ':'
        << m_metrics.dropLatePacket - m_lastMetrics.dropLatePacket << ':'
		<< m_metrics.dropEmptyPayload - m_lastMetrics.dropEmptyPayload << ':'
		<< m_metrics.dropTalkBalance - m_lastMetrics.dropTalkBalance << ':'
		<< m_metrics.audioNullFrames - m_lastMetrics.audioNullFrames << ':'
		<< m_metrics.rejectedChallenges - m_lastMetrics.rejectedChallenges << ':'
		<< m_metrics.serverFull - m_lastMetrics.serverFull << ':'
		<< std::setw(6) << m_metrics.inboundPayloads - m_lastMetrics.inboundPayloads << ':'
		<< std::setw(6) << m_metrics.outboundPayloads - m_lastMetrics.outboundPayloads << ':'
		<< std::setw(2) << m_metrics.sendReliable - m_lastMetrics.sendReliable << "  ["
		<< m_metrics.dropOverflow - m_lastMetrics.dropOverflow << ':'
		<< m_metrics.tooManyTalkers - m_lastMetrics.tooManyTalkers << ':'
		<< m_metrics.dropStopFrame - m_lastMetrics.dropStopFrame << ':' // incremental
		<< m_metrics.dropStopFrame << "]  "; // absolute

	static float accumulate_nullframe_percentage = 0.0f;
	static float peak_nullframe_percentage = 0.f;
	int audioNullFrames = m_metrics.audioNullFrames - m_lastMetrics.audioNullFrames;
	int inboundPayloads = m_metrics.inboundPayloads - m_lastMetrics.inboundPayloads;
	int totalAudioPayloads = audioNullFrames + inboundPayloads;
	float nullframe_percentage = (totalAudioPayloads > 0) ? float(audioNullFrames) / float(totalAudioPayloads) : 0.f;
	accumulate_nullframe_percentage += nullframe_percentage;
	float avg_nullframe_percentage = accumulate_nullframe_percentage / float(updateCount);
	
	if( nullframe_percentage > peak_nullframe_percentage )
	{
		peak_nullframe_percentage = nullframe_percentage;
	}

	//
	// track all dropped packets
	//
	int dropBadCid = m_metrics.dropBadCid - m_lastMetrics.dropBadCid;
	int dropBadHeader = m_metrics.dropBadHeader - m_lastMetrics.dropBadHeader;
	//m_metrics.dropBadSeq - m_lastMetrics.dropBadSeq,
	int dropBadState = m_metrics.dropBadState - m_lastMetrics.dropBadState;
	int dropUnexpectedMsg = m_metrics.dropUnexpectedMsg - m_lastMetrics.dropUnexpectedMsg;
	int dropUnexpectedSeq = m_metrics.dropUnexpectedSeq - m_lastMetrics.dropUnexpectedSeq;
	int dropBadAddress = m_metrics.dropBadAddress - m_lastMetrics.dropBadAddress;
	int dropPayloadSize = m_metrics.dropPayloadSize - m_lastMetrics.dropPayloadSize;
	int dropOverflow = m_metrics.dropOverflow - m_lastMetrics.dropOverflow;
    int dropLatePacketNotStarted = m_metrics.dropLatePacketNotStarted - m_lastMetrics.dropLatePacketNotStarted;
    int dropLatePacket = m_metrics.dropLatePacket - m_lastMetrics.dropLatePacket;
	int dropForced = m_metrics.dropForced - m_lastMetrics.dropForced;

	int dropTotal = dropForced + dropBadCid + dropBadHeader + dropBadState + dropUnexpectedMsg + dropUnexpectedSeq + dropBadAddress + dropPayloadSize + dropOverflow + dropLatePacketNotStarted + dropLatePacket;

	static float accumulate_drop_percentage = 0.0f;
	static float peak_drop_percentage = 0.f;
	int receivedPackets = m_metrics.readPacket - m_lastMetrics.readPacket;
	float drop_percentage = (receivedPackets > 0) ? float(dropTotal) / float(receivedPackets) : 0.f;
	accumulate_drop_percentage += drop_percentage;
	float avg_drop_percentage = accumulate_drop_percentage / float(updateCount);

	if( drop_percentage > peak_drop_percentage )
	{
		peak_drop_percentage = drop_percentage;
	}

	static float accumulate_resend_percentage = 0.0f;
	static float peak_resend_percentage = 0.0f;
	int sendReliable = m_metrics.sendReliable - m_lastMetrics.sendReliable;
	int resends = m_metrics.resends - m_lastMetrics.resends;
	static float ONE_OVER_RESEND_COUNT = 1.0f / float(protocol::RESEND_COUNT);
	float resend_percentage = (sendReliable > 0) ? (ONE_OVER_RESEND_COUNT * float(resends) / float(sendReliable)) : 0.f;
	accumulate_resend_percentage += resend_percentage;
	float avg_resend_percentage = accumulate_resend_percentage / float(updateCount);

	if( resend_percentage > peak_resend_percentage )
	{
	    peak_resend_percentage = resend_percentage;
	}

	nullframe_percentage *= 100.f;
	avg_nullframe_percentage *= 100.f;
	resend_percentage *= 100.f;
	avg_resend_percentage *= 100.f;
	drop_percentage *= 100.f;
	avg_drop_percentage *= 100.f;

	std::stringstream metrics_combined;
	metrics_combined
		<< '['
			<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << nullframe_percentage
			<< ' '
			<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(8) << avg_nullframe_percentage
			<< ' '
			<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << 100.f * peak_nullframe_percentage
		<< "] "
		<< '['
			<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << drop_percentage
			<< ' '
			<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(8) << avg_drop_percentage
			<< ' '
			<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << 100.f * peak_drop_percentage
		<< "] "
		<< '['
			<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << resend_percentage
			<< ' '
			<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(8) << avg_resend_percentage
			<< ' '
			<< std::setw(2) << std::setiosflags(std::ios::fixed) << std::setprecision(2) << 100.f * peak_resend_percentage
		<< "] "
		<< '['
			<< m_metrics.dropConnection - m_lastMetrics.dropConnection << ':' // incremental
			<< m_metrics.dropConnection // absolute
		<< ']';

	std::cerr << timeStr << '[' << common::getVoipPort() << "] " << metrics.str() << ' ' << metrics_system.str() << ' ' << metrics_combined.str() << std::endl;
}

class AllocMessage : public sonar::Message
{

};

const Server::Config &Server::getConfig(void) const
{
	return m_config;
}

Server::Metrics &Server::getMetrics(void)
{
	return m_metrics;
}

const Server::Metrics &Server::getMetrics(void) const
{
	return m_metrics;
}

void Server::poll(void)
{
	for (;;)
	{
		Message message;
		
		sonar::socklen_t len = sizeof(struct sockaddr_in);
		struct sockaddr_in remoteAddr;

		int result = ::recvfrom(m_sockfd, (char *) message.getBufferPtr(), message.getMaxSize(), common::DEF_MSG_NOSIGNAL, (struct sockaddr *) &remoteAddr, &len);
		
		if (result < 0) {
			if (errno != EAGAIN) {
				SYSLOGX(LOG_ERR, "socket recv err=%d", errno);
				++m_metrics.recvErrors;
			}
		}

		if (result < (int) sizeof(sonar::protocol::MessageHeader))
		{
			break;
		}
		
		m_metrics.rxPackets ++;

		message.update(result, remoteAddr);
        message.setTimestamp(common::getTimeAsMSEC()); // jitter

		processMessage(message);
	}

	update();

	while (!m_clientDeleteList.empty())
	{
		delete m_clientDeleteList.front();
		m_clientDeleteList.pop_front();
	}
}

void Server::run(void)
{
	for (;;)
	{
		poll();
		common::sleepExact(1);

		if (m_shutdownTime > 0 && getTime() > m_shutdownTime)
		{
			SYSLOG (LOG_INFO, "Shutdown delay specified, shutting down...\n");
			break;
		}

#if defined(_DEBUG) && (_WIN32)
		if (kbhit())
		{
			break;
		}
#endif
	}
}

void Server::destroyClient(ClientConnection *client)
{
	assert (client->isDisconnected());
	assert (m_clients[client->getCID()] == client);

	freeCid(client->getCID());

	m_clientDeleteList.push_back(client);
}

size_t Server::getClientCount(void) const
{
	return m_clientCount;
}

void Server::updateServerStats() const
{
	serverStats.currentUserCount = getClientCount();
	if (serverStats.currentUserCount > serverStats.peakUserCount)
		serverStats.peakUserCount = serverStats.currentUserCount;

	serverStats.currentChannelCount = m_channels.size();
	if (serverStats.currentChannelCount > serverStats.peakChannelCount)
		serverStats.peakChannelCount = serverStats.currentChannelCount;

	serverStats.avgTalkersPerChannel = 0.0f;
	auto lastChannel = m_channels.end();
	for (auto i = m_channels.begin(); i != lastChannel; ++i)
	{
		size_t channelCount = (*i)->getClientCount();
		if (channelCount > serverStats.peakUsersPerChannel)
			serverStats.peakUsersPerChannel = channelCount;

		// average talkers per frame
		float avgTalkersPerFrame = 0.0f;
		if( (*i)->getStatistics().frames > 0 )
		{
			avgTalkersPerFrame = (float)((*i)->getStatistics().talkers) / float((*i)->getStatistics().frames);
			//(*i)->resetTalkers();
		}
		serverStats.avgTalkersPerChannel += avgTalkersPerFrame;
	}
	
	// compute average number of talkers per channel
	if( m_channels.size() > 0 )
	  serverStats.avgTalkersPerChannel /= (float)m_channels.size();
}

void Server::unregisterClient (ClientConnection &client, sonar::CString &reasonType, sonar::CString &reasonDesc)
{
	if (client.isConnected())
	{
		client.disconnect(reasonType, reasonDesc);
	}

	Channel &channel = client.getChannel();

	channel.removeClient(client, reasonType, reasonDesc);

	client.getOperator().unmapClient(&client);
	m_clientCount --;

	SYSLOG (LOG_INFO, "User [%s] disconnected from channel [%s] now with %lu user(s)."
		, client.getUserId().c_str()
		, channel.getId().c_str()
		, channel.getClientCount());
		
	m_backendClient->eventClientUnregistered (
			client.getOperator().getId().c_str(),
			client.getUserId().c_str(),
			channel.getId().c_str(),
			getClientCount());

	if (channel.getClientCount() == 0)
	{
		Channel *pChannel = &channel;

		client.getOperator().unmapChannel(pChannel);

		for (ChannelList::iterator iter = m_channels.begin(); iter != m_channels.end(); iter ++)
		{
			if (*iter == pChannel)
			{
				// update the relevant stats
				++serverStats.numCompletedChannels;
				const Channel::Statistics& stats = (*iter)->getStatistics();
				size_t channelDurationInSecs = static_cast<size_t>((getTime() - stats.created) / 1000);
				if (channelDurationInSecs < serverStats.minChannelDurationInSecs)
					serverStats.minChannelDurationInSecs = channelDurationInSecs;
				else if (channelDurationInSecs > serverStats.maxChannelDurationInSecs)
				      serverStats.maxChannelDurationInSecs = channelDurationInSecs;
				serverStats.totalChannelDurationInSecs += channelDurationInSecs;

				serverStats.avgChannelDurationInSecs
					= serverStats.totalChannelDurationInSecs / serverStats.numCompletedChannels;

				m_channels.erase(iter);
				break;
			}
		}

		SYSLOG (LOG_INFO, "Channel [%s] destroyed.", pChannel->getId().c_str());

		m_backendClient->eventChannelDestroyed (
			client.getOperator().getId().c_str(),
			pChannel->getId().c_str());


		if (m_config.channelStatsLogging && m_chanStatsLog)
		{
			const Channel::Statistics &stats = pChannel->getStatistics();

			fprintf(m_chanStatsLog, "Decommissioning %s/%s\t%lld\t%lu\t%llu\t%lu\t%llu\t%lu\t%lu\n", 
				pChannel->getId().c_str(), 
				pChannel->getDesc().c_str(),
				getTime() - stats.created,
				stats.inPayloads,
				stats.inPayloadsSize,
				stats.outPayloads,
				stats.outPayloadsSize,
				stats.outJoinNotifs,
				stats.outPartNotifs);

			fprintf(stderr, "Decommissioning %s/%s %lld %lu %llu %lu %llu %lu %lu\n", 
				pChannel->getId().c_str(), 
				pChannel->getDesc().c_str(),
				getTime() - stats.created,
				stats.inPayloads,
				stats.inPayloadsSize,
				stats.outPayloads,
				stats.outPayloadsSize,
				stats.outJoinNotifs,
				stats.outPartNotifs);

			fflush(stderr);
			fflush(m_chanStatsLog);
		}

		delete pChannel;
	}

	client.detach();
}

void Server::registerClient(const sockaddr_in &source, CString &connectToken, sonar::protocol::SEQUENCE txNextSeq, unsigned long version)
{
	if (m_metrics.currOverload > 60)
	{
		++getMetrics().serverFull;
		ClientConnection::sendRegisteReplyError(source, "SERVER_IS_FULL", "Overload");
		SYSLOG (LOG_ERR, "SERVER_IS_FULL: Overload, currOverload=%d", m_metrics.currOverload);
		return;
	}

	if (connectToken.empty())
	{
		++getMetrics().tokenInvalid;
		ClientConnection::sendRegisteReplyError(source, "TOKEN_INVALID_ARGUMENT", "TokenEmpty");
		SYSLOG (LOG_ERR, "TOKEN_INVALID_ARGUMENT: TokenEmpty");
		return;
	}

	// Parse token
	sonar::Token token;
	if (!token.parse(connectToken))
	{
		++getMetrics().tokenInvalid;
		ClientConnection::sendRegisteReplyError(source, "TOKEN_INVALID_ARGUMENT", "TokenParse");
		SYSLOG (LOG_ERR, "TOKEN_INVALID_ARGUMENT: TokenParse, connectToken=%s", connectToken.c_str());
		return;
	}

	CString &operatorId = token.getOperatorId();
	CString &userId = token.getUserId();
	CString &userDesc = token.getUserDesc();
	CString &channelId = token.getChannelId();
	CString &channelDesc =token.getChannelDesc();

	SYSLOG (LOG_INFO, "Server::registerClient: operatorId=%s, userId=%s, userDesc=%s, channelId=%s, channelDesc=%s"
		, operatorId.c_str()
		, userId.c_str()
		, userDesc.c_str()
		, channelId.c_str()
		, channelDesc.c_str());

	if (operatorId.size() > 255 ||
		userId.size() > 255 ||
		userDesc.size() > 255 ||
		channelId.size() > 255 ||
		channelDesc.size() > 255)
	{
		++getMetrics().tokenInvalid;
		ClientConnection::sendRegisteReplyError(source, "TOKEN_INVALID_ARGUMENT", "TokenSizes");
		SYSLOG (LOG_ERR, "TOKEN_INVALID_ARGUMENT: TokenSizes, sizes=%lu:%lu:%lu:%lu:%lu"
			, operatorId.size()
			, userId.size()
			, userDesc.size()
			, channelId.size()
			, channelDesc.size());
		return;
	}

	CString &creationTime = token.getCreationTime ();
	CString &sign = token.getSign();

	// Validate timestamp
	INT64 templl;
	if (sscanf (creationTime.c_str (), "%" PRId64, &templl) != 1)
	{
		++getMetrics().tokenInvalid;
		ClientConnection::sendRegisteReplyError(source, "TOKEN_INVALID_ARGUMENT", "TokenFormatTime");
		SYSLOG (LOG_ERR, "TOKEN_INVALID_ARGUMENT: TokenFormatTime, creationTime=%s", creationTime.c_str());
		return;
	}

	time_t now = time(0);
	time_t timeCreation = (time_t) templl;

	if (now - timeCreation > protocol::TOKEN_MAX_AGE && m_backendClient->getRSAPubKey() != NULL)
	{
		++getMetrics().tokenExpired;
		ClientConnection::sendRegisteReplyError(source, "TOKEN_EXPIRED", "");
		SYSLOG (LOG_ERR, "TOKEN_EXPIRED, now=%lu, timeCreation=%lu", now, timeCreation);
		return;
	}

	// Validate signature
	char signature[protocol::RSA_SIGNATURE_BASE64_MAX_LENGTH];
	size_t cbSignature = sizeof(signature);

	if (sign.size() > protocol::RSA_SIGNATURE_BASE64_MAX_LENGTH)
	{
		++getMetrics().tokenInvalid;
		ClientConnection::sendRegisteReplyError(source, "TOKEN_INVALID_ARGUMENT", "SignatureLength");
		SYSLOG (LOG_ERR, "TOKEN_INVALID_ARGUMENT: SignatureLength, size=%lu", sign.size());
		return;
	}

	if (m_backendClient->getRSAPubKey())
	{
		if (!BackendClient::decodeBase64(sign, signature, &cbSignature))
		{
			++getMetrics().tokenInvalid;
			ClientConnection::sendRegisteReplyError(source, "TOKEN_INVALID_ARGUMENT", "SignatureDecode");
			SYSLOG (LOG_ERR, "TOKEN_INVALID_ARGUMENT: SignatureDecode, sign=%s, signature=%s", sign.c_str(), signature);
			return;
		}

		String message = connectToken.substr(0, connectToken.find_last_of('|') + 1);

		SYSLOG (LOG_DEBUG, "RSACheck, message=%s, sign=%s, signature=%s, cbSignature=%lu", message.c_str(), sign.c_str(), signature, cbSignature);
		if (!BackendClient::rsaValidateToken (m_backendClient->getRSAPubKey(), message, signature, cbSignature))
		{
			ClientConnection::sendRegisteReplyError(source, "TOKEN_INVALID", "RSACheck");
			SYSLOG (LOG_ERR, "TOKEN_INVALID: RSACheck");
			return;
		}
	}

	int cid = allocateCid();

	if (cid == -1)
	{
		++getMetrics().serverFull;
		ClientConnection::sendRegisteReplyError(source, "SERVER_IS_FULL", "OutOfCID");
		SYSLOG (LOG_ERR, "SERVER_IS_FULL: OutOfCID");
		return;
	}

	assert (m_clients[cid] == NULL);


	Operator &oper = getOperator(operatorId);

	ClientConnection *oldClient = oper.getClient(userId);

	if (oldClient)
	{
        const sockaddr_in &oldAddress = oldClient->getRemoteAddress();

        if (oldAddress.sin_addr.s_addr != source.sin_addr.s_addr ||
            oldAddress.sin_port != source.sin_port) // if user is trying to connect from another location
        {
		    oldClient->disconnect("REPLACED", "LoggedinElsewhere");
            SYSLOG (LOG_ERR, "REPLACED: LoggedinElsewhere, userId=%s", userId.c_str());
        }
        else
        {
            SYSLOG (LOG_INFO, "IGNORED: AlreadyConnected, userId=%s", userId.c_str());
            return; // ignore
        }
	}

	Channel *channel = oper.getChannel(channelId);

	if (channel == NULL)
	{
		channel = new Channel(channelId, channelDesc, getTime() + (m_spread % protocol::FRAME_TIMESLICE_MSEC), getTime());
		oper.mapChannel(channel);
		m_channels.push_back(channel);
		m_spread ++;
	}
	else
	{
		if (channel->getClientCount() + 1 > sonar::protocol::CHANNEL_MAX_CLIENTS)
		{
			++getMetrics().serverFull;
			freeCid(cid);
			ClientConnection::sendRegisteReplyError(source, "SERVER_IS_FULL", "ChannelFull");
			SYSLOG (LOG_ERR, "SERVER_IS_FULL: ChannelFull, channelId=%s", channel->getId().c_str());
			return;
		}
	}

	ClientConnection *client = new ClientConnection(version, userId, userDesc, cid, txNextSeq, &source, channel, &oper, getTime() + (m_spread % DefaultConnectionOptions.clientTickInterval), m_config.audioLoss, m_config.jitterBufferSize, m_config.jitterScaleFactor);
	m_spread ++;

	oper.mapClient(client);
	
	channel->addClient(*client);

    m_connectedClientIds.push_back(cid);
	m_clients[cid] = client;

	size_t len;

	ClientConnection **clients = channel->getClients(&len);
	
	ClientConnection::Message msgRegisterReply(sonar::protocol::MSG_REGISTERREPLY, client->getCID(), true);

	msgRegisterReply.writeUnsafeSmallString("SUCCESS");
	msgRegisterReply.writeUnsafeSmallString("");
	msgRegisterReply.writeUnsafeLong(client->getNextRxSeq());
	msgRegisterReply.writeUnsafeShort ((unsigned short) client->getCID());
	msgRegisterReply.writeUnsafeByte( (unsigned char) client->getCHID());
	msgRegisterReply.writeUnsafeByte((unsigned char) channel->getClientCount());
	client->sendMessage(msgRegisterReply);

	ClientConnection::Message *fragment = new ClientConnection::Message(sonar::protocol::MSG_PEERLIST_FRAGMENT, 0, true);
	
	for (size_t index = 0; index < len; index ++)
	{
		ClientConnection *peer = clients[index];

		if (peer == NULL)
			continue;
	
		size_t peerSize = 1 + peer->getUserId().size() + peer->getUserDesc().size() + 1 + 1;

		if (fragment->getBytesLeft() < peerSize)
		{
			client->sendMessage(*fragment);
			delete fragment;
			fragment = new ClientConnection::Message(sonar::protocol::MSG_PEERLIST_FRAGMENT, client->getCID(), true);
		}

		assert (fragment->getBytesLeft() >= peerSize);

		fragment->writeUnsafeByte( (unsigned char) peer->getCHID());
		fragment->writeUnsafeSmallString(peer->getUserId());
		fragment->writeUnsafeSmallString(peer->getUserDesc());
	}

	client->sendMessage(*fragment);
	delete fragment;
	m_clientCount ++;
	m_metrics.connections++;

	SYSLOG (LOG_INFO, "User [%s] connected to channel [%s] now with %lu user(s)."
		, client->getUserId().c_str()
		, channel->getId().c_str()
		, channel->getClientCount());

	m_backendClient->eventClientRegisteredToChannel(
		oper.getId().c_str(),
		client->getUserId().c_str(),
		client->getUserDesc().c_str(),
		channel->getId().c_str(),
		channel->getDesc().c_str(),
		inet_ntoa(source.sin_addr),
		ntohs (source.sin_port),
		getClientCount());
}

int Server::allocateCid(void)
{
    int cid = (m_cids.empty()) ? -1 : m_cids.popBegin();
    return cid;
	}

void Server::freeCid(int cid)
{
	m_clients[cid] = NULL;
    ConnectedClientIds::iterator where = std::find(m_connectedClientIds.begin(), m_connectedClientIds.end(), cid);
    if (where != m_connectedClientIds.end()) {
        ConnectedClientIds::iterator last = m_connectedClientIds.end() - 1;
        if (where != last)
            *where = *last;
        m_connectedClientIds.pop_back();
    } else {
        printf("failed to locate %d in m_connectedClientIds", cid);
    }
	m_cids.pushEnd(cid);
}

Operator &Server::getOperator(CString &id)
{
	OperatorMap::iterator iter = m_operators.find(id);

	Operator *oper;

	if (iter == m_operators.end())
	{
		if (m_operators.size() > 10000)
		{
			SYSLOG (LOG_EMERG, "FATAL: Operator count is too high");
			exit(-1);
		}

		oper = new Operator(id);
		m_operators[id] = oper;
	}
	else
		oper = iter->second;

	return *oper;
}

bool Server::cleanup(void)
{
	size_t usersCleaned = 0;

    for (ConnectedClientIds::iterator i(m_connectedClientIds.begin()), last(m_connectedClientIds.end());
        i != last;
        ++i)
	{
		ClientConnection *client = m_clients[*i];
		
		if (client == NULL)
			continue;

		usersCleaned ++;

		client->disconnect("LEAVING", "ServerShutdown");
	}

	assert (m_channels.empty());

	for (OperatorMap::iterator iter = m_operators.begin(); iter != m_operators.end(); iter ++)
	{
		Operator *oper = iter->second;

		assert (oper->getChannelMap().empty());
		assert (oper->getUserMap().empty());

		delete oper;
	}

	m_operators.clear();
	
	while (!m_clientDeleteList.empty())
	{
		delete m_clientDeleteList.front();
		m_clientDeleteList.pop_front();
	}

	return !((usersCleaned) > 0);
}

void Server::setRsaPubKey(const void *pubKey, size_t length)
{
	m_backendClient->updateRSAPubKey(pubKey, length);
}

BackendClient &Server::getBackendClient(void)
{
	return *m_backendClient;
}

void Server::updateBackendEventInterval(int interval)
{
    if( (interval > 0) && (m_config.backendEventInterval != interval) )
    {
        m_config.backendEventInterval = interval;
        m_healthCheck->updateBackendEventInterval(interval);
    }
}

}
