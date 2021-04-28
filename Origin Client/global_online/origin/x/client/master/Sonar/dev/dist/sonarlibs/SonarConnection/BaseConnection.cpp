#include <assert.h>
#include <stdlib.h>
#include <SonarCommon/Common.h>
#include <SonarConnection/Message.h>
#include <SonarConnection/BaseConnection.h>
#include <openssl/md5.h>
#include <stdint.h>
#include <time.h>

#ifdef _WIN32
#include <Windows.h>
#elif __APPLE__
#include <mach/mach_time.h>
#endif

namespace sonar
{

const ConnectionOptions DefaultConnectionOptions;

#if 1 // secure ACK counter
protocol::SEQUENCE BaseConnection::random(protocol::SEQUENCE unwanted)
{
	uint32_t value = 0;
	do
	{
		uint32_t data[4];
		data[0] = m_addr_local.sin_addr.s_addr;
		data[1] = m_addr_remote.sin_addr.s_addr;
		data[2] = (m_addr_local.sin_port << 16) + m_addr_remote.sin_port;
		data[3] = rand(); // secret: seeded with time

		MD5_CTX md5_ctx = { 0 };
		int32_t digest[4];
		int rc;
		
		rc = MD5_Init(&md5_ctx);
		rc = MD5_Update(&md5_ctx, (const void*)data, sizeof(data));
		rc = MD5_Final((unsigned char*)digest, &md5_ctx);
        (void) rc;

		value = digest[0];

#ifdef _WIN32
        static double PCFreq = 0.0;
        static bool PCFreqInitted = false;
        LARGE_INTEGER li;

        if( !PCFreqInitted )
        {
            if(QueryPerformanceFrequency(&li))
            {
                PCFreq = double(li.QuadPart)/1000000000.0;
                PCFreqInitted = true;
            }
        }

        if( QueryPerformanceCounter(&li) )
        {
            value += (int64_t)(double(li.QuadPart)/PCFreq) >> 6;
        }
    
#elif __APPLE__

    static mach_timebase_info_data_t sTimebaseInfo = { 0 };
    if ( sTimebaseInfo.denom == 0 ) {
        (void) mach_timebase_info(&sTimebaseInfo);
    }

    uint64_t time = mach_absolute_time();

    // Convert to nanoseconds.
    uint64_t nanoTime = time * sTimebaseInfo.numer / sTimebaseInfo.denom;
    // Convert to milliseconds
    value = static_cast<uint32_t>(nanoTime / 1000000);

#else
		struct timespec ts;
		if( clock_gettime(CLOCK_REALTIME, &ts) == 0)
		{
			value += ((ts.tv_sec * 1000000000) + ts.tv_nsec) >> 6; // increment every 64ns
		}
#endif
	} while (value == unwanted);

	return (protocol::SEQUENCE) value;
}
#else
static protocol::SEQUENCE random(protocol::SEQUENCE unwanted)
{
	int value = 0;

	do
	{
		value = rand();
	} while (value == unwanted);

