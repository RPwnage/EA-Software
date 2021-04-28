#define NOMINMAX

#include <SonarConnection/Connection.h>
#include <SonarConnection/Message.h>
#include <SonarConnection/Protocol.h>
#include <SonarCommon/Common.h>
#include <time.h>
#include <assert.h>
#include <stdlib.h>
#include <iomanip>
#include <iostream>
#include <algorithm>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <memory>

namespace {
    /// Size of network quality ping rolling window
    const int NETWORK_QUALITY_PING_ROLLING_WINDOW_SIZE = 5;
}

namespace sonar
{

class EmptyBlockedListProvider : public sonar::IBlockedListProvider
{
public:

    EmptyBlockedListProvider()
    {
    }

    const sonar::BlockedList& blockList() const override
    {
        return mEmptyBlockList;
    }

    bool hasChanges() const
    {
        return false;
    }

    void clearChanges()
    {
    }

    const sonar::BlockedList mEmptyBlockList;
};

EmptyBlockedListProvider sEmptyBlockedListProvider;

Connection::Connection(
    const ITimeProvider &time,
    const INetworkProvider &network,
    const IDebugLogger &logger,
    const ConnectionOptions& options,
    int nextPingOffset,
    IStatsProvider* stats,
    IBlockedListProvider* blockedListProvider)

