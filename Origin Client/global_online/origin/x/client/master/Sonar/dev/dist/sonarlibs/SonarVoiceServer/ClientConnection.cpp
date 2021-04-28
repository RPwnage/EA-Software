#define NOMINMAX 1

#include "ClientConnection.h"
#include "Server.h"

#include <SonarCommon/Common.h>

#include <assert.h>
#include <sstream>
#include <stdarg.h>

#ifdef _WIN32
typedef SSIZE_T ssize_t;
#endif

namespace sonar
{

class ServerTimeProvider : public sonar::ITimeProvider
{
	virtual common::Timestamp getMSECTime() const
	{
		return Server::getInstance().getTime();
	}
};

class ServerDebugLogger : public sonar::IDebugLogger
{
	virtual void printf(const char *format, ...) const
	{
/*
		va_list args;
		va_start (args, format);
        #if __linux
            SYSLOGV(LOG_DEBUG, format, args);
        #else
    		vfprintf(stderr, format, args);
        #endif
		va_end(args);
*/
	}
};


static ServerTimeProvider s_timeProvider;
static ServerDebugLogger s_logProvider;

AudioPayload *ClientConnection::m_nullFrame = (AudioPayload *) 0x1;

ClientConnection::ClientConnection(
    unsigned long version,
	CString &userId,
	CString &userDesc,
	int cid, sonar::protocol::SEQUENCE txNextSeq,
	const sockaddr_in *addr,
	Channel *channel,
	Operator *oper,
	long long nextFrame,
	int loss,
	int bufferSize,
	int jitterScaleFactor)
	: BaseConnection(s_timeProvider, s_logProvider, ConnectionOptions(20000), cid % 1000)
	, m_operator(oper)
	, m_channel(channel)
	, m_isDetached(false)
	, m_nextFrame(nextFrame)
	, m_audioLoss(loss)
	, m_AudioLargestIncomingSeq(0)
	, m_AudioMessagesReceived(0)
	, m_AudioIncomingMessageOverflowCounter(0)
	, m_BlockList()
	, m_InboundBlockList()
	, m_numClientStatsReceived(0)
	, m_rankScore(0.0f)
	, m_percentageStats()
	, m_jitterScaleFactor(jitterScaleFactor)
	, m_version(version)
{
	m_in.started = false;
	m_in.loss = 0;
	m_in.takeNumber = 0;
	m_in.talkBalance = 0;

	setUserId(userId);
	setUserDesc(userDesc);
	setCID(cid);

	if (addr)
	{
		m_addr = *addr;
		setRemoteAddress(m_addr);
	}
	setLocalAddress(common::getVoipAddress());

	changeState(Connecting);
	changeState(Connected);

	m_tx.nextSeq = txNextSeq;
	resetClientStats();
}

ClientConnection::~ClientConnection(void)
{
    {
        CriticalSection::Locker lock(m_in.payloadQueueCS);
        // need to delete any remaining payloads, not just clear the queue
        for (ClientConnection::PayloadQueue::iterator i(m_in.payloadQueue.begin()), last(m_in.payloadQueue.end());
            i != last;
            ++i) {
            delete *i;
        }
        m_in.payloadQueue.clear();
    }

    Server::getInstance().getMetrics().audioIncomingMessagesLost = ((m_AudioIncomingMessageOverflowCounter << 16) + m_AudioLargestIncomingSeq) - m_AudioMessagesReceived;
}

Operator &ClientConnection::getOperator(void) const
{
	return *m_operator;
}

void ClientConnection::update(long long now)
{
	long long diff = now - m_nextFrame;

	while (diff >= options().clientTickInterval)
	{
		frame();

		diff -= options().clientTickInterval;
		m_nextFrame += options().clientTickInterval;
	}
}

void ClientConnection::detach()
{
	assert (!isDetached());
	m_isDetached = true;
}

bool ClientConnection::isDetached()
{
	return m_isDetached;
}

void ClientConnection::handleAudioToServer(Message &message)
{
    ++m_AudioMessagesReceived;
    protocol::SEQUENCE seq = message.getSequence();
    int diff = seq - m_AudioLargestIncomingSeq;
    bool overflow = (diff > 32767) || (diff < -32767);
    bool outOfSequence = (diff > 0 && overflow) | (diff < 0 && !overflow);
    if (outOfSequence)
    {
        //SYSLOG(LOG_DEBUG, "seq=%d, largest=%d, diff=%d, overflow=%d", seq, m_AudioLargestIncomingSeq, (int) diff, (int) overflow);
        onMetricEvent(AudioMessageOutOfSequence);
        // TBD: consider skipping out-of-sequence messages
    }
    else 
    {
        if (overflow)
            ++m_AudioIncomingMessageOverflowCounter;
        m_AudioLargestIncomingSeq = seq;
    }

	if (message.getBytesLeft() < 1)
	{
        onMetricEvent(DropEmptyPayload);
		return;
	}
	
	unsigned char takeNumber = message.readUnsafeByte();

    uint32_t frameCounter = 0;
    if( hasProtocolFrameCounter() )
        frameCounter = message.readUnsafeLong();

	size_t cbPayload;
	const char *payload = message.readSmallBlob(&cbPayload);

    // force audio payload drop
    if (m_audioLoss > 0 && ((rand () % 100) + 1) < m_audioLoss)
    {
        onMetricEvent(DropForced);
        return;
    }

	if (payload == NULL)
	{
		onMetricEvent(DropBadPayload);
		return;
	}

	if (cbPayload > sonar::protocol::PAYLOAD_MAX_SIZE)
	{
		onMetricEvent(DropPayloadSize);
		return;
	}

//     if (ENABLE_VOIP_BLOCKING_TRACE)
//         SYSLOGX(LOG_DEBUG, "payload from = %s", getUserId().c_str());

    AudioPayload *ap = new AudioPayload(payload, cbPayload, takeNumber, frameCounter, m_version);
    assert(ap);
    {
        CriticalSection::Locker lock(m_in.payloadQueueCS);
        m_in.payloadQueue.push_back(ap);
    }
}

void ClientConnection::handleAudioStop(Message &message)
{
	if (message.getBytesLeft() < 1)
	{
        onMetricEvent(DropTruncatedMessage);
		return;
	}

	unsigned char takeNumber = message.readUnsafeByte();
    {
        CriticalSection::Locker lock(m_in.payloadQueueCS);
        m_in.payloadQueue.push_back(new AudioPayload(takeNumber));
    }
}

void ClientConnection::handleRegister(Message &message)
{
	assert (getCID() == 0);

	unsigned long version = 0;
	
	if (message.getBytesLeft() < 6)
	{
        onMetricEvent(DropTruncatedMessage);
		return;
	}

	version = message.readUnsafeLong();

	sonar::protocol::SEQUENCE txNextSeq = message.readUnsafeLong();

	if (!isProtocolSupported(version))
	{
		sendRegisteReplyError(message.getAddress(), "PROTOCOL_VERSION", "");
		return;
	}

	CString connectToken = message.readBigString();
	
	Server::getInstance().registerClient(message.getAddress(), connectToken, txNextSeq, version);
}

void ClientConnection::handleBlockListFragment(Message& message)
{
    if (message.getBytesLeft() < 4)
    {
        onMetricEvent(DropTruncatedMessage);
        return;
    }

    unsigned int numIdsInPacket = message.readUnsafeLong();

#if (ENABLE_VOIP_BLOCKING_TRACE)
    std::ostringstream dbg;
    dbg << "ClientConnection: block list fragment, #ids = " << numIdsInPacket;
#endif

    for (; numIdsInPacket; --numIdsInPacket)
    {
        // don't need to check message.getBytesLeft() since readSmallString() is safe
        BlockingId id = message.readSmallString();
        if (!id.empty())
        {
            m_InboundBlockList.push_back(id);
#if (ENABLE_VOIP_BLOCKING_TRACE)
            dbg << " " << id;
#endif
        }
    }

#if (ENABLE_VOIP_BLOCKING_TRACE)
    SYSLOG (LOG_DEBUG, dbg.str().c_str());
#endif
}

void ClientConnection::handleBlockListComplete(Message& /*message*/)
{
    if (ENABLE_VOIP_BLOCKING_TRACE)
        SYSLOGX(LOG_DEBUG, "block list complete\n");

    m_BlockList = m_InboundBlockList;
    m_InboundBlockList.clear();
    m_channel->updateBlocking(*this, m_BlockList);
}

void ClientConnection::handleEcho(Message &message)
{
	//FIXME: Don't process this message in prod?
	Message reply(sonar::protocol::MSG_ECHO, getCID(), true);

	reply.writeUnsafe(message.readUnsafe(message.getBytesLeft()), message.getBytesLeft());

	sendMessage(reply);
}

void ClientConnection::handleClientStats(Message& message)
{
    resetClientStats();

    if (message.getBytesLeft() < sizeof(short))
    {
        onMetricEvent(DropTruncatedMessage);
        SYSLOGX(LOG_DEBUG, "insufficient data");
        return;
    }

    short numStats = message.readUnsafeShort();

    if (numStats != sonar::CLIENT_STATS_COUNT)
    {
        SYSLOGX(LOG_DEBUG, "Unexpected client stats, expected=%d, got=%d", sonar::CLIENT_STATS_COUNT, numStats);
    }

    if (message.getBytesLeft() >= 4)
        m_clientStats.sent += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.received += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.outOfSequence += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.lost += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.badCid += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.truncated += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.audioMixClipping += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.socketRecvError += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.socketSendError += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.deltaPlaybackMean += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.deltaPlaybackMax += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.receiveToPlaybackMean += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.receiveToPlaybackMax += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4)
        m_clientStats.jitterArrivalMean += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4) {
        int jam = message.readUnsafeLong();
        m_clientStats.jitterArrivalMax = std::max(jam, m_clientStats.jitterArrivalMax);
    }

    if (message.getBytesLeft() >= 4)
        m_clientStats.jitterPlaybackMean += message.readUnsafeLong();

    if (message.getBytesLeft() >= 4) {
        int jpm = message.readUnsafeLong();
        m_clientStats.jitterPlaybackMax = std::max(jpm, m_clientStats.jitterPlaybackMax);
    }

    // network quality
    if (hasProtocolNetworkStats())
    {
        if (message.getBytesLeft() >= 4)
            m_clientStats.networkLoss += message.readUnsafeLong();

        if (message.getBytesLeft() >= 4)
            m_clientStats.networkJitter += message.readUnsafeLong();

        if (message.getBytesLeft() >= 4)
            m_clientStats.networkPing += message.readUnsafeLong();

        if (message.getBytesLeft() >= 4)
            m_clientStats.networkReceived += message.readUnsafeLong();

        if (message.getBytesLeft() >= 4)
            m_clientStats.networkQuality += message.readUnsafeLong();
    }

    m_numClientStatsReceived++;

    std::ostringstream info;
    info << "s=" << m_clientStats.sent
        << ", r=" << m_clientStats.received
        << ", oos=" << m_clientStats.outOfSequence
        << ", l=" << m_clientStats.lost
        << ", cid=" << m_clientStats.badCid
        << ", t=" << m_clientStats.truncated
        << ", ac=" << m_clientStats.audioMixClipping
        << ", sre=" << m_clientStats.socketRecvError
        << ", sse=" << m_clientStats.socketSendError
        << ", dpa=" << m_clientStats.deltaPlaybackMean
        << ", dpm=" << m_clientStats.deltaPlaybackMax
        << ", rpa=" << m_clientStats.receiveToPlaybackMean
        << ", rpm=" << m_clientStats.receiveToPlaybackMax
        << ", jaa=" << m_clientStats.jitterArrivalMean
        << ", jam=" << m_clientStats.jitterArrivalMax
        << ", jpa=" << m_clientStats.jitterPlaybackMean
        << ", jpm=" << m_clientStats.jitterPlaybackMax
        << ", nlo=" << m_clientStats.networkLoss
        << ", nji=" << m_clientStats.networkJitter
        << ", npi=" << m_clientStats.networkPing
        << ", nre=" << m_clientStats.networkReceived
        << ", nqu=" << m_clientStats.networkQuality;

    SYSLOG(LOG_DEBUG, "Client stats: %s\n", info.str().c_str());
    
	updateRankScore();
}

