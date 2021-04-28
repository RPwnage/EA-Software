#define NOMINMAX 1

#include "Channel.h"
#include "ClientConnection.h"
#include <assert.h>
#include "Server.h"

#include <algorithm>
#include <sstream>

namespace sonar
{

Channel::LatencyStats& Channel::LatencyStats::operator +=(const sonar::ClientConnection& conn)
{
    const ClientToServerStats &clientStats = conn.getClientStats();
    float latencySamples = (float) conn.getNumClientStatsReceived();

    deltaPlaybackMean += (unsigned int)((float)clientStats.deltaPlaybackMean / latencySamples);
    deltaPlaybackMax += (unsigned int)((float)clientStats.deltaPlaybackMax / latencySamples);
    receiveToPlaybackMean += (unsigned int)((float)clientStats.receiveToPlaybackMean / latencySamples);
    receiveToPlaybackMax += (unsigned int)((float)clientStats.receiveToPlaybackMax / latencySamples);

    return *this;
}

Channel::LatencyStats& Channel::LatencyStats::operator /=(int clientCount)
{
    if( clientCount > 0 )
    {
        deltaPlaybackMean = (unsigned int)((float)deltaPlaybackMean / (float)clientCount);
        deltaPlaybackMax = (unsigned int)((float)deltaPlaybackMax / (float)clientCount);
        receiveToPlaybackMean = (unsigned int)((float)receiveToPlaybackMean / (float)clientCount);
        receiveToPlaybackMax = (unsigned int)((float)receiveToPlaybackMax / (float)clientCount);
    }

    return *this;
}

Channel::Channel(CString &channelId, CString &channelDesc, long long nextFrame, long long now)
	: m_id(channelId)
	, m_desc(channelDesc)
	, m_clientCount(0)
	, m_nextFrame(nextFrame)
	, m_LastOutgoingAudioMessageSeq(0)
	, m_InactivityEventTime(0)
	, m_InactivityCheck(false)
	, m_BlockingStates(protocol::CHANNEL_MAX_CLIENTS, std::string(protocol::CHANNEL_MAX_CLIENTS, '0'))
	, m_BlockingMessageMap(protocol::CHANNEL_MAX_CLIENTS, NULL)
	, m_BlockingMessages()
{
	memset (m_clients, 0, sizeof(m_clients));
	memset (&m_stats, 0, sizeof(m_stats));

	m_stats.created = now;

	m_takeNumber = 0xab;
	m_audioState = Stopped;
    m_frameCounter = 1; // start at 1 since the client assumes that 0-frames are non-audio frames

	initInactivity();
}

Channel::~Channel(void)
{
	assert (m_clientCount == 0);
}

size_t Channel::getClientCount() const
{
	return m_clientCount;
}

void Channel::addClient(ClientConnection &client)
{
	int chid = 0;

	while (chid < sonar::protocol::CHANNEL_MAX_CLIENTS && m_clients[chid] != NULL)
	{
		chid ++;
	}

    assert (client.getCID() != 0);
	assert (chid < sonar::protocol::CHANNEL_MAX_CLIENTS);
	
	client.setCHID(chid);

	for (int index = 0; index < sonar::protocol::CHANNEL_MAX_CLIENTS; index ++)
	{
		ClientConnection *peer = m_clients[index];

		if (peer == NULL)
		{
			continue;
		}

		ClientConnection::Message msgChannelJoinNotif(sonar::protocol::MSG_CHANNEL_JOINNOTIF, 0, true);
		msgChannelJoinNotif.writeUnsafeByte( (unsigned char) client.getCHID());
		msgChannelJoinNotif.writeUnsafeSmallString(client.getUserId());
		msgChannelJoinNotif.writeUnsafeSmallString(client.getUserDesc());

		peer->sendMessage(msgChannelJoinNotif);

		m_stats.outJoinNotifs ++;
	}
	
	assert (m_clients[chid] == NULL);

	m_clients[chid] = &client;
	m_clientCount ++;

    addBlocking(client);
}

void Channel::removeClient(ClientConnection &client, sonar::CString &reasonType, sonar::CString &reasonDesc)
{
	assert (m_clients[client.getCHID()] != NULL);
	assert (m_clients[client.getCHID()] == &client);

	m_clients[client.getCHID()] = NULL;

	for (int index = 0; index < sonar::protocol::CHANNEL_MAX_CLIENTS; index ++)
	{
		ClientConnection *peer = m_clients[index];

		if (peer == NULL)
		{
			continue;
		}
		ClientConnection::Message msgPartNotif(sonar::protocol::MSG_CHANNEL_PARTNOTIF, 0, true);
		msgPartNotif.writeUnsafeByte( (unsigned char) client.getCHID());
		msgPartNotif.writeUnsafeSmallString(reasonType);
		msgPartNotif.writeUnsafeSmallString(reasonDesc);
		peer->sendMessage(msgPartNotif);

		m_stats.outPartNotifs ++;
	}

	m_clientCount --;
    removeBlocking(client);
}

CString &Channel::getId(void) const
{
	return m_id;
}

CString &Channel::getDesc(void) const
{
	return m_desc;
}

ClientConnection **Channel::getClients(size_t *outLen)
{
	*outLen = sizeof(m_clients) / sizeof(ClientConnection *);
	return m_clients;
}

ClientConnection *Channel::getClient(int index)
{
	ClientConnection *peer = NULL;
	
	for (int i = 0; i < sonar::protocol::CHANNEL_MAX_CLIENTS; i ++)
	{
		peer = m_clients[i];

		if (peer == NULL)
		{
			continue;
		}
		else if( index == i )
		{
			break;
		}
	}
	
	return peer;
}

void Channel::update(long long now)
{
	long long diff = now - m_nextFrame;

	while (diff >= sonar::protocol::FRAME_TIMESLICE_MSEC)
	{
		frame();

		diff -= sonar::protocol::FRAME_TIMESLICE_MSEC;
		m_nextFrame += sonar::protocol::FRAME_TIMESLICE_MSEC;
	}
}


// client blocking members
void Channel::debugDumpBlockingInfo() const
{
    if (ENABLE_VOIP_BLOCKING_TRACE)
    {
        for (int i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
        {
            if (m_clients[i] == NULL)
                continue;
            SYSLOG(LOG_DEBUG, "Channel::states: [%d] %s -> %s", i, m_BlockingStates[i].c_str(), m_BlockingMessageMap[i]->blockingInfo().c_str());
        }
    }
}

void Channel::debugDumpBlockingMessageClientMap() const
{
    if (ENABLE_VOIP_BLOCKING_TRACE)
    {
        SYSLOG(LOG_DEBUG, "m_BlockingMessages: size = [%d]\n", m_BlockingMessages.size());
        for (auto i = 0U; i < m_BlockingMessages.size(); ++i)
        {
            if (m_BlockingMessages[i].client == NULL)
                continue;
            SYSLOG(LOG_DEBUG, "m_BlockingMessages[%d]: client cid = [%d]\n", i, m_BlockingMessages[i].client->getCID());
        }
    }
}

void Channel::recomputeBlockingMessages()
{
    // make a normalized copy of the blocking states so that if 'a' blocks 'b' then 'b' blocks 'a'
    BlockingStates normalized(m_BlockingStates);
    for (int i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
    {
        for (int j = i + 1; j < protocol::CHANNEL_MAX_CLIENTS; ++j)
        {
            const std::string& a = normalized[i];
            const std::string& b = normalized[j];
            char newValue = (a.at(j) == '1' || b.at(i) == '1') ? '1' : '0';

            std::string c = normalized[i];
            std::string d = normalized[j];
            c[j] = newValue;
            d[i] = newValue;
            normalized[i] = c;
            normalized[j] = d;
        }
    }

    // set up the blocking messages to use
    m_BlockingMessages.resize(normalized.size());
    assert(m_BlockingMessages.size() == sonar::protocol::CHANNEL_MAX_CLIENTS);
    for (auto i = 0U; i < normalized.size(); ++i)
    {
        m_BlockingMessages[i].message.setBlockingInfo(normalized[i]);
        m_BlockingMessages[i].client = NULL;
    }

    // create the message map - mapping client => Message
    for (auto i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
    {
        m_BlockingMessageMap[i] = NULL;

        if (m_clients[i] == NULL)
            continue;

        m_BlockingMessageMap[i] = &m_BlockingMessages[i].message;
        m_BlockingMessages[i].client = m_clients[i];

        assert(m_BlockingMessageMap[i] != NULL);
    }

    debugDumpBlockingMessageClientMap();
}

void Channel::clearBlocking(const ClientConnection& client)
{
    // set all blocking states for the given client to unblocked (false)
    int chid = client.getCHID();
    if (chid < 0 || chid >= protocol::CHANNEL_MAX_CLIENTS)
    {
        SYSLOGX(LOG_ERR, "bad chid=%d", chid);
        debugDumpBlockingInfo();
        return;
    }

    m_BlockingStates[chid].assign(protocol::CHANNEL_MAX_CLIENTS, '0');

    // recompute the message map and messages
    recomputeBlockingMessages();

    if (ENABLE_VOIP_BLOCKING_TRACE)
        SYSLOG(LOG_DEBUG, "Channel::clearBlocking");
    debugDumpBlockingInfo();
}

void Channel::updateBlocking(const ClientConnection& client, const ClientConnection::ClientBlockList& blockList)
{
    int chid = client.getCHID();
    if (chid < 0 || chid >= protocol::CHANNEL_MAX_CLIENTS)
    {
        SYSLOGX(LOG_ERR, "bad chid=%d", chid);
        debugDumpBlockingInfo();
        return;
    }
    assert(m_clients[chid] != NULL);

    // iterate over the currently connected clients
    for (auto i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
    {
        if (m_clients[i] == NULL)
            continue;

        // determine whether the current client is on the block list
        const ClientConnection& candidate = *m_clients[i];
        auto where = std::find(blockList.cbegin(), blockList.cend(), candidate.getUserId());
        m_BlockingStates[chid][i] = (where != blockList.cend()) ? '1' : '0';
    }

    // recompute the message map and messages
    recomputeBlockingMessages();

    if (ENABLE_VOIP_BLOCKING_TRACE)
        SYSLOG(LOG_DEBUG, "Channel::updateBlocking");
    debugDumpBlockingInfo();
}

void Channel::addBlocking(const ClientConnection& client)
{
    int chid = client.getCHID();
    if (chid < 0 || chid >= protocol::CHANNEL_MAX_CLIENTS)
    {
        SYSLOGX(LOG_ERR, "bad chid=%d", chid);
        debugDumpBlockingInfo();
        return;
    }
    assert(m_clients[chid] != NULL);

    // iterate over the currently connected clients
    for (auto i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
    {
        if (m_clients[i] == NULL)
            continue;

        const ClientConnection& current = *m_clients[i];

        {
            // determine whether the current client is on the block list of the newly added client
            const ClientConnection::ClientBlockList& blockList = client.blockList();
            auto where = std::find(blockList.cbegin(), blockList.cend(), current.getUserId());
            if (where != blockList.cend())
                m_BlockingStates[chid][i] = '1';
        }

        {
            // determine whether the newly added client is on the block list of the current client
            const ClientConnection::ClientBlockList& blockList = current.blockList();
            auto where = std::find(blockList.cbegin(), blockList.cend(), client.getUserId());
            if (where != blockList.cend())
                m_BlockingStates[i][chid] = '1';
        }
    }

    // recompute the message map and messages
    recomputeBlockingMessages();

    SYSLOG(LOG_DEBUG, "Channel::addBlocking");
    debugDumpBlockingInfo();
}
 
void Channel::removeBlocking(const ClientConnection& client)
{
    if (ENABLE_VOIP_BLOCKING_TRACE)
        SYSLOG(LOG_DEBUG, "Channel::removeBlocking");
    clearBlocking(client);
    int chid = client.getCHID();
    assert(m_clients[chid] == NULL);
    m_BlockingMessageMap[chid] = NULL;
}

void Channel::frame(void)
{
	assert (m_clientCount > 0);
	size_t payloadCount = 0;
	m_stats.frames ++;

    for (auto i = m_BlockingMessages.begin(), last = m_BlockingMessages.end(); i != last; ++i)
    {
        if (i->client != NULL )
            i->message.reset(sonar::protocol::MSG_AUDIO_TO_CLIENT, 0, false, i->client->isClientOnlyJitterBufferAware());
    }

	int realPayloadCount = 0;
	unsigned int totalPayloadSize = 0;

    // 1) add server frame counter
	// 2) make room for the take numbers
	std::vector<unsigned char*> takePtr(m_BlockingMessages.size(), NULL);
    assert(takePtr.size() == sonar::protocol::CHANNEL_MAX_CLIENTS);
	for (unsigned int i = 0, last = m_BlockingMessages.size(); i != last; ++i)
	{
        const ClientConnection *connection = m_BlockingMessages[i].client;
        if (connection == NULL)
            continue;

        // add frame counter to message if necessary
        assert(connection->getCID() > 0);
        if (connection->hasProtocolFrameCounter())
        {
            m_BlockingMessages[i].message.writeUnsafeLong(m_frameCounter);
        }

        // reserve space for take number
		takePtr[i] = (unsigned char *) m_BlockingMessages[i].message.writeUnsafeReserve(1);
	}
	    
	// add audio content for every connected client
	for (int index = 0; index < sonar::protocol::CHANNEL_MAX_CLIENTS; index ++)
	{
		ClientConnection *client = m_clients[index];

		if (client == NULL)
			continue;
		
		AudioPayload *payload = client->getInAudio();

		if (payload == NULL)
			continue;

		const int chid = client->getCHID();
        assert(chid < protocol::CHANNEL_MAX_CLIENTS);

		payloadCount ++;
		m_stats.talkers ++;

		// iterate over all of our messages and determine which ones will get the audio from this client
		for (auto m = 0U; m < m_BlockingMessages.size(); ++m)
		{
            if (m_BlockingMessages[m].client == NULL)
                continue;

			Message& audioToClient = m_BlockingMessages[m].message;

			// determine if audio from this client is going into this message
			if (audioToClient.blockingInfo()[chid] == '1')
				continue;

			if (payload == ClientConnection::m_nullFrame)
			{
				Server::getInstance().getMetrics().audioNullFrames ++;

				if (audioToClient.getBytesLeft() >= 2)
				{
					audioToClient.writeUnsafeByte( (unsigned char) chid);
					audioToClient.writeUnsafeSmallBlob(NULL, 0);
				}
				else
				{
					Server::getInstance().getMetrics().tooManyTalkers ++;
				}
			}
			else
			{
				if(payload->isStopFrame())
				{
					Server::getInstance().getMetrics().dropStopFrame ++; // stop frames should never make it here
				}
				else
				{
                    const ClientConnection *connection = m_BlockingMessages[m].client;
                    assert(connection);

					realPayloadCount++;

					m_stats.inPayloads ++;
					m_stats.inPayloadsSize += payload->getSize();
					totalPayloadSize += payload->getSize();

					Server::getInstance().getMetrics().inboundPayloads ++;

                    size_t byteCount = payload->getSize() + 1 + 1;
                    bool hasFrameCounter = connection->hasProtocolFrameCounter();
                    if (hasFrameCounter)
                        byteCount += 4;
                    if (connection->isClientOnlyJitterBufferAware())
                        ++byteCount;

					if (audioToClient.getBytesLeft() >= byteCount)
					{
						audioToClient.writeUnsafeByte( (unsigned char) chid);
						audioToClient.writeUnsafeSmallBlob(payload->getPtr(), payload->getSize());

                        if (hasFrameCounter)
                            audioToClient.writeUnsafeLong( payload->getFrameCounter() );

                        if (connection->isClientOnlyJitterBufferAware())
                            audioToClient.writeUnsafeByte(payload->getSourceVersion() >= protocol::PROTOCOL_CLIENTONLYJITTERBUFFERAWARE);
                    }
					else
					{
						Server::getInstance().getMetrics().tooManyTalkers ++;
					}
				}
			}
		}

		if (payload != ClientConnection::m_nullFrame)
        {
		    delete payload;
        }
	}

	if (payloadCount == 0)
	{
		if (getAudioState() == Started)
		{
			setAudioState(Stopped);
			resetInactivity();
		}

		checkInactivity();

		return;
	} 

	if (getAudioState() == Stopped)
	{
		setAudioState(Started);
	}

    // fix up the take numbers
    unsigned char takeNumber = getTakeNumber();
    ++m_LastOutgoingAudioMessageSeq;
    for (unsigned int i = 0, last = m_BlockingMessages.size(); i != last; ++i)
    {
        if (m_BlockingMessages[i].client == NULL)
            continue;

        assert(takePtr[i] != NULL);
        if (takePtr[i] != NULL)
            *takePtr[i] = takeNumber;
        m_BlockingMessages[i].message.updateSequence(m_LastOutgoingAudioMessageSeq);
    }

	// send messages to all connected clients
	for (int index = 0; index < sonar::protocol::CHANNEL_MAX_CLIENTS; index ++)
	{
		ClientConnection *client = m_clients[index];

		if (client == NULL)
			continue;

		client->sendMessage(*m_BlockingMessageMap[index]);
		++m_stats.outPayloads;
		m_stats.outPayloadsSize += totalPayloadSize;

		Server::getInstance().getMetrics().outboundPayloads += realPayloadCount;
	}

    m_frameCounter++;
}

void Channel::setAudioState(AudioState state)
{
	if (getAudioState() == Stopped && state == Started)
	{
		m_takeNumber ++;

		if (m_takeNumber == 0)
			m_takeNumber ++;
	} 
	else
	if (getAudioState() == Started && state == Stopped)
	{
        ClientConnection::Message audioStop(sonar::protocol::MSG_AUDIO_STOP, 0, true);
        audioStop.writeUnsafeByte(m_takeNumber);

        ClientConnection::Message audioStopWithFrameCounter(sonar::protocol::MSG_AUDIO_STOP, 0, true);
        audioStopWithFrameCounter.writeUnsafeByte(m_takeNumber);
        audioStopWithFrameCounter.writeUnsafeLong(m_frameCounter++);

		for (int index = 0; index < sonar::protocol::CHANNEL_MAX_CLIENTS; index ++)
		{
			ClientConnection *client = m_clients[index];

			if (client == NULL)
				continue;

            if (client->hasProtocolFrameCounter() )
                client->sendMessage(audioStopWithFrameCounter);
            else
                client->sendMessage(audioStop);
        }
	}


	m_audioState = state;
}

Channel::AudioState Channel::getAudioState()
{
	return m_audioState;
}

unsigned char Channel::getTakeNumber()
{
	return m_takeNumber;
}

const Channel::Statistics &Channel::getStatistics()
{
	return m_stats;
}

void Channel::initInactivity()
{
    // inactivity
#if 0 // debug
    m_InactivityIntervals[0] = 30000; // 30 seconds
    m_InactivityIntervals[1] = 30000; // + 30 seconds = 1 minute
#else
    m_InactivityIntervals[0] = 60000; // 1 minutes
    m_InactivityIntervals[1] = 3540000; // + 59 minutes = 1 hour
#endif

    resetInactivity();
}

void Channel::resetInactivity()
{
    m_InactivityCheck = true;
    m_InactivityCheckIndex = 0;
    m_InactivityEventTime = common::getTimeAsMSEC() + m_InactivityIntervals[m_InactivityCheckIndex];
}

void Channel::checkInactivity()
{
    if (m_InactivityCheck && common::getTimeAsMSEC() > m_InactivityEventTime)
    {
        unsigned long totalInterval = 0;
        for (int i = 0; i < m_InactivityCheckIndex + 1; ++i)
        {
            totalInterval += m_InactivityIntervals[i];
        }

        sendInactivityMessage(totalInterval);

        m_InactivityCheckIndex++;
        if ( m_InactivityCheckIndex < INACTIVITY_INTERVAL_COUNT)
        {
            m_InactivityEventTime = common::getTimeAsMSEC() + m_InactivityIntervals[m_InactivityCheckIndex];
        }
        else
        {
            m_InactivityCheck = false;
        }

    }
}

void Channel::sendInactivityMessage(unsigned long interval)
{
    for (int index = 0; index < sonar::protocol::CHANNEL_MAX_CLIENTS; index ++)
    {
        ClientConnection *client = m_clients[index];

        if (client == NULL)
            continue;

        ClientConnection::Message channelInactivity(sonar::protocol::MSG_CHANNEL_INACTIVITY, 0, true);
        channelInactivity.writeUnsafeLong( interval );

        client->sendMessage(channelInactivity);
    }
}

void Channel::updateRankScore()
{
	// average percentages for all clients
	m_percentageStats.loss = 0.0f;
	m_percentageStats.oos = 0.0f;

	// iterate over the currently connected clients
	for (auto i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
	{
		if (m_clients[i] == NULL)
			continue;

		// add percentage from client
		m_percentageStats.loss += m_clients[i]->getPercentageStats().loss;
        m_percentageStats.oos += m_clients[i]->getPercentageStats().oos;
	}
	
	if( m_clientCount > 0 )
	{
		m_percentageStats.loss /= m_clientCount;
        m_percentageStats.oos /= m_clientCount;
	}

	m_rankScore = m_percentageStats.loss + m_percentageStats.oos + getJitterStats().arrivalJitterMean / 2000.0;

    SYSLOG(LOG_DEBUG, "channel rank: [%s] %.2f = %.2f lost + %.2f oos + %.2f arrival (%.2f playback)\n"
        , m_id.c_str()
        , m_rankScore
        , m_percentageStats.loss
        , m_percentageStats.oos
        , getJitterStats().arrivalJitterMean / 2000.0
        , getJitterStats().playbackJitterMean
        );
}

void Channel::updateLatencyStats()
{
    m_latencyStats.reset();

	// average latency stats from all clients
	// iterate over the currently connected clients
	for (auto i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
	{
		if (m_clients[i] == NULL)
			continue;

        m_latencyStats += *m_clients[i];
	}

    m_latencyStats /= m_clientCount;
}

const Channel::JitterStats& Channel::updateJitterStats()
{
    m_jitterStats.reset();

    // average jitter stats from all clients
    // iterate over the currently connected clients
    for (auto i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
    {
        if (m_clients[i] == NULL)
            continue;

        const ClientConnection::JitterStats &jitterStats = m_clients[i]->updateJitterStats();

        m_jitterStats.arrivalJitterMax = std::max<>(jitterStats.arrivalJitterMax, m_jitterStats.arrivalJitterMax);
        m_jitterStats.arrivalJitterMean += jitterStats.arrivalJitterMean;
        m_jitterStats.playbackJitterMax = std::max<>(jitterStats.playbackJitterMax, m_jitterStats.playbackJitterMax);
        m_jitterStats.playbackJitterMean += jitterStats.playbackJitterMean;
    }

    if( m_clientCount > 0 )
    {
        m_jitterStats.arrivalJitterMean = m_jitterStats.arrivalJitterMean / m_clientCount;
        m_jitterStats.playbackJitterMean = m_jitterStats.playbackJitterMean / m_clientCount;
    }

    return m_jitterStats;
}

const Channel::NetworkStats& Channel::updateNetworkStats()
{
    m_networkStats.reset();

    int clientsWithNetworkStats = 0;

    // average network stats from all clients
    // iterate over the currently connected clients
    for (auto i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
    {
        if (m_clients[i] == NULL)
            continue;

        const ClientConnection::NetworkStats &networkStats = m_clients[i]->updateNetworkStats();
        if (networkStats.valid)
        {
            clientsWithNetworkStats++;

            m_networkStats.loss += (float)networkStats.loss;
            m_networkStats.jitter += (float)networkStats.jitter;
            m_networkStats.ping += (float)networkStats.ping;
            m_networkStats.received += (float)networkStats.received;
            m_networkStats.quality += (float)networkStats.quality;
        }
    }

    if (clientsWithNetworkStats > 0)
    {
        m_networkStats.valid = true;
        m_networkStats.loss = m_networkStats.loss / (float)clientsWithNetworkStats;
        m_networkStats.jitter = m_networkStats.jitter / (float)clientsWithNetworkStats;
        m_networkStats.ping = m_networkStats.ping / (float)clientsWithNetworkStats;
        m_networkStats.received = m_networkStats.received / (float)clientsWithNetworkStats;
        m_networkStats.quality = m_networkStats.quality / (float)clientsWithNetworkStats;

    }

    return m_networkStats;
}

bool Channel::operator < (const Channel* channel) const
{
	return (m_rankScore < channel->m_rankScore);
}

void Channel::updateClientRanking()
{
	// rank client by voice quality performance based on client stats
	m_rankedClients.clear();
	// iterate over the currently connected clients
	for (auto i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
	{
		if (m_clients[i] == NULL)
			continue;
		
		//m_clients[i]->updateRankScore(); // This is already done for each client in ClientConnection::handleClientStats()
		m_rankedClients.push_back(m_clients[i]);
	}

	// sort 'm_rankedClients'
	std::sort(m_rankedClients.begin(), m_rankedClients.end());
}

CString Channel::getWorstClientList()
{
	String clientList;

	updateClientRanking();
	
	if( m_rankedClients.empty() )
	{
		  clientList = "no clients";
	}
	else
	{
		RankedClientList::iterator iter = m_rankedClients.begin();
		for( int i = 0; i < 10 && iter != m_rankedClients.end(); ++i, ++iter )
		{
			ClientConnection* client = *iter;
			if (client == NULL)
				continue;

			if( i > 0 )
				clientList += ", ";

			char number[5];
			sprintf(number, "%d", i);

			clientList += number;
			clientList += ") ";
			clientList += client->getName();
		}
	}
	
	return clientList;
}

bool Channel::isBad(int channelThreshold, int clientThreshold)
{
	bool bad = false;

	// count number of bad clients
	int badClientCount = 0;
	for (auto i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
	{
		if (m_clients[i] == NULL)
			continue;

		if( m_clients[i]->isBad(clientThreshold) )
			badClientCount++;
	}

	int totalClients = getClientCount();
	if( totalClients > 0 )
	{
		float badClientPercentage = float(badClientCount) / float(totalClients);
		badClientPercentage *= 100.f;
		float thresholdPercent = float(channelThreshold);
		if( badClientPercentage > thresholdPercent )
			bad = true;
	}

	return bad;
}

void Channel::resetClientStats()
{
    for (auto i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
    {
        if (m_clients[i] == NULL)
            continue;

        m_clients[i]->resetClientStats();
    }
}

void Channel::resetJitterStats()
{
 for (auto i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
    {
        if (m_clients[i] == NULL)
            continue;

        m_clients[i]->resetJitterStats();
    }
}

void Channel::writeMetricsJsonAllZero(
    char *filename)
{
    writeMetricsJson(filename, 0, 0.0f, ClientToServerStats(), LatencyStats(), JitterStats(), NetworkStats());
}

void Channel::writeMetricsJson(
    char *filename)
{
    float avgTalkers = (m_stats.frames > 0) ? ((float)m_stats.talkers / (float)m_stats.frames) : 0.f;
    writeMetricsJson(filename, getClientCount(), avgTalkers, m_clientStats, m_latencyStats, m_jitterStats, m_networkStats);
}

void Channel::writeMetricsJson(
    char *filename,
    int clientCount,
    float avgTalkers,
    ClientToServerStats const& clientStats,
    LatencyStats const& latencyStats,
    JitterStats const& jitterStats,
    NetworkStats const& networkStats
    )
{
#ifndef _WIN32
	if( filename == NULL )
		return;

	FILE *f = fopen(filename, "w");
	if( f != NULL )
	{
		fprintf(f,
			"{\"metrics\": {\n"
			"  \"state\": {\n"
			"    \"clients\": %u,\n"
			"    \"avgTalkers\": %0.2f\n"
			"  },\n"
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
            "    \"networkLoss\": %0.2f,\n"
            "    \"networkJitter\": %0.2f,\n"
            "    \"networkPing\": %0.2f,\n"
            "    \"networkReceived\": %0.2f,\n"
            "    \"networkQuality\": %0.4f\n"
            "  }\n"
			"}}",
			clientCount,
			avgTalkers,
			clientStats.sent,
			clientStats.received,
			clientStats.outOfSequence,
			clientStats.lost,
			clientStats.badCid,
			clientStats.truncated,
			clientStats.audioMixClipping,
			clientStats.socketRecvError,
			clientStats.socketSendError,
			latencyStats.deltaPlaybackMean,
			latencyStats.deltaPlaybackMax,
			latencyStats.receiveToPlaybackMean,
			latencyStats.receiveToPlaybackMax,
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

void Channel::writeClientJson()
{
	for( int i = 0; i < 10; ++i )
	{
		writeClientJson(i);
	}
}

void Channel::writeClientJson(unsigned int index)
{
#ifndef _WIN32
  	ClientConnection* client = NULL;
	if( index < m_rankedClients.size() )
	{
		client = m_rankedClients[index];
	}

	const char *clientJsonLocation = "Node/sonar-voiceserver/";
	char filename[256];
	snprintf(filename, 256, "%sworst_client_%d.json", clientJsonLocation, index);

    if( client == NULL )
    {
        ClientConnection::writeMetricsJsonAllZero(filename);
    }
    else
    {
	    client->writeMetricsJson(filename);
    }
#endif
}

void Channel::updateStats()
{
	memset(&m_clientStats, 0x00, sizeof(m_clientStats));

	// iterate over the currently connected clients
	for (auto i = 0; i < protocol::CHANNEL_MAX_CLIENTS; ++i)
	{
		if (m_clients[i] == NULL)
			continue;

		// add stats from client
		m_clientStats.sent += m_clients[i]->getClientStats().sent;
		m_clientStats.received += m_clients[i]->getClientStats().received;
		m_clientStats.outOfSequence += m_clients[i]->getClientStats().outOfSequence;
		m_clientStats.lost += m_clients[i]->getClientStats().lost;
		m_clientStats.badCid += m_clients[i]->getClientStats().badCid;
		m_clientStats.truncated += m_clients[i]->getClientStats().truncated;
		m_clientStats.audioMixClipping += m_clients[i]->getClientStats().audioMixClipping;
		m_clientStats.socketRecvError += m_clients[i]->getClientStats().socketRecvError;
		m_clientStats.socketSendError += m_clients[i]->getClientStats().socketSendError;
	}
}

}
