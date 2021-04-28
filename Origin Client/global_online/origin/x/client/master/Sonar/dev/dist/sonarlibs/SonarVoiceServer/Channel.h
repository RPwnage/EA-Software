#pragma once

#include <SonarConnection/Protocol.h>
#include <SonarCommon/Common.h>
#include "ClientConnection.h"
#include <time.h>

#include <algorithm>
#include <bitset>
#include <vector>

#define INACTIVITY_INTERVAL_COUNT 2

namespace sonar
{

class Channel
{
public:
	struct Statistics
	{
		long long created;
		unsigned long inPayloads;
		unsigned long long inPayloadsSize;
		unsigned long outPayloads;
		unsigned long long outPayloadsSize;
		unsigned long outJoinNotifs;
		unsigned long outPartNotifs;
		
		// number of talkers
		unsigned long talkers;
		unsigned long frames;
	};

	struct PercentageStats
	{
		float loss;
		float oos;

		PercentageStats() : loss(0.0f), oos(0.0f) {}
	};

	struct LatencyStats
	{
		unsigned int deltaPlaybackMean;
		unsigned int deltaPlaybackMax;
		unsigned int receiveToPlaybackMean;
        unsigned int receiveToPlaybackMax;
		
		LatencyStats()
        {
            reset();
        }

        void reset()
        {
            deltaPlaybackMean = 0;
            deltaPlaybackMax = 0;
            receiveToPlaybackMean = 0;
            receiveToPlaybackMax = 0;
        }

        LatencyStats& operator +=(const sonar::ClientConnection& conn);
        LatencyStats& operator /=(int clientCount);
	};

    struct JitterStats
    {
        float arrivalJitterMean;
        unsigned int arrivalJitterMax;
        float playbackJitterMean;
        unsigned int playbackJitterMax;

        JitterStats()
        {
            reset();
        }

        void reset() {
            arrivalJitterMean = 0.0f;
            arrivalJitterMax = 0;
            playbackJitterMean = 0.0f;
            playbackJitterMax = 0;
        }
    };

    struct NetworkStats
    {
        bool valid;
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
            valid = false;
            loss = 0.f;
            jitter = 0.f;
            ping = 0.f;
            received = 0.f;
            quality = 0.f;
        }
    };

    struct MessageAndClient
    {
        MessageAndClient() : client(NULL) {}
        MessageAndClient(const ClientConnection::Message& message_, ClientConnection* client_)
            : message(message_)
            , client(client_)
        {}

        ClientConnection::Message message;
        const ClientConnection* client;
    };

	// Each block state is a string of 0s and 1s "0010000001..." representing whether there is a block to
	// another connection. Using a string instead of a bitset to be able to use sort() and unique() to
	// easily figure out which states are identical and can share a message.
	typedef std::vector< std::string > BlockingStates; 
	// A list of unique messages to use to send audio to various clients.
	typedef std::vector< MessageAndClient > BlockingMessages;
	// Maps a given connection to the message in BlockingMessages that will be used to send audio to it.
	typedef std::vector< ClientConnection::Message* > BlockingMessageMap;

	typedef	SonarVector<ClientConnection*> RankedClientList;

	Channel(CString &channelId, CString &channelDesc, long long nextFrame, long long now);
	~Channel(void);

	void addClient(ClientConnection &client);
	void removeClient(ClientConnection &client, sonar::CString &reasonType, sonar::CString &reasonDesc);
	size_t getClientCount() const;
	ClientConnection **getClients(size_t *outLen);
	ClientConnection *getClient(int index);
	CString &getId(void) const;
	CString &getDesc(void) const;

	CString &getName(void) const { return m_name; }
	void setName(CString &name) { m_name = name; }

	const Statistics &getStatistics();
	void resetTalkers() { m_stats.talkers = 0; m_stats.frames = 0; }
	
	void update(long long now);

	// client blocking members
	void clearBlocking(const ClientConnection& client);
	void updateBlocking(const ClientConnection& client, const ClientConnection::ClientBlockList& blockList);

	// rank
	void updateRankScore();
	void updateLatencyStats();
	const JitterStats& updateJitterStats();
	const NetworkStats& updateNetworkStats();
	void updateStats();
	bool operator < (const Channel* channel) const;
	void updateClientRanking();
	void writeClientJson();
	void writeClientJsonAllZero(char *filename);

	static void writeMetricsJsonAllZero(char *filename);
	void writeMetricsJson(char *filename);

	const PercentageStats &getPercentageStats() const { return m_percentageStats; }
	const LatencyStats &getLatencyStats() const { return m_latencyStats; }
    const JitterStats &getJitterStats() const { return m_jitterStats; }
    const NetworkStats &getNetworkStats() const { return m_networkStats; }
	CString getWorstClientList(); // top 10
	bool isBad(int channelThreshold, int clientThreshold); // threshold = [0, 100]
	void resetClientStats();
    void resetJitterStats();

private:
	void frame(void);
	
	String m_id;
	String m_name;
	String m_desc;
	size_t m_clientCount;
	ClientConnection *m_clients[sonar::protocol::CHANNEL_MAX_CLIENTS];

	enum AudioState
	{
		Started,
		Stopped
	};

	void setAudioState(AudioState state);
	AudioState getAudioState();
	unsigned char getTakeNumber();
	
	AudioState m_audioState;
	unsigned char m_takeNumber;

	long long m_nextFrame;

	Statistics m_stats;
	unsigned int m_LastOutgoingAudioMessageSeq;

	// inactivity
	void initInactivity();
	void resetInactivity();
	void checkInactivity();
	void sendInactivityMessage(unsigned long interval);

	long long m_InactivityEventTime;
	bool m_InactivityCheck;
	unsigned long m_InactivityIntervals[INACTIVITY_INTERVAL_COUNT];
	int m_InactivityCheckIndex;

	void addBlocking(const ClientConnection& client);
	void removeBlocking(const ClientConnection& client);
	void recomputeBlockingMessages();
	void debugDumpBlockingInfo() const;
    void debugDumpBlockingMessageClientMap() const;

	BlockingStates m_BlockingStates;
	BlockingMessageMap m_BlockingMessageMap;
	BlockingMessages m_BlockingMessages;
	
	// rank
	void writeClientJson(unsigned int index);

    static void writeMetricsJson(
        char *filename,
        int clientCount,
        float avgTalkers,
        ClientToServerStats const& clientStats,
        LatencyStats const& latencyStats,
        JitterStats const& jitterStats,
        NetworkStats const& networkStats
    );

	float m_rankScore;
	PercentageStats m_percentageStats;
	LatencyStats m_latencyStats;
	JitterStats m_jitterStats;
	ClientToServerStats m_clientStats;
	NetworkStats m_networkStats;
	RankedClientList m_rankedClients;

    // jitter
    UINT32 m_frameCounter;
};

}