void ClientConnection::handleNetworkPing(Message& message)
{
    Message networkPingReply(sonar::protocol::MSG_NETWORK_PING_REPLY, getCID(), false);
    networkPingReply.updateSequence(message.getSequence());

    common::Timestamp sendTime = message.readUnsafeULongLong();
    networkPingReply.writeUnsafeULongLong(sendTime);

    sendMessage(networkPingReply);
}

void ClientConnection::sendRegisteReplyError(const sockaddr_in &addr, sonar::CString &reasonType, sonar::CString &reasonDesc)
{
	Message rrmsg(sonar::protocol::MSG_REGISTERREPLY, 0, false);
	rrmsg.writeUnsafeSmallString(reasonType);
	rrmsg.writeUnsafeSmallString(reasonDesc);

	ClientConnection::socketSendMessage(rrmsg, addr);
}

void ClientConnection::socketSendMessage(const Message &message, const struct sockaddr_in &addr)
{
	ssize_t result = sendto(Server::getInstance().getSocket(), 
		(const char *) message.getBufferPtr(), 
		message.getBufferSize(), 
		common::DEF_MSG_NOSIGNAL,
		(const sockaddr *) &addr, sizeof (sockaddr_in));

    if (result < 0) {
        SYSLOGX(LOG_ERR, "socket send err=%d, addr={%04x,%04x,%08x} #bytes=%d", errno, addr.sin_family, addr.sin_port, addr.sin_addr.s_addr, message.getBufferSize());
        logMetricEvent(SendError);
    }
}