	return (protocol::SEQUENCE) value;
}
#endif

BaseConnection::BaseConnection(
	const ITimeProvider &time,
	const IDebugLogger &logger,
	const ConnectionOptions& options,
	int nextPingOffset) 
    : m_options(options)
	, m_cid(-1)
	, m_chid(-1)
	, m_nextPingOffset(nextPingOffset)
	, m_currRelMessage(NULL)
	, m_state(Disconnected)
	, m_logger(&logger)
	, m_time(&time)
	, m_remoteClosed(false)
{
	m_tx.nextSeq = 0;
	m_tx.nextResend = -1;
	m_timers.nextPing = -1;
	m_rx.nextAck = 0;
}

BaseConnection::~BaseConnection(void)
{
	while (!m_tx.queue.empty())
	{
		delete m_tx.queue.popBegin();
	}
}

int BaseConnection::getCHID() const
{
	return m_chid;
}

int BaseConnection::getCID() const
{
	return m_cid;
}


void BaseConnection::sendUnregister()
{
	Message unregister(protocol::MSG_UNREGISTER, m_cid, isConnected());

	assert (!m_reasonType.empty());
	unregister.writeUnsafeSmallString(m_reasonType);
	unregister.writeUnsafeSmallString(m_reasonDesc);
	sendMessage(unregister);
}

bool BaseConnection::changeState(State newState)
{
	State oldState = m_state;

	switch (oldState)
	{
	case Disconnected: 
		switch (newState)
		{
		case Connecting:
			break;

		case Connected:
		default: 
			return false;
		}
		break;

	case Connected:
		switch (newState)
		{
		case Disconnected: 
			break;
		default: 
			return false;
		}
		break;
    
    default:
        break;
	}

	m_state = newState;
	
	switch (m_state)
	{

	case Connecting:
		{
            sonar::common::Log("sonar: changeState: m_state=Connecting");

			m_tx.nextResend = -1;
			m_tx.resendCount = 0;
			m_rx.nextAck = random(0);
			m_rx.prevAck = 0;
			m_tx.nextSeq = 0;
			onConnecting();
			break;
		}

	case Connected:
		{
            sonar::common::Log("sonar: changeState: m_state=Connected");

			m_remoteClosed = false;
			m_timers.nextPing = m_time->getMSECTime() + m_nextPingOffset + m_options.clientPingInterval;
            onConnected();
			break;
		}

	case Disconnected:
		{
            sonar::common::Log("sonar: changeState: m_state=Disconnected, m_remoteClosed=%d, m_reasonType=%s, m_reasonDesc=%s", m_remoteClosed, m_reasonType.c_str(), m_reasonDesc.c_str());

			m_timers.nextPing = -1;

			if (!m_remoteClosed)
				sendUnregister();

			while (!m_tx.queue.empty())
			{
				delete m_tx.queue.popBegin();
			}

			onDisconnected(m_reasonType, m_reasonDesc);
			return true;
		}

    default:
        break;
	}

	return true;
}

BaseConnection::State BaseConnection::getState()
{
	return m_state;
}

BaseConnection::Result BaseConnection::disconnect(sonar::CString &reasonType, sonar::CString &reasonDesc)
{
	m_reasonType = reasonType;
	m_reasonDesc = reasonDesc;

	SYSLOGX(LOG_INFO, "reasonType=%s, reasonDesc=%s", reasonType.c_str(), reasonDesc.c_str());

	if (getState() == Disconnected)
	{
		SYSLOGX(LOG_ERR, "getState() == Disconnected");
		return InvalidState;
	}

	if (!changeState(Disconnected))
	{
		SYSLOGX(LOG_ERR, "!changeState(Disconnected)");
		return InvalidState;
	}

	return Success;
}

void BaseConnection::dbgForceState(State state)
{
	m_state = state;
}

void BaseConnection::sendUnreliableMessage(Message &message, UpdateSourceOption option)
{
    if (option == UPDATE_SOURCE)
    {
	    message.getHeader().source = (UINT16) getCID();
    }
	onMetricEvent(SendPacket);
	socketSendMessage(message);

    if (ENABLE_CONNECTION_TRACE)
        LOGGER_PRINTF("Sending cmd=%d cid=%d seq=%04x\n", 
            message.getHeader().cmd, message.getHeader().source, message.getSequence());
}

void BaseConnection::popTxQueue()
{
    if (ENABLE_CONNECTION_TRACE)
	    LOGGER_PRINTF("Popping queue\n");

	Message *delMessage = m_tx.queue.popBegin();
	delete delMessage;

	if (m_tx.queue.empty())
	{
        if (ENABLE_CONNECTION_TRACE)
            LOGGER_PRINTF("Nothing left in queue\n");
		return;
	}

	// Send packet in queue
	sendTxQueue();
}

void BaseConnection::sendTxQueue()
{
	Message *message = m_tx.queue.begin();
	m_tx.resendCount = 0;
	m_tx.nextResend = m_time->getMSECTime() + protocol::RESEND_TIMEOUT_MSEC;
	message->updateSequence(m_tx.nextSeq);
	
	message->getHeader().source = (UINT16) getCID();
    if (ENABLE_CONNECTION_TRACE)
        LOGGER_PRINTF("Sending cmd=%d cid=%d seq=%04x\n",
            message->getHeader().cmd, message->getHeader().source, message->getSequence());

	onMetricEvent(SendPacket);
	onMetricEvent(SendReliable);
	socketSendMessage(*message);
}

void BaseConnection::sendReliableMessage (const Message &_message)
{
	Message *message = new Message(_message);

	assert (!m_tx.queue.isFull());

	if (!m_tx.queue.pushEnd (message))
	{
		delete message;
		return;
	}

	if (m_tx.queue.size () == 1)
	{
		sendTxQueue();
	}
	else
	{
        if (ENABLE_CONNECTION_TRACE)
            LOGGER_PRINTF("Enqueuing, cid=%d, size=%d\n", m_time->getMSECTime(), this, m_cid, m_tx.queue.size());
	}
}

void BaseConnection::sendMessage(Message &msg, UpdateSourceOption option)
{
	if (msg.isReliable())
	{
		sendReliableMessage(msg);
		return;
	}

	// Unreliables
	sendUnreliableMessage(msg, option);
}

void BaseConnection::sendAck(UINT8 commandBeingAcked)
{
	Message ack;

	ack.writeHeader(protocol::PT_ACK, 0, m_cid, isClientOnlyJitterBufferAware());
	protocol::MessageHeader &header = ack.getHeader();
	header.seq = m_rx.prevAck;
	ack.writeUnsafeLong(m_rx.nextAck);

    if (ENABLE_CONNECTION_TRACE) {
        LOGGER_PRINTF("Ack to %d prev=%04x, next=%04x, cid=%d\n",
            commandBeingAcked, m_rx.prevAck, m_rx.nextAck, m_cid);
    }

	onMetricEvent(SendPacket);
	socketSendMessage(ack);
}

void BaseConnection::processRelMessage(Message &message)
{
    if (ENABLE_CONNECTION_TRACE) {
        LOGGER_PRINTF("Processing reliable message cmd=%d seq=%04x\n",
            message.getHeader().cmd, message.getHeader().seq);
    }

	unsigned long seq = message.getSequence();

	if (seq != m_rx.nextAck)
	{
		onMetricEvent(DropUnexpectedSeq);
        sonar::common::Log("sonar: processRelMessage DropUnexpectedSeq: msg=%s", cmdToString(message.getHeader().cmd).c_str());

		LOGGER_PRINTF("Unexpected seq=%04x, expected %04x, cmd=%d, cid=%d\n",
            seq, m_rx.nextAck, message.getHeader().cmd, m_cid);
		sendAck(message.getHeader().cmd);
		return;
	}

	m_rx.prevAck = m_rx.nextAck;
	m_rx.nextAck = random(m_rx.prevAck);

	handleMessage(message);
    sendAck(message.getHeader().cmd);
}

void BaseConnection::handleUnregister(Message &message)
{
	m_reasonType = message.readSmallString();
	m_reasonDesc = message.readSmallString();
	m_remoteClosed = true;
	changeState(Disconnected);
}


void BaseConnection::handleMessage(Message &message)
{
    if (ENABLE_CONNECTION_TRACE) {
        LOGGER_PRINTF("Got message %d with sequence %04x\n",
            message.getHeader().cmd, message.getSequence());
    }

	switch (message.getHeader().cmd)
	{
	case protocol::MSG_PING:
		handlePing(message);
		break;

	case protocol::MSG_UNREGISTER:
		handleUnregister(message);
		break;

	default:
		onMetricEvent(DropUnexpectedMsg);
		LOGGER_PRINTF("Unexpected message %d received\n", (int) message.getHeader().cmd);
		break;
	}
}


void BaseConnection::processUnrelMessage(Message &message)
{
	handleMessage(message);
}

void BaseConnection::processAck(Message &message)
{
	if (message.getBytesLeft() < sizeof(protocol::SEQUENCE))
	{
		return;
	}

	if (m_tx.queue.empty())
	{
		onMetricEvent(AckUnexpectedSeq);
		LOGGER_PRINTF("Unexpected sequence received %08x, tx queue empty\n",
            message.getSequence());
		return;
	}

	if (message.getSequence() != m_tx.queue.begin()->getSequence())
	{
		onMetricEvent(AckUnexpectedSeq);
		LOGGER_PRINTF("Unexpected sequence received %08x, expected %08x\n", 
            message.getSequence(), m_tx.queue.begin()->getSequence());
		return;
	}

    unsigned long longData;
    if (!message.readSafeLong(longData))
    {
        LOGGER_PRINTF("Attempting to read %d byte(s). %d byte(s) available.\n", sizeof(longData), message.getBytesLeft());
        return;
    }
    m_tx.nextSeq = longData;

    if (ENABLE_CONNECTION_TRACE) {
        LOGGER_PRINTF("Received ack seq=%08x, next=%08x\n",
            message.getSequence(), m_tx.nextSeq);
    }
	popTxQueue();
	return;
}

void BaseConnection::postMessage(Message &message)
{
	assert (getState() != Disconnected);

	if (getState() == Disconnected)
	{
		onMetricEvent(DropBadState);
        sonar::common::Log("sonar: postMessage DropBadState");
		return;
	}

	if (!message.pullHeader())
	{
		onMetricEvent(DropBadHeader);
        sonar::common::Log("sonar: postMessage DropBadHeader");
		return;
	}

	protocol::MessageHeader &header = message.getHeader();
	protocol::PacketType packetType = (protocol::PacketType) ((header.flags & protocol::PACKETTYPE_MASK) >> protocol::PACKETTYPE_SHIFT);

	if (!validatePacket(message))
	{
		return;
	}

	switch (packetType)
	{
	case protocol::PT_SEGREL:
		processRelMessage (message);
		return;

	case protocol::PT_SEGUNREL:
		//Note: Unreliable segments must always have both hasMsgStart and hasMsgEnd
		processUnrelMessage (message);
		return;

	case protocol::PT_ACK:
		processAck (message);
		return;

	case protocol::PT_NULL:
		return;

	}

	return;
}

void BaseConnection::handlePing(Message &message)
{
}

CString BaseConnection::cmdToString(UINT8 cmd)
{
    switch(cmd)
    {
    case protocol::MSG_REGISTER:
        return "MSG_REGISTER";
        break;
    case protocol::MSG_CHALLENGE:
        return "MSG_CHALLENGE";
        break;
    case protocol::MSG_REGISTERREPLY:
        return "MSG_REGISTERREPLY";
        break;
    case protocol::MSG_UNREGISTER:
        return "MSG_UNREGISTER";
        break;
    case protocol::MSG_PEERLIST_FRAGMENT:
        return "MSG_PEERLIST_FRAGMENT";
        break;
    case protocol::MSG_PING:
        return "MSG_PING";
        break;
    case protocol::MSG_ECHO:
        return "MSG_ECHO";
        break;
    case protocol::MSG_AUDIO_TO_SERVER:
        return "MSG_AUDIO_TO_SERVER";
        break;
    case protocol::MSG_AUDIO_TO_CLIENT:
        return "MSG_AUDIO_TO_CLIENT";
        break;
    case protocol::MSG_AUDIO_STOP:
        return "MSG_AUDIO_STOP";
        break;
    case protocol::MSG_CHANNEL_JOINNOTIF:
        return "MSG_CHANNEL_JOINNOTIF";
        break;
    case protocol::MSG_CHANNEL_PARTNOTIF:
        return "MSG_CHANNEL_PARTNOTIF";
        break;
    case protocol::MSG_CHANNEL_INACTIVITY:
        return "MSG_CHANNEL_INACTIVITY";
        break;
    case protocol::MSG_BLOCK_LIST_COMPLETE:
        return "MSG_BLOCK_LIST_COMPLETE";
        break;
    case protocol::MSG_BLOCK_LIST_FRAGMENT:
        return "MSG_BLOCK_LIST_FRAGMENT";
        break;
    case protocol::MSG_AUDIO_END:
        return "MSG_AUDIO_END";
        break;
    case protocol::MSG_NETWORK_PING:
        return "MSG_NETWORK_PING";
        break;
    case protocol::MSG_NETWORK_PING_REPLY:
        return "MSG_NETWORK_PING_REPLY";
        break;
    }

    return "";
}

void BaseConnection::setDisconnectReason(sonar::String &reasonType, sonar::String &reasonDesc, sonar::CString cmd)
{
    reasonType = "LOCAL_CONNECTION_LOST";
    reasonDesc = cmd;
}

void BaseConnection::frame()
{
	State state = getState();

	common::Timestamp now = m_time->getMSECTime();

	if (state != Connected)
	{
		return;
	}

	if ( m_timers.nextPing != -1 && now > m_timers.nextPing)
	{
		m_timers.nextPing = now + m_options.clientPingInterval;
		Message ping(protocol::MSG_PING, m_cid, true);
		ping.writeUnsafeByte(0xea);
		sendMessage(ping);
	}

	if (!m_tx.queue.empty())
	{
		Message *message = m_tx.queue.begin();

		if (now < m_tx.nextResend)
		{
			return;
		}

		if (m_tx.resendCount >= protocol::RESEND_COUNT)
		{
            setDisconnectReason(m_reasonType, m_reasonDesc, cmdToString(message->getHeader().cmd));
			changeState(Disconnected);
			return;
		}

		m_tx.nextResend = now + protocol::RESEND_TIMEOUT_MSEC;
		m_tx.resendCount ++;

		LOGGER_PRINTF("Resending %08x\n", message->getSequence());

		onMetricEvent(Resend);
		onMetricEvent(SendPacket);
		socketSendMessage(*message);
	}
}

size_t BaseConnection::getTxQueueLength() const
{
	return m_tx.queue.size();
}

CString &BaseConnection::getUserId() const
{
	return m_userId;
}

CString &BaseConnection::getUserDesc() const
{
	return m_userDesc;
}

void BaseConnection::setCID(int cid)
{
	m_cid = cid;
}

void BaseConnection::setCHID(int chid)
{
	m_chid = chid;
}

void BaseConnection::setUserId(CString &userId)
{
	m_userId = userId;
}

void BaseConnection::setUserDesc(CString &userDesc)
{
	m_userDesc = userDesc;
}

}
