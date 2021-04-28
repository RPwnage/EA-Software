#pragma once

#include <SonarCommon/Common.h>
#include <SonarConnection/Message.h>
#include <SonarConnection/BaseConnection.h>
#include "AudioPayload.h"

#include <algorithm>
#include <cstdint>

namespace sonar
{

class Channel;
class Operator;

class ClientConnection : public sonar::BaseConnection
{
public:
	typedef sonar::Message Message;

	struct PercentageStats
	{
		float loss;
		float oos;
		
		PercentageStats()
            : loss(0.0f)
            , oos(0.0f)
        {
        }
	};

    struct JitterStats
    {
        float arrivalJitterMean;
        unsigned int arrivalJitterMax;
        float playbackJitterMean;
        unsigned int playbackJitterMax;

        JitterStats()
            : arrivalJitterMean(0)
            , arrivalJitterMax(0)
            , playbackJitterMean(0)
            , playbackJitterMax(0)
        {
        }
    };

    struct NetworkStats
    {
        bool valid;
        unsigned int loss;
        unsigned int jitter;
        unsigned int ping;
        unsigned int received;
        unsigned int quality;

        NetworkStats()
        {
            reset();
        }

        void reset() {
            valid = false;
            loss = 0;
            jitter = 0;
            ping = 0;
            received = 0;
            quality = 0;
        }
    };

	ClientConnection(unsigned long version, CString &userId, CString &userDesc, int cid, sonar::protocol::SEQUENCE txNextSeq, const sockaddr_in *addr, Channel *channel, Operator *oper, long long nextFrame, int loss = 0, int bufferSize = 0, int jitterScaleFactor = 2000);
	~ClientConnection(void);

	void update(long long now);

	void detach();
	bool isDetached();

	Operator &getOperator(void) const;
	Channel &getChannel(void) const;
	CString &getName(void) const { return m_name; }
	void setName(CString &name) { m_name = name; }

	sonar::protocol::SEQUENCE getNextRxSeq(void);
	const sockaddr_in &getRemoteAddress();

	AudioPayload *getInAudio(void);

	static AudioPayload *m_nullFrame;

	typedef String BlockingId;
	typedef SonarVector<BlockingId> ClientBlockList;

	const ClientBlockList& blockList() const { return m_BlockList; }

	// rank
	void updateRankScore();
    float rank() const { return m_rankScore; }

	bool operator < (const ClientConnection* connection) const;

	static void writeMetricsJsonAllZero(char *filename);
    void writeMetricsJson(char *filename);

	const ClientToServerStats &getClientStats() const { return m_clientStats; }
	const unsigned int getNumClientStatsReceived() const { return m_numClientStatsReceived; }
	void resetClientStats();
	const PercentageStats &getPercentageStats() const { return m_percentageStats; }
	bool isBad(int threshold); // threshold = [0, 100]

    // jitter
    const JitterStats &getJitterStats();
    void resetJitterStats();
    const JitterStats& updateJitterStats();

    // network
    const NetworkStats &getNetworkStats() const { return m_networkStats; }
    const NetworkStats& updateNetworkStats();

    // compatibility
    bool isProtocolSupported(unsigned long version) const {
        return (version == sonar::protocol::PROTOCOL_CLIENTONLYJITTERBUFFERAWARE ||
                version == sonar::protocol::PROTOCOL_FRAME_COUNTER ||
                version == sonar::protocol::PROTOCOL_NETWORK_STATS ||
                version == sonar::protocol::PROTOCOL_VERSION_ORIGINAL);
    }
    bool hasProtocolFrameCounter() const { return (m_version >= sonar::protocol::PROTOCOL_FRAME_COUNTER); }
    bool hasProtocolNetworkStats() const { return (m_version >= sonar::protocol::PROTOCOL_NETWORK_STATS); }

	static void sendRegisteReplyError(const sockaddr_in &addr, sonar::CString &reasonType, sonar::CString &reasonDesc);
	static void socketSendMessage(const Message &message, const struct sockaddr_in &addr);

    virtual bool isClientOnlyJitterBufferAware() const {
        return m_version >= protocol::PROTOCOL_CLIENTONLYJITTERBUFFERAWARE;
    }

private:
	virtual void onDisconnected(sonar::CString &reasonType, sonar::CString &reasonDesc);
	virtual void onConnecting(void);
    virtual void onConnected(void);
	virtual void handleMessage(Message &message);
	virtual void handleEcho(Message &message);
	virtual void socketSendMessage(const Message &message);
	virtual bool validatePacket(const Message &message);
	virtual void onMetricEvent(MetricEvent evt, int count = 1)
	{
		ClientConnection::logMetricEvent(evt);
	}

	void handleRegister(Message &message);
	void handleAudioToServer(Message &message);
	void handleAudioStop(Message &message);
	void handleBlockListComplete(Message &message);
	void handleBlockListFragment(Message &message);
	void handleClientStats(Message& message);
    void handleNetworkPing(Message& message);

	static void logMetricEvent(const MetricEvent evt, int count = 1);
    static void writeMetricsJson(
        char *filename,
        unsigned int deltaPlaybackMean,
        unsigned int deltaPlaybackMax,
        unsigned int receiveToPlaybackMean,
        unsigned int receiveToPlaybackMax,
        ClientToServerStats const& clientStats,
        JitterStats const& jitterStats,
        NetworkStats const& networkStats);

	Operator *m_operator;
	struct sockaddr_in m_addr;
	Channel *m_channel;
	bool m_isDetached;

	long long m_nextFrame;

    typedef std::deque<AudioPayload*> PayloadQueue;
	struct
	{
        PayloadQueue payloadQueue;
        CriticalSection payloadQueueCS;
		bool started;
		unsigned char takeNumber;
		int loss;
		int talkBalance;
	} m_in;

	int m_audioLoss;
	unsigned int m_AudioLargestIncomingSeq;
	unsigned int m_AudioMessagesReceived;
	unsigned int m_AudioIncomingMessageOverflowCounter;

	SonarVector<BlockingId> m_BlockList;
	SonarVector<BlockingId> m_InboundBlockList;
	
	ClientToServerStats m_clientStats;
	unsigned int m_numClientStatsReceived;

	// rank
	float m_rankScore;
	PercentageStats m_percentageStats;
	String m_name;

    // jitter
    JitterStats m_jitterStats;
    int m_jitterScaleFactor;

    // network
    NetworkStats m_networkStats;

    // compatibility
    unsigned long m_version;
};

}