void ClientConnection::socketSendMessage(const Message &message)
{
	socketSendMessage(message, m_addr);
}

Channel &ClientConnection::getChannel(void) const
{
	return *m_channel;
}

void ClientConnection::handleMessage(Message &message)
{
	switch (message.getHeader().cmd)
	{
		case sonar::protocol::MSG_REGISTER:
			handleRegister(message);
			break;
		case sonar::protocol::MSG_CHALLENGE:
			Server::getInstance().handleChallenge(message);
			break;

		case sonar::protocol::MSG_ECHO:
			//FIXME: Should probably not reply to this in prod
			handleEcho(message);
			break;

		case sonar::protocol::MSG_AUDIO_TO_SERVER:
			handleAudioToServer(message);
			break;

		case sonar::protocol::MSG_AUDIO_STOP:
			handleAudioStop(message);
			break;

        case sonar::protocol::MSG_BLOCK_LIST_COMPLETE:
            handleBlockListComplete(message);
            break;

        case sonar::protocol::MSG_BLOCK_LIST_FRAGMENT:
            handleBlockListFragment(message);
            break;

        case sonar::protocol::MSG_CLIENT_STATS:
            handleClientStats(message);
            break;

        case sonar::protocol::MSG_NETWORK_PING:
            handleNetworkPing(message);
            break;

		default:
			BaseConnection::handleMessage(message);
			break;
	}
}


