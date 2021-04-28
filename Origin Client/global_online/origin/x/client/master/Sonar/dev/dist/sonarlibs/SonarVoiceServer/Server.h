#pragma once
#include <SonarCommon/Common.h>
#include "ClientConnection.h"
#include "BackendClient.h"
#include "HealthCheck.h"
#include <map>
#include <deque>
#include <exception>
#include <stdio.h>
#include "Allocator.h"

namespace sonar
{

class Operator;

class Server
{
public:
	static const int VERSION_MAJOR=10;
	static const int VERSION_MINOR=2;
	static const int VERSION_PATCH=0;
	static const char *VERSION_NAME;

	static const long long BACKEND_FRAME_INTERVAL_MSEC = 100;
	static const long long BACKEND_EVENT_INTERVAL_MSEC = 60000;
	static const int BACKEND_HEALTH_PORT = 10189;
	static const long long METRICS_FILE_INTERVAL_MSEC = 60000;
	static const long long CLIENT_STATS_FILE_INTERVAL_MSEC = 60000;
	static const long long HEALTHCHECK_INTERVAL_MSEC = 60000;
	static const long long PRINTOUT_INTERVAL_MSEC = 10000;
	static const int BAD_CHANNEL_PERCENT = 20;
	static const int BAD_CLIENT_PERCENT = 3;
	static const int JITTER_SCALE_FACTOR = 2000;

	class StartupException : public std::exception
	{
	public:
		StartupException(const char *msg) 
			: m_msg(msg)
		{
		}

		virtual const char *what() const throw()
		{
			return m_msg;
		}

	private:
		const char *m_msg;
	};

	struct HealthStats
	{
		// server
		float dropConnectionPercentage;
		float dropPacketsPercentage;
		float resendPercentage;
		float nullFramePercentage;
		// client
		float lostMessagePercentage;
		float outOfSequenceMessagePercentage;
		
		HealthStats()
		: dropConnectionPercentage(0.0f)
		, dropPacketsPercentage(0.0f)
		, resendPercentage(0.0f)
		, nullFramePercentage(0.0f)
		, lostMessagePercentage(0.0f)
		, outOfSequenceMessagePercentage(0.0f)
		{}
	};

	struct Metrics
	{
		int tooManyTalkers;
		int rejectedChallenges;
		int audioNullFrames;
		int dropPayloadSize;
		int dropOverflow;
        int dropLatePacketNotStarted;
		int dropLatePacket;
		int dropUnexpectedMsg;
		int dropUnexpectedSeq;
		int dropBadCid;
		int dropBadHeader;
		int dropBadState;
		int dropBadAddress;
		int dropBadSeq;
		int dropForced;
		int dropEmptyPayload;
		int dropStopFrame;
		int dropTalkBalance;
		int ackUnexpectedSeq;
		int sendPacket;
		int readPacket;
		int resends;
		int rxPackets;
		int txPackets;
		int inboundPayloads;
		int outboundPayloads;
		int sendReliable;
		int currOverload;
		long long cpuTime;
		int tokenInvalid;
		int tokenExpired;
		int serverFull;
		unsigned int audioIncomingMessagesBadSequence;
		unsigned int audioIncomingMessagesLost;
		int dropConnection;
		int connections;
		int recvErrors;
		int sendErrors;
		int dropTruncatedMessages;
        int maxAudioLoss;
	};

	struct PercentageStats
	{
		float loss;
		float oos;
        float jitter;

		PercentageStats() : loss(0.0f), oos(0.0f), jitter(0.0f) {}
	};
	
	struct LatencyStats
	{
		unsigned int deltaPlaybackMean;
		unsigned int deltaPlaybackMax;
		unsigned int receiveToPlaybackMean;
		unsigned int receiveToPlaybackMax;
		
		LatencyStats() : deltaPlaybackMean(0), deltaPlaybackMax(0), receiveToPlaybackMean(0), receiveToPlaybackMax(0) {}
	};

    struct JitterStats
    {
        float jitterMean;
        unsigned int jitterMax;

        JitterStats() : jitterMean(0), jitterMax(0) {}
    };

    struct NetworkStats
    {
        float loss;
        float jitter;
        float ping;
        float received;
        float quality;

        NetworkStats()
        {
            reset();
        }

        void reset() {
            loss = 0.f;
            jitter = 0.f;
            ping = 0.f;
            received = 0.f;
            quality = 0.f;
        }
    };

	class Config
	{
	public:
		Config()
		{
			talkBalanceMax = protocol::IN_TAKE_MAX_FRAMES;
			talkBalancePenalty = talkBalanceMax;
			talkBalanceMin = talkBalanceMax / 2;
			shutdownDelay = 0;
			channelStatsLogging = false;
			gnuPlot = false;
			audioLoss = 0;
			loadTest = false;
#if ENABLE_HEALTH_CHECK
			healthCheckInterval = HEALTHCHECK_INTERVAL_MSEC;
#endif
			backendEventInterval = BACKEND_EVENT_INTERVAL_MSEC;
			backendHealthPort = 0;
			metricsFileInterval = METRICS_FILE_INTERVAL_MSEC;
			clientStatsFileInterval = CLIENT_STATS_FILE_INTERVAL_MSEC;
			printoutInterval = PRINTOUT_INTERVAL_MSEC;
			badChannelPercent = BAD_CHANNEL_PERCENT;
			badClientPercent = BAD_CLIENT_PERCENT;
			jitterBufferSize = 0;
			jitterScaleFactor = JITTER_SCALE_FACTOR;
		}