	: BaseConnection(time, logger, options, nextPingOffset)
	, m_transport(NULL)
	, m_network(&network)
	, m_blockedListProvider(blockedListProvider ? *blockedListProvider : sEmptyBlockedListProvider)
	, m_stats(stats)
    , m_latchedClientStats()
    , m_latchedClientStatsTelemetry()
    , m_chanSize(0)
	, m_eventSink(NULL)
	, m_AudioFirstIncomingSeq(0)
	, m_AudioLastOutgoingSeq(0)
	, m_AudioLargestIncomingSeq(0)
#if defined(_DEBUG)
	, m_AudioLastUploadTime(0)
#endif
    , m_startConnectTime(0)
    , m_localBaseTimestamp(0)
    , m_lastNormalizedLocalArrivalTime(0)
    , m_lastNormalizedLocalPlaybackTime(0)
    , m_numberOfMeasurements(0)
    , m_arrivalTotal(0)
    , m_playbackTotal(0)
    , m_arrivalMax(0)
    , m_playbackMax(0)
    , m_jitterBase(protocol::CHANNEL_MAX_CLIENTS)
    , m_AudioFrameCounter(1)
    , m_registerRetries(0)
    , m_networkPingSequence(0)
    , m_networkPingLastSendTime(0)
    , m_networkPingLastRecvTime(0)
    , m_networkStats(options.clientNetworkGoodLoss, options.clientNetworkGoodJitter, options.clientNetworkGoodPing,
                     options.clientNetworkOkLoss, options.clientNetworkOkJitter, options.clientNetworkOkPing,
                     options.clientNetworkPoorLoss, options.clientNetworkPoorJitter, options.clientNetworkPoorPing,
                     options.clientJitterBufferSize)
    , m_networkReadThread()
    , m_networkReadTerminateFlag(false)
{
	srand( (unsigned int) ::time(0) + (unsigned int) time.getMSECTime());
	m_timers.tsNextFrame = 0;
    m_timers.tsRegisterTimeout = 0;
    m_timers.tsNextNetworkPing = -1;
    m_timers.tsFirstNetworkPing = -1;
    m_timers.tsNextAudioMessageDue = 0;
    m_timers.tsNextJitterBufferOccupancySample = -1;
    m_localClientFrameCounter = 0;
}

Connection::~Connection(void)
{
    CriticalSection::Locker lock(m_networkReadMutex);
	delete m_transport;
    m_transport = NULL;
}

void Connection::setEventSink(IConnectionEvents *eventSink)
{
	m_eventSink = eventSink;
}

void Connection::sendRegister(void)
{
	Message msg(protocol::MSG_REGISTER, 0, false);

	msg.writeUnsafeLong(protocol::PROTOCOL_VERSION);
	msg.writeUnsafeLong(m_rx.nextAck);

	if (m_connectToken.size() > (msg.getBytesLeft() - 1))
	{
		return;
	}

	msg.writeUnsafeBigString(m_connectToken);

	m_timers.tsRegisterTimeout = m_time->getMSECTime() + options().clientRegisterTimeout;
    m_chanSize = 0;

    // special hook for QA to force drop MSG_REGISTER messages
    if( (internal::sonarTestingRegisterPacketLossPercentage > 0) && ((rand () % 100) + 1) < internal::sonarTestingRegisterPacketLossPercentage )
        return;

	sendMessage(msg, DONT_UPDATE_SOURCE);
}

bool isErasable(std::string& i)
{
    return i.empty();
}

void Connection::sendBlockedList()
{
    assert(getCID() != -1);

    // get a copy of the blocked list and cleanse it of bad slots
    BlockedList blocked = m_blockedListProvider.blockList();
    blocked.erase(std::remove_if(blocked.begin(), blocked.end(), isErasable), blocked.end());

    // we need to limit the number of blocked ids sent in a packet
    unsigned int MAX_IDS_IN_PACKET = 40;
    unsigned int roomInPacket = 0;
    unsigned int numIdsToSend;
    unsigned int remainingIds = blocked.size();

    Message msg;
    for (auto i(blocked.cbegin()), last(blocked.cend()); i != last; ++i)
    {
        if (roomInPacket == 0)
        {
            msg = Message(protocol::MSG_BLOCK_LIST_FRAGMENT, getCID(), true);
            numIdsToSend = std::min(remainingIds, MAX_IDS_IN_PACKET);
            msg.writeUnsafeLong(numIdsToSend);
            roomInPacket = numIdsToSend;
        }

        msg.writeUnsafeSmallString(*i);
        --roomInPacket;

        if (roomInPacket == 0)
        {
            if (ENABLE_VOIP_BLOCKING_TRACE)
                LOGGER_PRINTF("sending blocked list fragment - %d ids)\n", numIdsToSend);
            sendMessage(msg);
            remainingIds -= numIdsToSend;
        }
    }

    if (ENABLE_VOIP_BLOCKING_TRACE)
        LOGGER_PRINTF("sending blocked list complete)\n");

    Message blockMsg = Message(protocol::MSG_BLOCK_LIST_COMPLETE, getCID(), true);
    sendMessage(blockMsg);
}

void Connection::sendClientStats(unsigned long long timestamp)
{
    NetworkStats::Stats stats = m_networkStats.getStats();

    int deltaJitterMeasurements = (m_stats->jitterNumMeasurements() - m_latchedClientStats.jitterNumMeasurements());
    int jitterArrivalMean = (deltaJitterMeasurements) ? (m_stats->jitterArrivalMeanCumulative() - m_latchedClientStats.jitterArrivalMeanCumulative()) / deltaJitterMeasurements : 0;
    int jitterPlaybackMean = (deltaJitterMeasurements) ? (m_stats->jitterPlaybackMeanCumulative() - m_latchedClientStats.jitterPlaybackMeanCumulative()) / deltaJitterMeasurements : 0;

    if (common::ENABLE_CONNECTION_STATS)
    {
        std::stringstream logMsg;
            logMsg
                << std::setw(10) << timestamp << ':'
                << std::setw(1)
                << m_stats->audioMessagesSent() - m_latchedClientStats.audioMessagesSent() << ':'
                << m_stats->audioMessagesReceived() - m_latchedClientStats.audioMessagesReceived() << ':'
                << m_stats->audioMessagesOutOfSequence() - m_latchedClientStats.audioMessagesOutOfSequence() << ':'
                << m_stats->audioMessagesLost() - m_latchedClientStats.audioMessagesLost() << ':'
                << "(" << m_AudioLargestIncomingSeq << "-" << m_AudioFirstIncomingSeq << "-" << (m_stats->audioMessagesReceived() - m_latchedClientStats.audioMessagesReceived()) << ")"
                << m_stats->badCid() - m_latchedClientStats.badCid() << ':'
                << m_stats->truncatedMessages() - m_latchedClientStats.truncatedMessages() << ':'
                << m_stats->audioMixClipping() - m_latchedClientStats.audioMixClipping() << ':'
                << m_stats->socketSendError() - m_latchedClientStats.socketSendError() << ':'
                << m_stats->socketRecvError() - m_latchedClientStats.socketRecvError() << ':'
                << m_stats->deltaPlaybackMean() << ':'
                << m_stats->deltaPlaybackMax() << ':'
                << m_stats->receiveToPlaybackMean() << ':'
                << m_stats->receiveToPlaybackMax() << ':'
                << jitterArrivalMean << ':'
                << m_stats->jitterArrivalMax() << ':'
                << jitterPlaybackMean << ':'
                << m_stats->jitterPlaybackMax() << ':'
                << m_stats->playbackUnderrun() - m_latchedClientStats.playbackUnderrun() << ':'
                << m_stats->playbackOverflow() - m_latchedClientStats.playbackOverflow() << ':'
                << m_stats->playbackDeviceLost() - m_latchedClientStats.playbackDeviceLost() << ':'
                << m_stats->dropNotConnected() - m_latchedClientStats.dropNotConnected() << ':'
                << m_stats->dropReadServerFrameCounter() - m_latchedClientStats.dropReadServerFrameCounter() << ':'
                << m_stats->dropReadTakeNumber() - m_latchedClientStats.dropReadTakeNumber() << ':'
                << m_stats->dropTakeStopped() - m_latchedClientStats.dropTakeStopped() << ':'
                << m_stats->dropReadCid() - m_latchedClientStats.dropReadCid() << ':'
                << m_stats->dropNullPayload() - m_latchedClientStats.dropNullPayload() << ':'
                << m_stats->dropReadClientFrameCounter() - m_latchedClientStats.dropReadClientFrameCounter() << ':'
                << m_stats->jitterbufferUnderrun() - m_latchedClientStats.jitterbufferUnderrun() << ':'
                << stats.loss << ':'
                << stats.jitter << ':'
                << stats.ping << ':'
                << stats.numberOfSamples << ':'
                << stats.quality;
            common::Log(logMsg.str().c_str());
    }

    Message msg(protocol::MSG_CLIENT_STATS, getCID(), false);

    msg.writeUnsafeShort(sonar::CLIENT_STATS_COUNT); // number of stats - used for verification on the server
    msg.writeUnsafeLong(m_stats->audioMessagesSent() - m_latchedClientStats.audioMessagesSent());
    msg.writeUnsafeLong(m_stats->audioMessagesReceived() - m_latchedClientStats.audioMessagesReceived());
    msg.writeUnsafeLong(m_stats->audioMessagesOutOfSequence() - m_latchedClientStats.audioMessagesOutOfSequence());
    msg.writeUnsafeLong(m_stats->audioMessagesLost() - m_latchedClientStats.audioMessagesLost());
    msg.writeUnsafeLong(m_stats->badCid() - m_latchedClientStats.badCid());
    msg.writeUnsafeLong(m_stats->truncatedMessages() - m_latchedClientStats.truncatedMessages());
    msg.writeUnsafeLong(m_stats->audioMixClipping() - m_latchedClientStats.audioMixClipping());
    msg.writeUnsafeLong(m_stats->socketRecvError() - m_latchedClientStats.socketRecvError());
    msg.writeUnsafeLong(m_stats->socketSendError() - m_latchedClientStats.socketSendError());
    msg.writeUnsafeLong(m_stats->deltaPlaybackMean());
    msg.writeUnsafeLong(m_stats->deltaPlaybackMax());
    msg.writeUnsafeLong(m_stats->receiveToPlaybackMean());
    msg.writeUnsafeLong(m_stats->receiveToPlaybackMax());
    msg.writeUnsafeLong(jitterArrivalMean);
    msg.writeUnsafeLong(m_stats->jitterArrivalMax());
    msg.writeUnsafeLong(jitterPlaybackMean);
    msg.writeUnsafeLong(m_stats->jitterPlaybackMax());

    // NOTE: The following stats are sent to Telemetry but not to zabbix via the voiceServer.
    //       There is no immediate good reason (to counter the added payload) to send these.
    // playbackUnderrun
    // playbackOverflow
    // playbackDeviceLost
    // dropNotConnected
    // dropReadServerFrameCounter
    // dropReadTakeNumber
    // dropTakeStopped
    // dropReadCid
    // dropNullPayload
    // dropReadClientFrameCounter
    // jitterbufferUnderrun

    // network stats
    msg.writeUnsafeLong(stats.loss);
    msg.writeUnsafeLong(stats.jitter);
    msg.writeUnsafeLong(stats.ping);
    msg.writeUnsafeLong(stats.numberOfSamples);
    msg.writeUnsafeLong(stats.quality);

    sendMessage(msg);

    // reset stats after we've finished sending them
    m_AudioFirstIncomingSeq = m_AudioLargestIncomingSeq;

    // latch the stats
    m_latchedClientStats = *m_stats;
}

// TBD: move these CPU timing functions to platform services
static float CalculateCPULoad(uint64_t idleTicks, uint64_t totalTicks)
{
    static uint64_t _previousTotalTicks = 0;
    static uint64_t _previousIdleTicks = 0;

    uint64_t totalTicksSinceLastTime = totalTicks - _previousTotalTicks;
    uint64_t idleTicksSinceLastTime = idleTicks - _previousIdleTicks;

    float ret = 1.0f - ((totalTicksSinceLastTime > 0) ? ((float)idleTicksSinceLastTime) / totalTicksSinceLastTime : 0);

    _previousTotalTicks = totalTicks;
    _previousIdleTicks = idleTicks;

    return ret;
}

inline uint64_t FileTimeToInt64(const FILETIME & ft) {
    return (((uint64_t)(ft.dwHighDateTime)) << 32) | ((uint64_t)ft.dwLowDateTime);
}

// Returns 1.0f for "CPU fully pinned", 0.0f for "CPU idle", or somewhere in between
// You'll need to call this at regular intervals, since it measures the load between
// the previous call and the current one.  Returns -1.0 on error.
float GetCPULoad()
{
    FILETIME idleTime, kernelTime, userTime;
    if (!GetSystemTimes(&idleTime, &kernelTime, &userTime))
        return -1.0f;
    
    return CalculateCPULoad(FileTimeToInt64(idleTime), FileTimeToInt64(kernelTime) + FileTimeToInt64(userTime));
}

void Connection::sendClientStatsTelemetry(unsigned long long timestamp, sonar::String const& channelId)
{
    NetworkStats::Stats stats = m_networkStats.getStats();
    float cpuLoad = GetCPULoad();

    common::Telemetry(
        sonar::SONAR_TELEMETRY_CLIENT_STATS,
        channelId.c_str(),

        // Avg, max, min latency(RTT)
        stats.pingMin,
        stats.pingMax,
        stats.ping,
        // bandwidth(bytes received and bytes sent)
        m_stats->audioBytesSent() - m_latchedClientStats.audioBytesSent(),
        m_stats->audioBytesReceived() - m_latchedClientStats.audioBytesReceived(),
        // packet loss
        m_stats->audioMessagesLost() - m_latchedClientStats.audioMessagesLost(),
        // Out of order messages received
        m_stats->audioMessagesOutOfSequence() - m_latchedClientStats.audioMessagesOutOfSequence(),
        // jitter(between successively received packets)
        0, //m_stats->jitterArrivalMin(),
        m_stats->jitterArrivalMax(),
        m_stats->jitterArrivalMean(),
        // cpu usage
        static_cast<uint32_t>(cpuLoad * 100.0f)
        );

    // update the latched client stats after sending them
    m_latchedClientStatsTelemetry = *m_stats;
}

int Connection::getNetworkQuality()
{
    return m_networkStats.getQuality();
}

NetworkStats::Stats Connection::getNetworkStatsOverall()
{
    return m_networkStats.getStats(true);
}

bool Connection::isOnBlockedList(CString& userId) const
{
    const BlockedList blocked = m_blockedListProvider.blockList();
    return std::find(blocked.cbegin(), blocked.cend(), userId) != blocked.cend();
}


Connection::Result Connection::connect(CString &connectToken)
{
    if (ENABLE_CONNECTION_TRACE)
        LOGGER_PRINTF("connecting\n");

	m_connectToken = connectToken;
	m_AudioFirstIncomingSeq = 0;
	m_AudioLargestIncomingSeq = 0;
	m_AudioLastOutgoingSeq = 0;
	m_AudioLastUploadTime = 0;
    m_AudioFrameCounter = 1;
    m_registerRetries = 0;

	if (!m_token.parse(connectToken))
	{
		common::Telemetry(sonar::SONAR_TELEMETRY_ERROR, "SConnection", "connect_invalid_token", 0);
		return InvalidToken;
	}
	
	if (!changeState(Connecting))
	{
		return InvalidState;
	}

    m_timers.tsNextAudioMessageDue = 0;

    // reset the connection start baseline
    m_startConnectTime = m_time->getMSECTime();

	return Success;
}

Connection::Result Connection::disconnect(sonar::CString &reasonType, sonar::CString &reasonDesc)
{
    if (ENABLE_CONNECTION_TRACE) {
        LOGGER_PRINTF("disconnecting\n");
    }

    BaseConnection::Result result = BaseConnection::disconnect(reasonType, reasonDesc);

    if (result == BaseConnection::Success)
    {
        if (m_transport)
        {
            m_transport->close();
            delete m_transport;
            m_transport = NULL;
            LOGGER_PRINTF("closed transport, cid=%d\n", getCID());
        }
        m_startConnectTime = 0;
    }

    return result;
}

const Connection::ClientMap &Connection::getClients()
{
	return m_channel.clients;
}

void Connection::handleMessage(Message &message)
{
    if (ENABLE_CONNECTION_TRACE) {
        LOGGER_PRINTF("Got message %d with sequence %04x\n",
            message.getHeader().cmd, message.getSequence());
    }

	switch (message.getHeader().cmd)
	{
	case protocol::MSG_REGISTERREPLY:
		handleRegisterReply(message);
		break;

	case protocol::MSG_PEERLIST_FRAGMENT:
		handlePeerListFragment(message);
		break;

	case protocol::MSG_CHANNEL_JOINNOTIF:
		handleChannelJoinNotif(message);
		break;

	case protocol::MSG_CHANNEL_PARTNOTIF:
		handleChannelPartNotif(message);
		break;

	case protocol::MSG_AUDIO_STOP:
        // nothing to do (other than ACK the message)
		break;

	case protocol::MSG_AUDIO_TO_CLIENT:
		handleAudioToClient(message);
		break;
	
	case protocol::MSG_ECHO:
		if (m_eventSink) m_eventSink->onEchoMessage(message);
		break;

    case protocol::MSG_CHANNEL_INACTIVITY:
        handleChannelInactivity(message);
        break;

    case protocol::MSG_AUDIO_END:
        handleAudioEnd(message);
        break;

    case protocol::MSG_NETWORK_PING_REPLY:
        handleNetworkPingReply(message);
        break;
    
    default:
		BaseConnection::handleMessage(message);
		break;
    }
}

void Connection::setDisconnectReason(sonar::String &reasonType, sonar::String &reasonDesc, sonar::CString cmd)
{
    reasonType = "REMOTE_CONNECTION_LOST";
    reasonDesc = cmd;
}

void Connection::prepareStatsForTransmission()
{
    int lost = std::max(static_cast<int>(m_AudioLargestIncomingSeq) - static_cast<int>(m_AudioFirstIncomingSeq) - static_cast<int>(m_stats->audioMessagesReceived()), 0);
    onMetricEvent(BaseConnection::LostAudioPacket, lost);

    int32_t jitterArrivalMean = 0;
    int32_t jitterArrivalMax = 0;
    int32_t jitterPlaybackMean = 0;
    int32_t jitterPlaybackMax = 0;

    if (m_numberOfMeasurements > 0) {
        jitterArrivalMean = m_arrivalTotal / m_numberOfMeasurements;
        jitterArrivalMax = m_arrivalMax;
        jitterPlaybackMean = m_playbackTotal / m_numberOfMeasurements;
        jitterPlaybackMax = m_playbackMax;

        m_arrivalTotal = 0;
        m_playbackTotal = 0;
        m_arrivalMax = 0;
        m_playbackMax = 0;
        m_numberOfMeasurements = 0;
    }

    m_stats->jitterArrivalMean() = jitterArrivalMean;
    m_stats->jitterArrivalMeanCumulative() += jitterArrivalMean;
    m_stats->jitterArrivalMax() = jitterArrivalMax;
    m_stats->jitterArrivalMaxCumulative() = std::max(m_stats->jitterArrivalMaxCumulative(), jitterArrivalMax);
    m_stats->jitterPlaybackMean() = jitterPlaybackMean;
    m_stats->jitterPlaybackMeanCumulative() += jitterPlaybackMean;
    m_stats->jitterPlaybackMax() = jitterPlaybackMax;
    m_stats->jitterPlaybackMaxCumulative() = std::max(m_stats->jitterPlaybackMaxCumulative(), jitterPlaybackMax);
    ++m_stats->jitterNumMeasurements();
}

bool Connection::poll()
{
    if (common::ENABLE_PAYLOAD_TRACING) {
        LOGGER_PRINTF("connection::poll\n");
    }

    frame();
    pingNetwork();

    if (getCID() > 0 && m_blockedListProvider.hasChanges())
    {
        sendBlockedList();
        m_blockedListProvider.clearChanges();
    }

    bool isAudioEnd = false;

    // process all received messages
    while (!m_incomingMessages.empty()) {
        Message* message = NULL;
        {
            CriticalSection::Locker lock(m_incomingMessagesCS);
            message = m_incomingMessages.front();
            if (!message)
                continue;

            m_incomingMessages.pop_front();
            }

            protocol::MessageHeader& header = message->getHeader(); (void) header;
        isAudioEnd = (header.cmd == protocol::MSG_AUDIO_END);

        postMessage(*message);
        delete message;
            }

    // prepare an audio frame
    processDejitteredAudioFrame();

    size_t maxJitterBufferDepth = 0;
    for (auto chid = 0; chid < protocol::CHANNEL_MAX_CLIENTS; ++chid) {
        maxJitterBufferDepth = std::max(maxJitterBufferDepth, m_jitterBase[chid].jitterBuffer.size());
            }

    onMetricEvent(BaseConnection::JitterBufferDepth, maxJitterBufferDepth);

    updateJitterBufferOccupancy();

    return true;
}

int Connection::startNetworkReadThread()
{
    // verify that we don't already have a read thread
    assert(!m_networkReadThread.isValid());

    if (!m_networkReadThread.isValid()) {
        m_networkReadThread = jlib::Thread::createThread(Connection::networkReadThreadProc, this);

        // handle thread creation failure
        if (!m_networkReadThread.isValid()) {
            m_reasonType  = "NETWORK_READ_THREAD_CREATION_FAILED";
            changeState(Disconnected);
            delete m_transport;
            m_transport = NULL;
            return -1;
        }
        m_networkReadTerminateFlag = false;
    }

    return 0;
}

int Connection::stopNetworkReadThread()
{
    // This is sooo ugly. I hope that someone smarter than me can clean it up.
    // The problem: the networkReadThreadProc just sits on a blocking read.
    // On Linux (and Unix?), closing the transport socket doesn't terminate the
    // blocking read and the proper way to terminate it is to cancel the pthread.
    // On Windows, terminating the thread doesn't clean up the CriticalSection
    // that guards the read so we have to close the socket to get the blocking
    // read to complete and exit the thread normally (by setting the terminate
    // flag).

#if __linux
    // Cancel the thread to cause the blocking read to complete.
    if (m_networkReadThread.isValid()) {
        m_networkReadThread.cancel(); // this should kill the blocking read
        m_networkReadThread.join();
        m_networkReadThread = jlib::Thread();
    }
#endif

    if (m_transport)
    {
        // mark the thread as terminable and close the socket
        m_networkReadTerminateFlag = true;
        m_transport->close();
        CriticalSection::Locker lock(m_networkReadMutex);
        delete m_transport;
        m_transport = NULL;
    }

#ifdef _WIN32
    // Close the socket handle on Windows
    m_networkReadThread = jlib::Thread();
#endif

    return 0;
}

void* Connection::networkReadThreadProc(void* arg)
{
    Connection* this_ = reinterpret_cast<Connection*>(arg);
    while (!this_->m_networkReadTerminateFlag) {
        this_->updateIncomingNetwork();
    }
    return NULL;
}

void Connection::updateIncomingNetwork()
{
    if (m_transport == NULL || m_transport->isClosed())
        return;

    CriticalSection::Locker lock(m_networkReadMutex);

    std::unique_ptr<Message> current(new Message());
    size_t cbSize = current->getMaxSize();
    ssize_t socketResult;
    bool result = m_transport->rxMessagePoll(current->getBufferPtr(), &cbSize, socketResult);
    if (!result)
    {
        if (socketResult < 0)
            onMetricEvent(BaseConnection::SocketRecvError);
        return;
    }

    if (cbSize == 0)
    {
        return;
    }

    current->setTimestamp(m_time->getMSECTime());
    struct sockaddr_in dummy;
    current->update(cbSize, dummy);

    std::unique_ptr<Message> audioEnd;

    protocol::MessageHeader& header = current->getHeader();

    // check to see if we need to queue an audio end marker
    // the MSG_AUDIO_STOP message goes into the control message queue so that it's ACK is prioritized
    // the MSG_AUDIO_END message get added to the audio message queue as a take terminator
    if (header.cmd == protocol::MSG_AUDIO_STOP) {
        // create a MSG_AUDIO_END
        audioEnd.reset(new Message(*current));
        protocol::MessageHeader& audioEndHeader = audioEnd->getHeader();
        audioEndHeader.cmd = protocol::MSG_AUDIO_END;
        audioEndHeader.source = header.source;
        audioEndHeader.flags = ( ((int) protocol::PacketType::PT_SEGUNREL) & protocol::PACKETTYPE_MASK) << protocol::PACKETTYPE_SHIFT;

        {
            CriticalSection::Locker lock(m_incomingMessagesCS);
            m_incomingMessages.push_back(audioEnd.release());
    }
    }

    {
        CriticalSection::Locker lock(m_incomingMessagesCS);
        m_incomingMessages.push_back(current.release());
    }
}

Connection::ClientInfo Connection::parseClientInfo(Message &message)
{
	ClientInfo ci;

	if (message.getBytesLeft ()  < 3)
	{
		return ci;
	}

    unsigned char byteData;
    if (!message.readSafeByte(byteData))
    {
        LOGGER_PRINTF("Attempting to read %d byte(s). %d byte(s) available.\n", sizeof(byteData), message.getBytesLeft());
        return ci;
    }
    ci.id = (int) byteData;

	ci.userId = message.readSmallString();
	ci.userDesc = message.readSmallString();
	return ci;
}

void Connection::handleRegisterReply(Message &message)
{
	String reasonType = message.readSmallString();
	String reasonDesc = message.readSmallString();

    if (internal::sonarTestingVoiceServerRegisterResponse != "") {
        reasonType = internal::sonarTestingVoiceServerRegisterResponse;
    }
	bool bError = false;

	if (reasonType != "SUCCESS")
	{
		bError = true;
		m_remoteClosed = true;
		disconnect(reasonType, reasonDesc);
		return;
	}

	if (message.getBytesLeft() < (sizeof(protocol::SEQUENCE) + sizeof(unsigned short) + sizeof(unsigned char) + sizeof(unsigned char)))
	{
		return;
	}

	protocol::SEQUENCE nextSeq = message.readUnsafeLong();
	unsigned short cid = message.readUnsafeShort();
	unsigned char chid = message.readUnsafeByte();
	unsigned char chanSize = message.readUnsafeByte();

    if (ENABLE_CONNECTION_TRACE)
        LOGGER_PRINTF("cid=%d chid=%d\n", cid, chid);

    assert (m_chanSize == 0);
	m_chanSize = chanSize;
	setCID(cid);
	setCHID(chid);
	m_tx.nextSeq = nextSeq;
}


void Connection::frame()
{
	if (m_timers.tsNextFrame == -1)
	{
		return;
	}

	common::Timestamp now = m_time->getMSECTime();

	if (now < m_timers.tsNextFrame)
	{
		return;
	}

	m_timers.tsNextFrame = now + options().clientTickInterval;

	State state = getState();

	if (state == Connecting)
	{
		if (now > m_timers.tsRegisterTimeout)
		{
            if (m_registerRetries < options().clientRegisterMaxRetries)
            {
                m_registerRetries++;
                sendRegister();
            }
            else
            {
			    m_reasonType  = "REGISTER_TIMEDOUT";
			    m_reasonDesc  = "";
			    changeState(Disconnected);
            }
		}

		return;
	}

	BaseConnection::frame();
}

void Connection::pingNetwork()
{
    if (m_timers.tsNextNetworkPing == -1)
    {
        return;
    }

    common::Timestamp now = m_time->getMSECTime();

    if (now < m_timers.tsNextNetworkPing)
    {
        return;
    }

    if ( m_timers.tsNextNetworkPing != -1 && now > m_timers.tsNextNetworkPing)
    {
        bool isPastPingStartupDuration = (now - m_timers.tsFirstNetworkPing) > options().clientNetworkPingStartupDuration;
        m_timers.tsNextNetworkPing = now + (isPastPingStartupDuration ? options().clientNetworkQualityPingLongInterval : options().clientNetworkQualityPingShortInterval);
        Message networkPing(protocol::MSG_NETWORK_PING, getCID(), false);
        networkPing.updateSequence(++m_networkPingSequence); // 1-based
        networkPing.writeUnsafeULongLong(now); // send time
        sendMessage(networkPing);
    }
}

void Connection::updateJitterBufferOccupancy()
{
    if (m_timers.tsNextJitterBufferOccupancySample == -1)
    {
        return;
    }

    common::Timestamp now = m_time->getMSECTime();

    if (now < m_timers.tsNextJitterBufferOccupancySample)
    {
        return;
    }

    if ( m_timers.tsNextJitterBufferOccupancySample != -1 && now > m_timers.tsNextJitterBufferOccupancySample)
    {
        m_jitterBufferOccupancy.count++;
        m_jitterBufferOccupancy.total += m_incomingMessages.size();
        m_jitterBufferOccupancy.max = std::max(m_jitterBufferOccupancy.max, (int32_t)m_incomingMessages.size());
        m_timers.tsNextJitterBufferOccupancySample = now + 1000; // once per second
    }
}

void Connection::processDejitteredAudioFrame()
{
    ++m_localClientFrameCounter;

    //if (common::ENABLE_PAYLOAD_TRACING)
    //    LOGGER_PRINTF("process frame#=%d\n", m_localClientFrameCounter);

    long long totalTimestamps = 0;
    int32_t numTimestamps = 0;

    // iterate through the jitter buffer, pulling off audio payloads and process them
    if (m_eventSink)
        m_eventSink->onFrameBegin(common::getTimeAsMSEC());

    for (auto chid = 0; chid < protocol::CHANNEL_MAX_CLIENTS; ++chid) {

        JitterInfo::JitterBufferT& jit = m_jitterBase[chid].jitterBuffer;

        if (jit.isStopped()) {
            //if (common::ENABLE_PAYLOAD_TRACING)
            //    LOGGER_PRINTF("jb stopped, cid=%d\n", cid);
            continue;
        }

        JitterInfo::JitterBufferEntryT entry;
        char const* payload = NULL;
        if (jit.pop(m_localClientFrameCounter, &entry)) {
            // if we got an 'audio end' message, stop the jitter buffer
            bool isAudioEnd = entry.payload.size() == 0;
            if (isAudioEnd) {
                jit.stop();
                m_jitterBase[chid].remoteClientFirstFrameCounter = 0;
                if (common::ENABLE_PAYLOAD_TRACING)
                    LOGGER_PRINTF("audio end, stopping jitter buffer, cid=%d, pri=%d\n", chid, m_localClientFrameCounter);
                continue;
            } else {
                payload = &entry.payload[0];
            }
        } else {
            // if the jitter buffer has drained, stop it
            if (jit.size() == 0) {
                jit.stop();
                m_jitterBase[chid].remoteClientFirstFrameCounter = 0;
                if (common::ENABLE_PAYLOAD_TRACING)
                    LOGGER_PRINTF("jitter buffer drained, stopping, cid=%d, pri=%d\n", chid, m_localClientFrameCounter);
                continue;
            }
        }

        if (common::ENABLE_PAYLOAD_TRACING) {
            LOGGER_PRINTF("%s\t\t\t%d\t%d\t%d\t%d\n",
                entry.payload.size() ? "-" : "m",
                chid,
                entry.payload.size(),
                m_localClientFrameCounter,
                jit.size());
        }

        if (m_eventSink) {
            m_eventSink->onFrameClientPayload(chid, entry.payload.size() ? &entry.payload[0] : NULL, entry.payload.size());
        }

        if (payload) {
            totalTimestamps += entry.timestamp;
            ++numTimestamps;
        }
    }

    if (m_eventSink)
        m_eventSink->onFrameEnd((numTimestamps) ? (totalTimestamps / numTimestamps) : 0);

    //if (common::ENABLE_PAYLOAD_TRACING)
    //    LOGGER_PRINTF("end frame#=%d\n", m_localClientFrameCounter);
}

void Connection::handlePeerListFragment(Message &message)
{
	assert (m_chanSize > 0);

	while (message.getBytesLeft() > 0 && m_chanSize > 0)
	{
		ClientInfo ci = parseClientInfo(message);
		m_channel.clients[ci.id] = ci;
		m_chanSize --;
	}

	if (m_chanSize == 0)
	{
		changeState(Connected);
		m_inTake.number = 0;
		m_inTake.stopped = true;

		m_outTake.number = 0xab;
		m_outTake.stopped = true;

		if (m_eventSink) m_eventSink->onConnect(m_token.getChannelId(), m_token.getChannelDesc());
	}

}

void Connection::handleChannelJoinNotif(Message &message)
{
	ClientInfo ci = parseClientInfo(message);
	m_channel.clients[ci.id] = ci;
	if (m_eventSink) m_eventSink->onClientJoined(ci.id, ci.userId, ci.userDesc);
}

void Connection::handleChannelPartNotif(Message &message)
{
	if (message.getBytesLeft ()  < 1)
	{
		return;
	}

    unsigned char byteData;
    if (!message.readSafeByte(byteData))
    {
        LOGGER_PRINTF("Attempting to read %d byte(s). %d byte(s) available.\n", sizeof(byteData), message.getBytesLeft());
        return;
    }
    int userChid = (int) byteData;

	String reasonType = message.readSmallString();
	String reasonDesc = message.readSmallString();
	
	ClientMap::iterator iter = m_channel.clients.find(userChid);
	assert (iter != m_channel.clients.end());

	if (iter == m_channel.clients.end())
	{
		return;
	}

	if (m_eventSink) m_eventSink->onClientParted(userChid, 
		iter->second.userId, 
		iter->second.userDesc, 
		reasonType,
		reasonDesc);

	m_channel.clients.erase(iter);
}

void Connection::handleAudioToClient(Message &message)
{
	if (getState() != Connected)
	{
        onMetricEvent(BaseConnection::DropNotConnected);
        if (ENABLE_CONNECTION_TRACE)
            LOGGER_PRINTF("not connected, m_cid=%d\n", getCID());
		return;
	}

    // track the first incoming sequence number to allow us to calculate the number of dropped messages
    if (m_AudioFirstIncomingSeq == 0) {
        m_AudioFirstIncomingSeq = message.getSequence();
    }

    protocol::SEQUENCE seq = message.getSequence();
    bool outOfSequence = seq < m_AudioLargestIncomingSeq;
    if (outOfSequence)
    {
        onMetricEvent(BaseConnection::AudioMessageOutOfSequence);
        // TBD: consider skipping out-of-sequence messages
    }
    else
    {
        m_AudioLargestIncomingSeq = seq;
    }

    if (message.getBytesLeft() < sizeof(char) + sizeof(UINT32))
    {
        if (ENABLE_CONNECTION_TRACE)
            LOGGER_PRINTF("truncated\n");
        onMetricEvent(DropTruncatedMessage);
        return;
    }

    unsigned long longData;
    if (!message.readSafeLong(longData))
    {
        onMetricEvent(BaseConnection::DropReadServerFrameCounter);
        LOGGER_PRINTF("Attempting to read %d byte(s). %d byte(s) available.\n", sizeof(longData), message.getBytesLeft());
        return;
    }
    uint32_t serverFrameCounter = longData;

    unsigned char byteData;
    if (!message.readSafeByte(byteData))
    {
        onMetricEvent(BaseConnection::DropReadTakeNumber);
        LOGGER_PRINTF(" Attempting to read %d byte(s). %d byte(s) available.\n", sizeof(byteData), message.getBytesLeft());
        return;
    }
    int takeNumber = (int) byteData;

	if (m_inTake.number == takeNumber && m_inTake.stopped)
	{
        onMetricEvent(BaseConnection::DropTakeStopped);
        if (ENABLE_CONNECTION_TRACE)
            LOGGER_PRINTF("take stopped %d\n", takeNumber);
		return;
	}

    bool isNewTake = m_inTake.number != takeNumber;
	if (isNewTake)
	{
		if (m_eventSink) m_eventSink->onTakeBegin();

        if (ENABLE_CONNECTION_TRACE)
            LOGGER_PRINTF("take begins\n", takeNumber);
	}

	m_inTake.number = takeNumber;
	m_inTake.stopped = false;

    onMetricEvent(BaseConnection::ReadAudioPacket);

    bool isTakeStart = m_localBaseTimestamp == 0;
    if (isTakeStart) {
        m_localBaseTimestamp = message.timestamp();
        m_lastNormalizedLocalArrivalTime = -protocol::FRAME_TIMESLICE_MSEC;
        m_lastNormalizedLocalPlaybackTime = -protocol::FRAME_TIMESLICE_MSEC;
    }

    assert(message.timestamp() != 0);

    while (message.getBytesLeft() >= sizeof(char) + sizeof(char) + sizeof(UINT32) + sizeof(char)) // chid + blob len + frame counter + clientOnlyJitterBufferAware flag
	{
        unsigned char byteData;
        if (!message.readSafeByte(byteData))
        {
            onMetricEvent(BaseConnection::DropReadCid);
            LOGGER_PRINTF("Attempting to read %d byte(s). %d byte(s) available.\n", sizeof(byteData), message.getBytesLeft());
            break;
        }
        int chid = (int) byteData;
        assert(chid < protocol::CHANNEL_MAX_CLIENTS);

        size_t cbPayload = 0;
		const char *payload = message.readSmallBlob(&cbPayload);

        assert((cbPayload && payload) || (!cbPayload && !payload));

        if (payload == NULL) {
            onMetricEvent(BaseConnection::DropNullPayload);

            if (ENABLE_CONNECTION_TRACE) {
                LOGGER_PRINTF("skipped null payload, %d, remaining=%d\n",
                    serverFrameCounter, message.getBytesLeft());
            }
            continue;
        }

        unsigned long longData;
        if (!message.readSafeLong(longData))
        {
            onMetricEvent(BaseConnection::DropReadClientFrameCounter);
            LOGGER_PRINTF("Attempting to read %d byte(s). %d byte(s) available.\n", sizeof(longData), message.getBytesLeft());
            break;
        }
        UINT32 remoteClientCurrentFrameCounter = longData;

        // read whether the remote client is aware of the client-only jitter-buffer protocol 
        if (!message.readSafeByte(byteData))
        {
            onMetricEvent(BaseConnection::DropReadClientFrameCounter);
            LOGGER_PRINTF("Attempting to read %d byte(s). %d byte(s) available.\n", sizeof(longData), message.getBytesLeft());
            break;
        }
        bool isRemoteClientAwareOfClientOnlyJitterBuffer = byteData;

        // track the first audio message for a channel for time normalization
        if (m_jitterBase[chid].remoteClientFirstFrameCounter == 0) {
            m_jitterBase[chid].remoteClientFirstFrameCounter = remoteClientCurrentFrameCounter;
            m_jitterBase[chid].remoteClientFirstFrameLocalCounterOffset = m_localClientFrameCounter;

            if (common::ENABLE_PAYLOAD_TRACING) {
                LOGGER_PRINTF("ts\t%d\t\t%d\t%d\n",
                    m_jitterBase[chid].remoteClientFirstFrameCounter,
                    chid,
                    m_jitterBase[chid].remoteClientFirstFrameLocalCounterOffset);
            }
        }

        m_jitterBase[chid].gotPayload = 1;
        m_jitterBase[chid].remoteClientCurrentFrameRemoteCounter = remoteClientCurrentFrameCounter;

        uint32_t frameCounter;
        if (isRemoteClientAwareOfClientOnlyJitterBuffer) {
            frameCounter = (remoteClientCurrentFrameCounter - m_jitterBase[chid].remoteClientFirstFrameCounter) + m_jitterBase[chid].remoteClientFirstFrameLocalCounterOffset;
        } else {
            frameCounter = (remoteClientCurrentFrameCounter - m_jitterBase[chid].remoteClientFirstFrameCounter) / protocol::FRAME_TIMESLICE_MSEC + m_jitterBase[chid].remoteClientFirstFrameLocalCounterOffset;
            if (m_jitterBase[chid].mostRecentQueuedFrameCounter >= (frameCounter + options().clientJitterBufferSize)) {
                ++frameCounter;
	}
    }

        m_jitterBase[chid].mostRecentQueuedFrameCounter = frameCounter + options().clientJitterBufferSize;
        m_jitterBase[chid].jitterBuffer.push(m_jitterBase[chid].mostRecentQueuedFrameCounter, JitterInfo::JitterBufferEntryT( payload, cbPayload, message.timestamp() ));

        onMetricEvent(BaseConnection::ReadAudioPacketSize, cbPayload);

        if (common::ENABLE_PAYLOAD_TRACING) {
            LOGGER_PRINTF("+\t%d\t%d\t%d\t%d\t%d\t=(%d-%d)%s+%d+%d\t%d\n",
                m_jitterBase[chid].remoteClientCurrentFrameRemoteCounter,
                static_cast<int>(message.timestamp()),
                chid,
                cbPayload,
                m_jitterBase[chid].mostRecentQueuedFrameCounter,
                m_jitterBase[chid].remoteClientCurrentFrameRemoteCounter,
                m_jitterBase[chid].remoteClientFirstFrameCounter,
                (isRemoteClientAwareOfClientOnlyJitterBuffer) ? "" : "/ 20",
                m_jitterBase[chid].remoteClientFirstFrameLocalCounterOffset,
                options().clientJitterBufferSize,
                m_jitterBase[chid].jitterBuffer.size());
        }
    }
    }

void Connection::handleAudioEnd(Message &message)
{
	if (getState() != Connected)
	{
		return;
	}

    if (message.getBytesLeft() < sizeof(unsigned char) + sizeof(unsigned long))
    {
        onMetricEvent(DropTruncatedMessage);
        return;
    }

    unsigned char byteData;
    if (!message.readSafeByte(byteData))
    {
        LOGGER_PRINTF("Attempting to read %d byte(s). %d byte(s) available.\n", sizeof(byteData), message.getBytesLeft());
        return;
    }
    int takeNumber = (int) byteData;

    unsigned long longData;
    if (!message.readSafeLong(longData))
    {
        LOGGER_PRINTF("Attempting to read %d byte(s). %d byte(s) available.\n", sizeof(longData), message.getBytesLeft());
        return;
    }
    UINT32 serverFrameCounter = longData;

    (void) takeNumber;
    (void) serverFrameCounter;

	if (!m_inTake.stopped && m_inTake.number != 0)
	{
        if (ENABLE_CONNECTION_TRACE)
            LOGGER_PRINTF("te\n");

		if (m_eventSink) m_eventSink->onTakeEnd();
	}

	m_inTake.stopped = true;

    // push the audio stop into every active jitter buffer
    for (auto chid = 0; chid < protocol::CHANNEL_MAX_CLIENTS; ++chid) {

        if (m_jitterBase[chid].jitterBuffer.isStopped())
            continue;

        uint32_t priority = m_jitterBase[chid].mostRecentQueuedFrameCounter + 1;

        m_jitterBase[chid].jitterBuffer.push(priority, JitterInfo::JitterBufferEntryT(NULL, 0));

        // reset to allow next take to occur
        m_jitterBase[chid].remoteClientFirstFrameCounter = 0;

        if (common::ENABLE_PAYLOAD_TRACING)
            LOGGER_PRINTF("queued audio end, cid=%d, pri=%d\n", chid, priority);
    }

    m_localBaseTimestamp = 0;

	/*
	This is an edge case which happens if the server gets maximum loss from a client's take and thus starts ignoring it. 
	In that situation the client with full loss must bump its outgoing take number to make the server understand it's
	still talking. 
	
	The thinking around this is that forcing a client to retrieve the reliable AudioStop message before the server starts
	retransmitting protects other users from potentially bad experience of the lossy client */

	if (!m_outTake.stopped)
	{
		m_outTake.number ++;

		if (m_outTake.number == 0)
			m_outTake.number++;
	}
}

void Connection::handleChannelInactivity(Message &message)
{
    if (getState() != Connected)
    {
        return;
    }

    unsigned long longData;
    if (!message.readSafeLong(longData))
    {
        LOGGER_PRINTF(" Attempting to read %d byte(s). %d byte(s) available.\n", sizeof(longData), message.getBytesLeft());
        return;
    }
    unsigned long interval = longData;

    if( m_eventSink ) m_eventSink->onChannelInactivity(interval);
}

void Connection::handleNetworkPingReply(Message &message)
{
    if (getState() != Connected)
    {
        return;
    }

    // get send and receive times for this packet
    unsigned long long longLongData;
    if (!message.readSafeULongLong(longLongData))
    {
        LOGGER_PRINTF(__FUNCTION__ "Attempting to read %d byte(s). %d byte(s) available.\n", sizeof(longLongData), message.getBytesLeft());
        return;
    }
    common::Timestamp sendTime = longLongData;
    common::Timestamp recvTime = message.timestamp();

    // initialize
    if(m_networkPingLastSendTime == 0)
    {
        m_networkPingLastSendTime = sendTime;
        m_networkPingLastRecvTime = recvTime;
    }

    // compute network stats
    NetworkStats::Stat stat;
    stat.ping = recvTime - sendTime;
    stat.jitter = std::abs((recvTime - m_networkPingLastRecvTime) - (sendTime - m_networkPingLastSendTime));
    stat.sequence = message.getSequence();
    
    // accumulate network stats
    m_networkStats.addStat(stat);

    // save current send and receive times
    m_networkPingLastSendTime = sendTime;
    m_networkPingLastRecvTime = recvTime;
}

const Token &Connection::getToken() const
{
	return m_token;
}

void Connection::socketSendMessage(const Message &message)
{
	if (m_transport)
    {
        ssize_t socketResult = 0;
        m_transport->txMessageSend(message.getBufferPtr(), message.getBufferSize(), socketResult);
        if (socketResult < 0)
            onMetricEvent(BaseConnection::SocketSendError);
    }
}

void Connection::onDisconnected(sonar::CString &reasonType, sonar::CString &reasonDesc)
{
    int error = stopNetworkReadThread();
    if (error)
        return;

	m_channel.clients.clear();
    {
        CriticalSection::Locker lock(m_incomingMessagesCS);

        for (IncomingMessages::iterator i(m_incomingMessages.begin()), last(m_incomingMessages.end());
            i != last;
            ++i) {
                delete *i;
        }
        m_incomingMessages.clear();
    }
    m_timers.tsNextNetworkPing = -1;

	if (m_eventSink) m_eventSink->onDisconnect(reasonType, reasonDesc);

    // reset AFTER onDisconnect finishes using the network stats
    m_networkStats.reset();

    m_jitterBufferOccupancy.reset();
}

void Connection::onConnecting(void)
{
	setCID(0);

	m_timers.tsNextFrame = m_time->getMSECTime() + options().clientTickInterval;
	m_transport = m_network->connect(m_token.getVoipAddress());

	if (m_transport == NULL)
	{
		m_reasonType  = "UDP_ALLOC_FAILED";
		changeState(Disconnected);
        return;
	}

    if (startNetworkReadThread() != 0)
    {
        return;
    }

    setLocalAddress(m_transport->getLocalAddress());
    setRemoteAddress(m_transport->getRemoteAddress());

	sendRegister();
}

void Connection::onConnected(void)
{
    m_timers.tsNextNetworkPing = m_time->getMSECTime();
    m_timers.tsFirstNetworkPing = m_timers.tsNextNetworkPing;
    m_networkPingSequence = 0;

    m_timers.tsNextJitterBufferOccupancySample = m_time->getMSECTime();
}

void Connection::sendAudioStop(void)
{
	assert (!m_outTake.stopped);

	if (!m_outTake.stopped)
	{
		Message audioStop(protocol::MSG_AUDIO_STOP, getCID(), true);
		audioStop.writeUnsafeByte(m_outTake.number);
        audioStop.writeUnsafeLong(m_AudioFrameCounter++);
        //audioStop.writeUnsafeLong(connectionTime());
		sendMessage(audioStop);
	}

	m_outTake.stopped = true;
}

unsigned char Connection::dbgGetOutTakeNumber(void)
{
	return m_outTake.number;
}

void Connection::dbgSetOutTakeNumber(unsigned char number)
{
	m_outTake.number = number;
}


void Connection::sendAudioPayload(const void *payload, size_t cbSize, sonar::INT64 timestamp)
{
	assert (cbSize <= protocol::PAYLOAD_MAX_SIZE);

	if (cbSize > protocol::PAYLOAD_MAX_SIZE)
	{
		return;
	}

	if (getState () != Connected)
	{
		return;
	}

	if (m_outTake.stopped)
	{
		m_outTake.number ++;

		if (m_outTake.number == 0)
			m_outTake.number++;
		
		m_outTake.stopped = false;
	}

    int cid = getCID();
    Message audioToServer(protocol::MSG_AUDIO_TO_SERVER, cid, false);
    audioToServer.updateSequence(++m_AudioLastOutgoingSeq);
	audioToServer.writeUnsafeByte(m_outTake.number);
    //audioToServer.writeUnsafeLong(timestamp ? (timestamp - m_startConnectTime) : connectionTime());
    audioToServer.writeUnsafeLong(m_AudioFrameCounter++);
	audioToServer.writeUnsafeSmallBlob(payload, cbSize);
	sendMessage(audioToServer);

    onMetricEvent(BaseConnection::SentAudioPacket);
    onMetricEvent(BaseConnection::SentAudioPacketSize, cbSize);
}

bool Connection::validatePacket(const Message &message)
{
	if (isConnected() && getCID() != message.getHeader().source)
	{
		onMetricEvent(DropBadCid);
        sonar::common::Log("sonar: validatePacket DropBadCid: cid=%d, source=%d, msg=%s", getCID(), message.getHeader().source, cmdToString(message.getHeader().cmd).c_str());

		return false;
	}

	return true;
}

void Connection::onMetricEvent(MetricEvent evt, int count)
{
    if (!m_stats)
        return;

    switch (evt)
    {
    case SentAudioPacket:           m_stats->audioMessagesSent() += count; break;
    case SentAudioPacketSize:	    m_stats->audioBytesSent() += count; break;
    case ReadAudioPacket:		    m_stats->audioMessagesReceived() += count; break;
    case ReadAudioPacketSize:       m_stats->audioBytesReceived() += count; break;
    case AudioMessageOutOfSequence:	m_stats->audioMessagesOutOfSequence() += count; break;
    case LostAudioPacket:		    m_stats->audioMessagesLost() += count; break;
    case AudioMixClipping:          m_stats->audioMixClipping() += count; break;
    case SocketRecvError:           m_stats->socketRecvError() += count; break;
    case SocketSendError:           m_stats->socketSendError() += count; break;

    case DropBadCid:                m_stats->badCid() += count; break;
    case DropTruncatedMessage:      m_stats->truncatedMessages() += count; break;

    case DropNotConnected:          m_stats->dropNotConnected() += count; break;
    case DropReadServerFrameCounter:m_stats->dropReadServerFrameCounter() += count; break;
    case DropReadTakeNumber:        m_stats->dropReadTakeNumber() += count; break;
    case DropTakeStopped:           m_stats->dropTakeStopped() += count; break;
    case DropReadCid:               m_stats->dropReadCid() += count; break;
    case DropNullPayload:           m_stats->dropNullPayload() += count; break;
    case DropReadClientFrameCounter:m_stats->dropReadClientFrameCounter() += count; break;
    case JitterBufferUnderrun:      m_stats->jitterbufferUnderrun() += count; break;
    case JitterBufferDepth:         m_stats->jitterBufferDepth() = count; break;

    default:
        break;
    }
}

uint32_t Connection::voiceServerAddressIPv4() const
{
	// compute the server IP address from the connection
	char* voip = const_cast<char*>(getToken().getVoipAddress().c_str());
	uint32_t serverIP = std::strtoul(voip, &voip, 10);
	serverIP <<= 8; serverIP |= std::strtoul(++voip, &voip, 10);
	serverIP <<= 8; serverIP |= std::strtoul(++voip, &voip, 10);
	serverIP <<= 8; serverIP |= std::strtoul(++voip, &voip, 10);
	return serverIP;
}

NetworkStats::NetworkStats(int32_t goodLoss, int32_t goodJitter, int32_t goodPing,
                           int32_t okLoss, int32_t okJitter, int32_t okPing,
                           int32_t poorLoss, int32_t poorJitter, int32_t poorPing,
                           int32_t jitterBufferSize)
: m_goodLoss(goodLoss)
, m_goodJitter(goodJitter)
, m_goodPing(goodPing)
, m_okLoss(okLoss)
, m_okJitter(okJitter)
, m_okPing(okPing)
, m_poorLoss(poorLoss)
, m_poorJitter(poorJitter)
, m_poorPing(poorPing)
, m_jitterBufferSize(jitterBufferSize)
, m_numberOfSamples(0)
, m_pingTotal(0)
, m_jitterTotal(0)
, m_largestSequence(0)
, m_rollingPingTotal(0)
, m_rollingJitterTotal(0)
, m_pingMin(UINT32_MAX)
, m_pingMax(0)
{
}

NetworkStats::~NetworkStats()
{
}

void NetworkStats::addStat(const Stat& stat)
{
    // overall stats
    m_numberOfSamples++;
    m_pingTotal += stat.ping;
    m_jitterTotal += stat.jitter;
    m_largestSequence = stat.sequence;

    // pseudo-windowed ping min & max - reset manually
    if (m_pingMin > stat.ping)
        m_pingMin = stat.ping;
    if (m_pingMax < stat.ping)
        m_pingMax = stat.ping;

    // rolling average of NETWORK_QUALITY_PING_ROLLING_WINDOW_SIZE samples
    m_rollingWindow.push(Element(stat, stat.sequence));
    m_rollingPingTotal += stat.ping;
    m_rollingJitterTotal += stat.jitter;
    if (m_largestSequence >= NETWORK_QUALITY_PING_ROLLING_WINDOW_SIZE)
    {
        // remove data outside rolling window
        uint32_t oldSequence = m_largestSequence - NETWORK_QUALITY_PING_ROLLING_WINDOW_SIZE;
        Element e = m_rollingWindow.top();
        while( e.data().sequence <= oldSequence )
        {
            m_rollingPingTotal -= e.data().ping;
            m_rollingJitterTotal -= e.data().jitter;
            m_rollingWindow.pop();
            e = m_rollingWindow.top();
        }
    }
}

int NetworkStats::convertToQuality(int32_t loss, int32_t jitter, int32_t ping, int32_t received)
{
    int quality = 3;

    int32_t lossPercentage = 100.f * (float)loss / (float)(loss + received);
    int32_t jitterPercentage = 0;
    if( ping > 0 )
    {
        // take into account jitter buffer effect
        int32_t jitterBuffered = jitter - m_jitterBufferSize * protocol::FRAME_TIMESLICE_MSEC;
        if( jitterBuffered < 0 )
            jitterBuffered = 0;
        jitterPercentage = 100.0f * ((float)jitterBuffered / (float)ping);
    }

    // http://www.pingtest.net/learn.php
    if( lossPercentage < m_goodLoss && jitterPercentage < m_goodJitter && ping <= m_goodPing ) // A and B
        quality = 3;
    else if( lossPercentage < m_okLoss && jitterPercentage < m_okJitter && ping <= m_okPing ) // C
        quality = 2;
    else if( lossPercentage < m_poorLoss && jitterPercentage < m_poorJitter && ping <= m_poorPing ) // D
        quality = 1;
    else
        quality = 0;

    return quality;
}

int NetworkStats::getQuality()
{
    int quality = 0;

    int32_t numberOfSamples = m_rollingWindow.size();
    if( numberOfSamples )
    {
        assert(numberOfSamples <= NETWORK_QUALITY_PING_ROLLING_WINDOW_SIZE+1);
        int32_t ping = m_rollingPingTotal / numberOfSamples;
        int32_t jitter = m_rollingJitterTotal / numberOfSamples;
        int32_t loss = (m_largestSequence >= NETWORK_QUALITY_PING_ROLLING_WINDOW_SIZE) ? NETWORK_QUALITY_PING_ROLLING_WINDOW_SIZE : m_largestSequence;
        loss -= numberOfSamples;

        // convert to network quality (0-3)
        quality = convertToQuality(loss, jitter, ping, numberOfSamples);
    }

    return quality;
}

NetworkStats::Stats NetworkStats::getStats(bool overall /*= false*/)
{
    Stats stats;

    if( overall )
    {
        if( m_numberOfSamples > 0 )
        {
            stats.numberOfSamples = m_numberOfSamples;
            stats.loss = m_largestSequence - m_numberOfSamples;
            stats.jitter = m_jitterTotal / m_numberOfSamples;
            stats.ping = m_pingTotal / m_numberOfSamples;
            stats.pingMin = 0; // we currently aren't tracking overall ping min
            stats.pingMax = 0; // we currently aren't tracking overall ping max
            stats.quality = convertToQuality(stats.loss, stats.jitter, stats.ping, stats.numberOfSamples);
        }
    }
    else
    {
        int32_t numberOfSamples = m_rollingWindow.size();
        if( numberOfSamples )
        {
            assert(numberOfSamples <= NETWORK_QUALITY_PING_ROLLING_WINDOW_SIZE+1);
            stats.numberOfSamples = numberOfSamples;
            stats.loss = (m_largestSequence >= NETWORK_QUALITY_PING_ROLLING_WINDOW_SIZE) ? NETWORK_QUALITY_PING_ROLLING_WINDOW_SIZE : m_largestSequence;
            stats.loss -= numberOfSamples;
            stats.jitter = m_rollingJitterTotal / numberOfSamples;
            stats.ping = m_rollingPingTotal / numberOfSamples;
            stats.pingMin = m_pingMin;
            stats.pingMax = m_pingMax;
            stats.quality = convertToQuality(stats.loss, stats.jitter, stats.ping, stats.numberOfSamples);
        }
    }

    return stats;
}

void NetworkStats::reset()
{
    m_numberOfSamples = 0;
    m_pingTotal = 0;
    m_jitterTotal = 0;
    m_largestSequence = 0;

    m_rollingPingTotal = 0;
    m_rollingJitterTotal = 0;
    while(!m_rollingWindow.empty())
    {
        m_rollingWindow.pop();
    }
}

void NetworkStats::resetWindowedPingStats() {
    m_pingMin = UINT32_MAX;
    m_pingMax = 0;
}

}