void ClientConnection::onConnecting(void)
{
}

void ClientConnection::onConnected(void)
{
}

void ClientConnection::onDisconnected(sonar::CString &reasonType, sonar::CString &reasonDesc)
{
	if (!isDetached())
	{
		Server::getInstance().unregisterClient(*this, reasonType, reasonDesc);

		if (reasonType.find("CONNECTION_LOST") != sonar::CString::npos)
		{
			onMetricEvent(DropConnection);
		}
	}

	if (isDetached())
	{
		Server::getInstance().destroyClient(this);
	}
}

sonar::protocol::SEQUENCE ClientConnection::getNextRxSeq(void)
{
	return m_rx.nextAck;
}

const sockaddr_in &ClientConnection::getRemoteAddress()
{
	return m_addr;
}

AudioPayload *ClientConnection::getInAudio()
{
    if (m_in.payloadQueue.empty())
        return NULL;
    
    AudioPayload *payload = NULL;
    {
        CriticalSection::Locker lock(m_in.payloadQueueCS);
        payload = m_in.payloadQueue.front();
        m_in.payloadQueue.pop_front();
    }
    
	if (!m_in.started)
	{
#if ENABLE_TALK_BALANCE
		m_in.talkBalance --;

		if (m_in.talkBalance < 0)
			m_in.talkBalance = 0;
#endif

		if (payload == NULL)
		{
			return NULL;
		}

#if ENABLE_TALK_BALANCE
		if (m_in.talkBalance >= Server::getInstance().getConfig().talkBalanceMin)
		{
			delete payload;
			return NULL;
		}
#endif

		if (m_in.takeNumber == payload->getTake())
		{
			// Late packet 6:[6S]:6
			onMetricEvent(DropLatePacketNotStarted);
			delete payload;
			return getInAudio();
		}

		m_in.loss = 0;
		m_in.takeNumber = payload->getTake();
		m_in.started = true;
		return payload;
	}

	// Started
	if (payload == NULL)
	{
		m_in.loss ++;

		if (m_in.loss >= sonar::protocol::MAX_AUDIO_LOSS)
		{
            onMetricEvent(MaxAudioLoss);
			m_in.started = false;
            m_in.takeNumber = 0;
		}

		payload = m_nullFrame;
	} 
	else
	{
		if (!((payload->getTake() ==  m_in.takeNumber) ||
			(payload->getTake() > m_in.takeNumber && (payload->getTake() - m_in.takeNumber) < 0x7f) ||
			(payload->getTake() < m_in.takeNumber && (m_in.takeNumber - payload->getTake()) > 0x7f)))
		{
			// Late packet
			onMetricEvent(DropLatePacket);
			delete payload;
			return getInAudio();
		}

		m_in.takeNumber = payload->getTake();

		if (payload->isStopFrame())
		{
			m_in.started = false;
			//onMetricEvent(DropStopFrame); // This is a legit stop frame; do not mark as a bad frame.
			delete payload;
			return getInAudio();
		}

		m_in.loss = 0;
	}

#if ENABLE_TALK_BALANCE
	int talkBalanceMax = Server::getInstance().getConfig().talkBalanceMax;
	if (m_in.talkBalance <= talkBalanceMax)
	{
		m_in.talkBalance ++;
	}

	if (m_in.talkBalance == talkBalanceMax)
	{
		m_in.talkBalance = talkBalanceMax + Server::getInstance().getConfig().talkBalancePenalty;
		if (payload != m_nullFrame)
		{
			onMetricEvent(DropTalkBalance);
			delete payload;
		}
		m_in.started = false;
		return getInAudio();
	}
#endif

	return payload;
}

