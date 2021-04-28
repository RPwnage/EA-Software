#pragma once
#include <SonarConnection/common.h>
#include <SonarConnection/Protocol.h>
#include <SonarConnection/Queue.h>

#if SONAR_LOGGING_ENABLED > 0
//#define LOGGER_PRINTF(format, ...) 	m_logger->printf("%08lld [%p]: %s " format, m_time->getMSECTime(), this, __FUNCTION__, ##__VA_ARGS__);
#define LOGGER_PRINTF(format, ...) 	m_logger->printf("%08lld\t" format, m_time->getMSECTime(), ##__VA_ARGS__);
#else
#define LOGGER_PRINTF(format, ...) 	{}
#endif

namespace sonar
{

class Message;

struct ConnectionOptions
{
    ConnectionOptions(
        int pingInterval = 20000,
        int tickInterval = 100,
        int jitterBufferSize = 8,
        int registerTimeout = protocol::REGISTER_TIMEOUT,
        int registerMaxRetries = protocol::REGISTER_MAX_RETRIES,
        int networkGoodLoss = protocol::NETWORK_GOOD_LOSS,
        int networkGoodJitter = protocol::NETWORK_GOOD_JITTER,
        int networkGoodPing = protocol::NETWORK_GOOD_PING,
        int networkOkLoss = protocol::NETWORK_OK_LOSS,
        int networkOkJitter = protocol::NETWORK_OK_JITTER,
        int networkOkPing = protocol::NETWORK_OK_PING,
        int networkPoorLoss = protocol::NETWORK_POOR_LOSS,
        int networkPoorJitter = protocol::NETWORK_POOR_JITTER,
        int networkPoorPing = protocol::NETWORK_POOR_PING,
        int networkPingStartupDuration = 3000,
        int networkQualityPingShortInterval = 200,
        int networkQualityPingLongInterval = 2000)
        : clientPingInterval(pingInterval)
        , clientTickInterval(tickInterval)
        , clientJitterBufferSize(jitterBufferSize)
        , clientRegisterTimeout(registerTimeout)
        , clientRegisterMaxRetries(registerMaxRetries)
        , clientNetworkGoodLoss(networkGoodLoss)
        , clientNetworkGoodJitter(networkGoodJitter)
        , clientNetworkGoodPing(networkGoodPing)
        , clientNetworkOkLoss(networkOkLoss)
        , clientNetworkOkJitter(networkOkJitter)
        , clientNetworkOkPing(networkOkPing)
        , clientNetworkPoorLoss(networkPoorLoss)
        , clientNetworkPoorJitter(networkPoorJitter)
        , clientNetworkPoorPing(networkPoorPing)
        , clientNetworkPingStartupDuration(networkPingStartupDuration)
        , clientNetworkQualityPingShortInterval(networkQualityPingShortInterval)
        , clientNetworkQualityPingLongInterval(networkQualityPingLongInterval)
    {
    }

    int clientPingInterval;
    int clientTickInterval;
    int clientJitterBufferSize;
    int clientRegisterTimeout;
    int clientRegisterMaxRetries;
    int clientNetworkGoodLoss;
    int clientNetworkGoodJitter;
    int clientNetworkGoodPing;
    int clientNetworkOkLoss;
    int clientNetworkOkJitter;
    int clientNetworkOkPing;
    int clientNetworkPoorLoss;
    int clientNetworkPoorJitter;
    int clientNetworkPoorPing;
    int clientNetworkPingStartupDuration;
    int clientNetworkQualityPingShortInterval;
    int clientNetworkQualityPingLongInterval;
};

extern const ConnectionOptions DefaultConnectionOptions;

class BaseConnection
{
public:

    enum Result
    {
        Success,
        InvalidToken,
        InvalidState,
    };

    enum UpdateSourceOption 
    {
        UPDATE_SOURCE,
        DONT_UPDATE_SOURCE
    };

    BaseConnection(
        const ITimeProvider &time,
        const IDebugLogger &logger,
        const ConnectionOptions& options,
        int nextPingOffset);
    virtual ~BaseConnection(void);
    bool isConnected() const;
    bool isConnecting() const;
    bool isDisconnected() const;
    void sendMessage(Message &msg, UpdateSourceOption option = UPDATE_SOURCE);
    virtual Result connect(CString &connectToken) { return Success; }
    virtual Result disconnect(sonar::CString &reasonType, sonar::CString &reasonDesc);
    void postMessage(Message &message);
    CString &getUserId() const;
    CString &getUserDesc() const;
    int getCID() const;
    int getCHID() const;
    void setCHID(int chid);
    size_t getTxQueueLength() const;