	public:
		int talkBalanceMax;
		int talkBalancePenalty;
		int talkBalanceMin;
		int shutdownDelay;
		bool channelStatsLogging;
		bool gnuPlot;
		int audioLoss;
		bool loadTest;
		int healthCheckInterval;
		int backendEventInterval;
		int backendHealthPort;
		int metricsFileInterval;
		int clientStatsFileInterval;
		int printoutInterval;
		int badChannelPercent;
		int badClientPercent;
		int jitterBufferSize;
		int jitterScaleFactor;
	};
	
	typedef SonarDeque<Channel *> ChannelList;
	typedef	SonarVector<Channel*> RankedChannelList;

	Server(CString &_hostname,
		CString &_voipAddress,
		CString &_controlAddress,  
		CString &_location,
		int _maxClients,
		int _voipPort,
		const Config &config);

	~Server(void);

	static Server &getInstance(void);
	void run(void);
	void poll(void);
	long long getTime() const;
	SOCKET getSocket(void);
	void registerClient(const sockaddr_in &source, CString &connectToken, sonar::protocol::SEQUENCE txNextSeq, unsigned long version);
	void unregisterClient(ClientConnection &client, sonar::CString &reasonType, sonar::CString &reasonDesc);
	void destroyClient(ClientConnection *client);
	
	void handleChallenge(Message &message);

	const sockaddr_in &getVoipAddress() const;

	bool cleanup(void);

	size_t getClientCount(void) const;
	const ChannelList &getChannelList() const { return m_channels; }

	Operator &getOperator(CString &id);

	void setRsaPubKey(const void *pubKey, size_t length);

	Metrics &getMetrics(void);
	const Metrics &getMetrics(void) const;
	const Config &getConfig(void) const;

	BackendClient &getBackendClient(void);

	void updateBackendEventInterval(int interval);

private:

	int allocateCid(void);
	void freeCid(int cid);
	
	void update();
	void frame();

	float computeCPUUsage(long long int lastCpuTime, int intervalMsec);
	float computeMemoryUsage();

	void updateServerStats() const;

	void writeMetricsJson();
	void writeChannelJson(unsigned int index);
    void writeChannelJsonAllZero(char *filename);
	void writeClientJson();
	void resetClientStats();
	void computeHealthStats(HealthStats& healthStats);
	void updateChannelRanking();
	void updatePercentageStats();
	void updateLatencyStats();
	void updateJitterStats();
	void updateNetworkStats();
	CString getWorstChannelList();
	int badChannelCount();

    // jitter
    void resetJitterStats();

	// loadtest
	void printMetricsForLoadTest(const char * timeStr, const std::stringstream & metrics_system);
private:
	
	FILE *m_chanStatsLog;

	BackendClient *m_backendClient;
	SOCKET m_sockfd;

	struct sockaddr_in m_voipAddress;
	
    typedef std::vector<int> ConnectedClientIds;
    ConnectedClientIds m_connectedClientIds;
    typedef std::vector<ClientConnection*> ConnectedClients;
    ConnectedClients m_clients;

	ChannelList m_channels;
	RankedChannelList m_rankedChannels;

	typedef SonarMap<String, Operator *> OperatorMap;

	Queue<int, sonar::protocol::SERVER_MAX_CLIENTS> m_cids;

	OperatorMap m_operators;

	typedef SonarDeque<ClientConnection *> ClientList;
	ClientList m_clientDeleteList;

	void processMessage(ClientConnection::Message &message);

	static Server *s_instance;

#if ENABLE_HEALTH_CHECK
	HealthCheck *m_healthCheck;
#endif

	unsigned int m_spread;
	long long m_nextBackendFrame;
	long long m_nextBackendEvent;
	long long m_nextMetricsFileWrite;
	long long m_nextClientStatsFileWrite;
	long long m_nextPrintout;
	long long m_updateTime;

	long long m_lastChallenge;
	int m_inTakeMaxFrames;

	long long m_shutdownTime;

	Metrics m_metrics;
	Metrics m_lastMetrics;
	Metrics m_lastEventMetrics; // for Backend server
	Metrics m_lastJsonMetrics; // for metrics.json file
	Config m_config;
	HealthStats m_healthStats;
	PercentageStats m_percentageStats;
	LatencyStats m_latencyStats;
	JitterStats m_jitterStats;
	NetworkStats m_networkStats;

	class AllocMessage : public sonar::Message
	{
	public:
		static void* operator new (size_t size); 
		static void operator delete (void *p);
	};

	size_t m_clientCount;

	Allocator<AllocMessage, 1, true> m_messageAllocator;

public:
	Allocator<AudioPayload, 1, true> m_payloadAllocator;

};

}