bool ClientConnection::validatePacket(const Message &message)
{
	const protocol::MessageHeader header = message.getHeader();

	if (getCID() == 0)
	{
		if (header.cmd != protocol::MSG_REGISTER && header.cmd != protocol::MSG_CHALLENGE)
		{
			onMetricEvent(DropBadCid);
			return false;
		}

		if (message.isReliable())
		{
			onMetricEvent(DropBadCid);
			return false;
		}
	}
	else
	{
		if (header.source != getCID())
		{
			onMetricEvent(DropBadCid);
			return false;
		}
	}

	return true;
}

// rank
void ClientConnection::updateRankScore()
{
    m_percentageStats.loss = 0.0f;
    m_percentageStats.oos = 0.0f;

    // lost packet percentage
    unsigned int denominator = m_clientStats.lost + m_clientStats.received;
    if( denominator > 0 )
    {
	    m_percentageStats.loss = (float)m_clientStats.lost / (float)denominator;
    }

    // out of sequence packet percentage
    if( m_clientStats.received > 0 )
    {
	    m_percentageStats.oos = (float)m_clientStats.outOfSequence / (float)m_clientStats.received;
    }

    // note: these are not true percentages but simply values that are placed in an appropriate
    // value range to match the lost and oos values.
    float arrivalPercentage = m_clientStats.jitterArrivalMean / (float)m_jitterScaleFactor;
    float playbackPercentage = m_clientStats.jitterPlaybackMean / (float)m_jitterScaleFactor;
	m_rankScore = m_percentageStats.loss + m_percentageStats.oos + arrivalPercentage;

    SYSLOG(LOG_DEBUG, "client rank: %.3f = %.3f lost + %.3f oos + %.3f arrival (%.3f playback)\n"
        , m_rankScore
        , m_percentageStats.loss
        , m_percentageStats.oos
        , arrivalPercentage
        , static_cast<float>(playbackPercentage)
    );
}

bool ClientConnection::operator < (const ClientConnection* connection) const
{
	return (m_rankScore < connection->m_rankScore);
}