    void setLocalAddress(sockaddr_in addr) { m_addr_local = addr; }
    void setRemoteAddress(sockaddr_in addr) { m_addr_remote = addr; }
    const sockaddr_in &getLocalAddress() const { return m_addr_local; }
    const sockaddr_in &getRemoteAddress() const { return m_addr_remote; }

    CString cmdToString(UINT8 cmd);

    /// Returns true if this connection is aware of jitter-buffers restricted to the client
    virtual bool isClientOnlyJitterBufferAware() const = 0;

private:
    void sendUnregister();

    virtual void socketSendMessage(const Message &message) = 0;

    void processRelMessage(Message &message);
    void processUnrelMessage(Message &message);
    void processAck(Message &message);

    protocol::SEQUENCE random(protocol::SEQUENCE unwanted);
    void sendAck(UINT8 commandBeingAcked);
    void sendUnreliableMessage (Message &message, UpdateSourceOption option);
    void sendReliableMessage (const Message &message);
    void handleUnregister(Message &message);
    void handlePing(Message &message);
    void sendTxQueue();
    void popTxQueue();

    virtual void onDisconnected(sonar::CString &reasonType, sonar::CString &reasonDesc) = 0;
    virtual void onConnecting(void) = 0;
    virtual void onConnected(void) = 0;
    virtual bool validatePacket(const Message &message) = 0;
    virtual void setDisconnectReason(sonar::String &reasonType, sonar::String &reasonDesc, sonar::CString cmd);

protected:
    enum State
    {
        None,
        Connecting,
        Connected,
        Disconnected,
    };

    enum MetricEvent
    {
        DropBadState,
        DropBadHeader,
        DropBadCid,
        DropBadSeq,
        DropUnexpectedMsg,
        DropUnexpectedSeq,
        DropBadPayload,
        DropPayloadSize,
        DropOverflow,
        DropLatePacket,
        DropForced,
        DropEmptyPayload,
        DropStopFrame,
        DropTalkBalance,
        DropTruncatedMessage,

        AckUnexpectedSeq,
        Resend,
        ReadPacket,
        SendPacket,
        SendReliable,

        AudioMessageOutOfSequence,
        DropConnection,
        RecvError,
        SendError,

        // client-only metrics
        LostAudioPacket,
        ReadAudioPacket,
        ReadAudioPacketSize,
        SentAudioPacket,
        SentAudioPacketSize,
        SocketRecvError,
        SocketSendError,
        AudioMixClipping,

        // failure points during handleAudioToClient() call
        DropNotConnected,
        DropReadServerFrameCounter,
        DropReadTakeNumber,
        DropTakeStopped,
        DropReadCid,
        DropNullPayload,
        DropReadClientFrameCounter,

        JitterBufferUnderrun,
        JitterBufferDepth,

        DropLatePacketNotStarted,
        MaxAudioLoss
    };

    void dbgForceState(State state);

    void frame();
    void setCID(int cid);
    void setUserId(CString &userId);
    void setUserDesc(CString &userDesc);
    virtual void onMetricEvent(MetricEvent evt, int count = 1) = 0;

    virtual void handleMessage (Message &message) = 0;
    State getState();
    bool changeState(State newState);

    const ConnectionOptions& options() { return m_options; }

private:

    ConnectionOptions m_options;

    struct 
    {
        long long nextPing;
    } m_timers; 

    int m_cid;
    int m_chid;
    int m_nextPingOffset;

    typedef Queue<Message *, 512> MessageQueue;

    sockaddr_in m_addr_local;
    sockaddr_in m_addr_remote;

protected:
    struct
    {
        protocol::SEQUENCE nextSeq;
        MessageQueue queue;
        long long nextResend;
        int resendCount;
    } m_tx;

private:
    Message *m_currRelMessage;

    State m_state;

    String m_userId;
    String m_userDesc;


protected:
    const IDebugLogger *m_logger;
    const ITimeProvider *m_time;
    String m_reasonType;
    String m_reasonDesc;
    bool m_remoteClosed;

    struct
    {
        protocol::SEQUENCE prevAck;
        protocol::SEQUENCE nextAck;
    } m_rx;
};

// inline members

inline bool BaseConnection::isConnected() const
{
    return m_state == Connected;
}

inline bool BaseConnection::isConnecting() const
{
    return m_state == Connecting;
}

inline bool BaseConnection::isDisconnected() const
{
    return m_state == Disconnected;
}

}
