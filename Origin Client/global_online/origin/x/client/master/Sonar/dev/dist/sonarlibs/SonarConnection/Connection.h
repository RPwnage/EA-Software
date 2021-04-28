#pragma once

#include <SonarConnection/BaseConnection.h>
#include <SonarConnection/common.h>
#include <SonarConnection/IConnectionEvents.h>
#include <SonarConnection/Message.h>
#include <SonarCommon/JitterBuffer.h>
#include <SonarCommon/Token.h>
#include <jlib/Thread.h>

#include <memory>
#include <queue>    // std::priority_queue

namespace sonar
{
// This class tracks network conditions for a specific connection.
// The network conditions consists of 3 values:
//   1) packet loss
//   2) jitter
//   3) ping
// 
class NetworkStats
{
public:
    struct Stat
    {
        uint32_t ping;
        uint32_t jitter;
        unsigned long sequence;
    };

    struct Stats
    {
        uint32_t ping;
        uint32_t jitter;
        uint32_t loss;
        uint32_t numberOfSamples;
        uint32_t quality;
        uint32_t pingMin;
        uint32_t pingMax;

        Stats()
            : ping(0)
            , jitter(0)
            , loss(0)
            , numberOfSamples(0)
            , quality(0)
            , pingMin(0)
            , pingMax(0)
        {
        }
    };

    // An element of the priority_queue (i.e., m_rollingWindow)
    class Element
    {
    public:
        Element(const Stat &data, UINT32 priority)
            : m_data(data)
            , m_priority(priority)
        {
        }

        ~Element() {}

        Stat & data() { return m_data; }
        uint32_t priority() const { return m_priority; }

    private:
        Stat m_data;
        uint32_t m_priority;
    };

    // Comparison function to keep lower priority items at the top
    class Comparison
    {
    public:
        bool operator() (const Element& lhs, const Element& rhs) const
        {
            return (lhs.priority() > rhs.priority());
        }
    };

    NetworkStats(int32_t goodLoss, int32_t goodJitter, int32_t goodPing,
                 int32_t okLoss, int32_t okJitter, int32_t okPing,
                 int32_t poorLoss, int32_t poorJitter, int32_t poorPing,
                 int32_t jitterBufferSize);
    ~NetworkStats();

    // Add network condition data point
    void addStat(const Stat& stat);

    // Get the network quality based on the current rolling window
    int getQuality();

    // Get the network stats and quality based on received data
    // overall: if true, use all collected data
    //          else, use data from rolling window
    Stats getStats(bool overall = false);

    void reset();
    void resetWindowedPingStats();

private:
    // Compute a quality metric (0-3) based on packet loss, jitter, and ping.
    int convertToQuality(int32_t loss, int32_t jitter, int32_t ping, int32_t received);

private:
    // values for computing quality
    const int32_t m_goodLoss;
    const int32_t m_goodJitter;
    const int32_t m_goodPing;
    const int32_t m_okLoss;
    const int32_t m_okJitter;
    const int32_t m_okPing;
    const int32_t m_poorLoss;
    const int32_t m_poorJitter;
    const int32_t m_poorPing;
    const int32_t m_jitterBufferSize;

    // total average
    uint32_t m_numberOfSamples;
    uint32_t m_pingTotal;
    uint32_t m_jitterTotal;
    unsigned long m_largestSequence;

    // rolling average
    uint32_t m_rollingPingTotal;
    uint32_t m_rollingJitterTotal;

    // window ping stats
    uint32_t m_pingMin;
    uint32_t m_pingMax;

    // Keep data points (i.e., stored in Element) for computing network conditions for a rolling window
    // Each data point is ordered by the sequence number so that any data points outside of the window (lower sequence number)
    // can be easily removed from the queue.
    typedef std::priority_queue<Element, std::vector<Element>, Comparison> RollingWindow;
    RollingWindow m_rollingWindow;
};

struct JitterBufferOccupancy {
    int32_t total;
    int32_t max;
    uint32_t count;

    JitterBufferOccupancy() {
        reset();
    }

    void reset() {
        total = 0;
        max = 0;
        count = 0;
    }
};

class Connection : public BaseConnection
{
public:

    struct ClientInfo
    {
        String userId;
        String userDesc;
        int id;
    };
    typedef SonarMap<int, ClientInfo> ClientMap;

    Connection(
        const ITimeProvider &time,
        const INetworkProvider &network,
        const IDebugLogger &logger,
        const ConnectionOptions& options,
        int nextPingOffset = 0,
        IStatsProvider* stats = NULL,
        IBlockedListProvider* blockedListProvider = NULL);
    virtual ~Connection(void);

    void setEventSink(IConnectionEvents *eventSink);
    virtual Result connect(CString &connectToken);
    virtual Result disconnect(sonar::CString &reasonType, sonar::CString &reasonDesc);

    bool poll();
    void updateIncomingNetwork();

    const Token &getToken() const;
    const ClientMap &getClients();
    void sendAudioStop(void);
    void sendAudioPayload(const void *payload, size_t cbSize, sonar::INT64 timestamp = 0);

    unsigned char dbgGetOutTakeNumber(void);
    void dbgSetOutTakeNumber(unsigned char number);

    bool isOnBlockedList(CString& userId) const;

    void prepareStatsForTransmission();
    void sendClientStats(unsigned long long timestamp);
    void sendClientStatsTelemetry(unsigned long long timestamp, sonar::String const& channelId);

    int getNetworkQuality();
    NetworkStats::Stats getNetworkStatsOverall();

    /// \brief Returns the IPv4 address of the most-recently connected voice server.
    uint32_t voiceServerAddressIPv4() const;

    const JitterBufferOccupancy& getJitterBufferOccupancyStats() const { return m_jitterBufferOccupancy; }

    IStatsProvider* stats() const { return m_stats; }
    ClientStats& latchedClientStats() { return m_latchedClientStats; }
    ClientStats& latchedClientStatsTelemetry() { return m_latchedClientStatsTelemetry; }

    virtual bool isClientOnlyJitterBufferAware() const {
        return protocol::PROTOCOL_VERSION >= protocol::PROTOCOL_CLIENTONLYJITTERBUFFERAWARE;
    }


protected:
    AbstractTransport *m_transport;
    const INetworkProvider *m_network;
    IBlockedListProvider& m_blockedListProvider;
    IStatsProvider* m_stats;
    ClientStats m_latchedClientStats;
    ClientStats m_latchedClientStatsTelemetry;
    int m_chanSize;

    virtual void handleMessage(Message &message);
    virtual void setDisconnectReason(sonar::String &reasonType, sonar::String &reasonDesc, sonar::CString cmd);

    /// \brief Returns the amount of time connect in milliseconds, 0 if not connected.
    sonar::common::Timestamp connectionTime() const;

private:

    IConnectionEvents *m_eventSink;

    struct
    {
        int number;
        bool stopped;
    } m_inTake;

    struct 
    {
        unsigned char number;
        bool stopped;
    } m_outTake;

    struct 
    {
        long long tsRegisterTimeout;
        long long tsNextFrame;
        long long tsFirstNetworkPing;
        long long tsNextNetworkPing;
        long long tsNextAudioMessageDue;
        long long tsNextJitterBufferOccupancySample;
    } m_timers; 

    String m_connectToken;

    struct
    {
        ClientMap clients;
    } m_channel;

    Token m_token;

    protocol::SEQUENCE m_AudioFirstIncomingSeq;     // sequence number of the first received audio message (used to calculate the number of missing audio packets)
    protocol::SEQUENCE m_AudioLastOutgoingSeq;      // sequence number of the most recently sent audio message
    protocol::SEQUENCE m_AudioLargestIncomingSeq;   // sequence number of the most recently received audio message
    long long m_AudioLastUploadTime;

    CriticalSection m_incomingMessagesCS;
    typedef std::deque<Message*> IncomingMessages;
    IncomingMessages m_incomingMessages;
    uint32_t m_localClientFrameCounter; // monotonously incrementing counter of audio frames