void ClientConnection::writeMetricsJsonAllZero(char *filename)
{
    writeMetricsJson(filename, 0, 0, 0, 0, ClientToServerStats(), JitterStats(), NetworkStats());
}

void ClientConnection::writeMetricsJson(char *filename)
{
    unsigned int deltaPlaybackMean = (float)m_clientStats.deltaPlaybackMean / (float)m_numClientStatsReceived;
    unsigned int deltaPlaybackMax = (float)m_clientStats.deltaPlaybackMax / (float)m_numClientStatsReceived;
    unsigned int receiveToPlaybackMean = (float)m_clientStats.receiveToPlaybackMean / (float)m_numClientStatsReceived;
    unsigned int receiveToPlaybackMax = (float)m_clientStats.receiveToPlaybackMax / (float)m_numClientStatsReceived;
    writeMetricsJson(filename, deltaPlaybackMean, deltaPlaybackMax, receiveToPlaybackMean, receiveToPlaybackMax, m_clientStats, m_jitterStats, m_networkStats);
}

void ClientConnection::writeMetricsJson(
    char *filename,
    unsigned int deltaPlaybackMean,
    unsigned int deltaPlaybackMax,
    unsigned int receiveToPlaybackMean,
    unsigned int receiveToPlaybackMax,
    ClientToServerStats const& clientStats,
    JitterStats const& jitterStats,
    NetworkStats const& networkStats)
{
#ifndef _WIN32
	if( filename == NULL )
		return;

	FILE *f = fopen(filename, "w");
	if( f != NULL )
	{
		fprintf(f,
			"{\"metrics\": {\n"
			"  \"counter\": {\n"
			"    \"sent\": %u,\n"
			"    \"received\": %u,\n"
			"    \"outOfSequence\": %u,\n"
			"    \"lost\": %u,\n"
			"    \"badCid\": %u,\n"
			"    \"truncated\": %u,\n"
			"    \"audioMixClipping\": %u,\n"
			"    \"socketRecvError\": %u,\n"
			"    \"socketSendError\": %u\n"
			"  },\n"
			"  \"latency\": {\n"
			"    \"deltaPlaybackMean\": %u,\n"
			"    \"deltaPlaybackMax\": %u,\n"
			"    \"receiveToPlaybackMean\": %u,\n"
			"    \"receiveToPlaybackMax\": %u\n"
            "  },\n"
            "  \"jitter\": {\n"
            "    \"jitterArrivalMean\": %.2f,\n"
            "    \"jitterArrivalMax\": %d,\n"
            "    \"jitterPlaybackMean\": %.2f,\n"
            "    \"jitterPlaybackMax\": %d\n"
            "  },\n"
            "  \"network\": {\n"
            "    \"networkLoss\": %u,\n"
            "    \"networkJitter\": %u,\n"
            "    \"networkPing\": %u,\n"
            "    \"networkReceived\": %u,\n"
            "    \"networkQuality\": %u\n"
            "  }\n"
            "}}",
			clientStats.sent,
			clientStats.received,
			clientStats.outOfSequence,
			clientStats.lost,
			clientStats.badCid,
			clientStats.truncated,
			clientStats.audioMixClipping,
			clientStats.socketRecvError,
			clientStats.socketSendError,
            deltaPlaybackMean,
            deltaPlaybackMax,
            receiveToPlaybackMean,
            receiveToPlaybackMax,
            jitterStats.arrivalJitterMean,
            jitterStats.arrivalJitterMax,
            jitterStats.playbackJitterMean,
            jitterStats.playbackJitterMax,
            networkStats.loss,
            networkStats.jitter,
            networkStats.ping,
            networkStats.received,
            networkStats.quality
		);

		fclose(f);
	}
#endif
}

void ClientConnection::resetClientStats()
{
	memset(&m_clientStats, 0x00, sizeof(m_clientStats));
	m_numClientStatsReceived = 0;
}

bool ClientConnection::isBad(int threshold)
{
	float thresholdPercent = float(threshold);
	if( m_rankScore * 100.f > thresholdPercent )
		return true;
	
	return false;
}

const ClientConnection::JitterStats &ClientConnection::getJitterStats()
{
    return m_jitterStats;
}

void ClientConnection::resetJitterStats()
{
    m_jitterStats.arrivalJitterMean = 0.0f;
    m_jitterStats.arrivalJitterMax = 0;
    m_jitterStats.playbackJitterMean = 0.0f;
    m_jitterStats.playbackJitterMax = 0;
}

const ClientConnection::JitterStats& ClientConnection::updateJitterStats()
{
    resetJitterStats();

    // jitter score
    // not really a percentage but a similar representative value
    if (m_numClientStatsReceived > 0) {
        m_jitterStats.arrivalJitterMean = m_clientStats.jitterArrivalMean / m_numClientStatsReceived;
        m_jitterStats.arrivalJitterMax = m_clientStats.jitterArrivalMax;
        m_jitterStats.playbackJitterMean = m_clientStats.jitterPlaybackMean / m_numClientStatsReceived;
        m_jitterStats.playbackJitterMax = m_clientStats.jitterPlaybackMax;
    }

    return m_jitterStats;
}

const ClientConnection::NetworkStats& ClientConnection::updateNetworkStats()
{
    m_networkStats.reset();

    // network stats
    if (hasProtocolNetworkStats() && m_numClientStatsReceived > 0) {
        m_networkStats.valid = true;
        m_networkStats.loss = m_clientStats.networkLoss / m_numClientStatsReceived;
        m_networkStats.jitter = m_clientStats.networkJitter / m_numClientStatsReceived;
        m_networkStats.ping = m_clientStats.networkPing / m_numClientStatsReceived;
        m_networkStats.received = m_clientStats.networkReceived / m_numClientStatsReceived;
        m_networkStats.quality = m_clientStats.networkQuality / m_numClientStatsReceived;
    }

    return m_networkStats;
}

void ClientConnection::logMetricEvent(const MetricEvent evt, int count)
{
	switch (evt)
	{
	case DropBadCid:		        Server::getInstance().getMetrics().dropBadCid ++; break;
	case AckUnexpectedSeq:		    Server::getInstance().getMetrics().ackUnexpectedSeq ++; break;
	case Resend:			        Server::getInstance().getMetrics().resends ++; break;
	case SendPacket:		        Server::getInstance().getMetrics().sendPacket ++; break;
	case ReadPacket:		        Server::getInstance().getMetrics().readPacket ++; break;
	case DropBadState:		        Server::getInstance().getMetrics().dropBadState ++; break;
	case DropBadHeader:		        Server::getInstance().getMetrics().dropBadHeader ++; break;
	case DropUnexpectedMsg:		    Server::getInstance().getMetrics().dropUnexpectedMsg ++; break;
	case DropUnexpectedSeq:		    Server::getInstance().getMetrics().dropUnexpectedSeq ++; break;
	case DropPayloadSize:		    Server::getInstance().getMetrics().dropPayloadSize ++; break;
	case DropOverflow:		        Server::getInstance().getMetrics().dropOverflow ++; break;
    case DropLatePacketNotStarted:  Server::getInstance().getMetrics().dropLatePacketNotStarted ++; break;
    case DropLatePacket:		    Server::getInstance().getMetrics().dropLatePacket ++; break;
	case DropForced:		        Server::getInstance().getMetrics().dropForced ++; break;
	case DropEmptyPayload:		    Server::getInstance().getMetrics().dropEmptyPayload ++; break;
	case DropStopFrame:		        Server::getInstance().getMetrics().dropStopFrame ++; break;
	case DropTalkBalance:		    Server::getInstance().getMetrics().dropTalkBalance ++; break;
	case SendReliable:		        Server::getInstance().getMetrics().sendReliable ++; break;
	case AudioMessageOutOfSequence:	Server::getInstance().getMetrics().audioIncomingMessagesBadSequence ++; break;
	case DropConnection:		    Server::getInstance().getMetrics().dropConnection ++; break;
	case RecvError:		            Server::getInstance().getMetrics().recvErrors ++; break;
	case SendError:		            Server::getInstance().getMetrics().sendErrors ++; break;
	case DropTruncatedMessage:      Server::getInstance().getMetrics().dropTruncatedMessages ++; break;
	case MaxAudioLoss:              Server::getInstance().getMetrics().maxAudioLoss ++; break;

	case DropBadSeq:
	case DropBadPayload:
        	break;

    case LostAudioPacket:
    default:
        break;
	}
}

}