    // Jitter buffer stats
    common::Timestamp m_startConnectTime;
    common::Timestamp m_localBaseTimestamp;
    int32_t m_lastNormalizedLocalArrivalTime;
    int32_t m_lastNormalizedLocalPlaybackTime;
    uint32_t m_numberOfMeasurements;
    int64_t m_arrivalTotal;
    int64_t m_playbackTotal;
    int32_t m_arrivalMax;
    int32_t m_playbackMax;
    struct JitterInfo {
        uint32_t remoteClientFirstFrameCounter;
        uint32_t remoteClientCurrentFrameRemoteCounter;
        int32_t gotPayload;
        uint32_t remoteClientFirstFrameLocalCounterOffset;
        uint32_t mostRecentQueuedFrameCounter;
        static const unsigned int MaxJitterBufferCapacity = 24;
        struct JitterBufferEntryT {
            mutable std::vector<char const> payload;
            long long timestamp;

            JitterBufferEntryT(char const* inPayload = NULL, uint32_t inPayloadSize = 0, long long inTimestamp = 0)
                : payload(inPayload, inPayload + inPayloadSize)
                , timestamp(inTimestamp)
            {
            }

            JitterBufferEntryT(JitterBufferEntryT const& from)
                : payload(from.payload)
                , timestamp(from.timestamp)
            {
            }

            JitterBufferEntryT& operator=(JitterBufferEntryT const& from) {
                payload = from.payload;
                timestamp = from.timestamp;
                return *this;
            }
    };
        typedef JitterBuffer<JitterBufferEntryT, MaxJitterBufferCapacity> JitterBufferT;
        JitterBufferT jitterBuffer;

        JitterInfo()
            : remoteClientFirstFrameCounter(0)
            , remoteClientCurrentFrameRemoteCounter(0)
            , gotPayload(false)
            , remoteClientFirstFrameLocalCounterOffset(0)
            , jitterBuffer()
        {
        }

        JitterInfo& operator=(JitterInfo const& from) {
            if (&from == this)
                return *this;

            remoteClientFirstFrameCounter = from.remoteClientFirstFrameCounter;
            remoteClientCurrentFrameRemoteCounter = from.remoteClientCurrentFrameRemoteCounter;
            gotPayload = from.gotPayload;
            // jitterBuffer = from.jitterBuffer;
            remoteClientFirstFrameLocalCounterOffset = from.remoteClientFirstFrameLocalCounterOffset;
            return *this;
        }
    };

    std::vector<JitterInfo> m_jitterBase;

    JitterBufferOccupancy m_jitterBufferOccupancy;

    // Outgoing audio packet frame counter
    UINT32 m_AudioFrameCounter;
    int m_registerRetries;

    // network ping
    protocol::SEQUENCE m_networkPingSequence;
    common::Timestamp m_networkPingLastSendTime;
    common::Timestamp m_networkPingLastRecvTime;
    NetworkStats m_networkStats;

    void sendRegister(void);
    void sendBlockedList();

    ClientInfo parseClientInfo(Message &message);

    void handleRegisterReply(Message &message);
    void handleChannelJoinNotif(Message &message);
    void handleChannelPartNotif(Message &message);
    void handlePeerListFragment(Message &message);
    void handleAudioToClient(Message &message);
    void handleAudioEnd(Message &message);
    void handleChannelInactivity(Message &message);
    void handleNetworkPingReply(Message &message);

    void frame();
    void pingNetwork();
    void updateJitterBufferOccupancy();
    void processDejitteredAudioFrame();

    virtual void socketSendMessage(const Message &message);
    virtual void onDisconnected(sonar::CString &reasonType, sonar::CString &reasonDesc);
    virtual void onConnecting(void);
    virtual void onConnected(void);
    virtual void onMetricEvent(MetricEvent evt, int count = 1);
    virtual bool validatePacket(const Message &message);

    jlib::Thread m_networkReadThread;
    CriticalSection m_networkReadMutex;
    volatile bool m_networkReadTerminateFlag;

    int startNetworkReadThread();
    int stopNetworkReadThread();
    static void* networkReadThreadProc(void* arg);
};

// inlines

inline sonar::common::Timestamp Connection::connectionTime() const {
    return (m_startConnectTime) ? m_time->getMSECTime() - m_startConnectTime : 0;
}

}
