#include <assert.h>
#include <stdlib.h>
#include <iomanip>
#include <iostream>

#include "SwarmConnection.h"
#include <SonarClient/win32/providers.h>
#include <TestUtils/TokenSigner.h>
#include "Swarm.h"

static sonar::DefaultTimeProvider s_time;
static sonar::Udp4NetworkProvider s_network;
static sonar::StderrLogger s_logger;

static const long long FRAMES_PER_SECONDS = (1000 / sonar::protocol::FRAME_TIMESLICE_MSEC);
static const long long FRAMES_PER_MINUTE=FRAMES_PER_SECONDS * 60;
static const size_t BYTES_PER_FRAME=40;

#include <string>
#include <sstream>
#include <time.h>

static sonar::String MakeToken(int userId, int channelId, const char *serverAddress, int serverPort, const char *prefix)
{
	sonar::StringStream ss;

	time_t expires = time(0);
	expires += 86400;

	ss << "SONAR2|";
	ss << serverAddress << ":" << serverPort << "|";
	ss << serverAddress << ":" << serverPort << "|";
	ss << "Origin|";
	ss << "u" << prefix << "-" << userId << "|";
	ss << "U" << prefix << "-" << userId << "|";
	ss << "c" << prefix << "-" << channelId << "|";
	ss << "C" << prefix << "-" << channelId << "|";
	ss << "|" << expires << "|";

	return ss.str();
}


SwarmConnection::SwarmConnection(
	int userId,
	int channelId,
	sonar::CString &serverAddress,
	int serverPort,
	long long connectDelay,
	Swarm::Config &config,
	Swarm *swarm,
	sonar::TokenSigner *tokenSigner,
	int nextPingOffset)

	: sonar::Connection(s_time, s_network, s_logger, sonar::DefaultConnectionOptions, nextPingOffset, NULL, this)
	, m_userId(userId)
	, m_channelId(channelId)
	, m_serverPort(serverPort)
	, m_serverAddress(serverAddress)
	, m_talkModStart(-1)
	, m_talkEnd(-1)
	, m_talkStart(-1)
	, m_swarm(swarm)
	, m_config(config)
	, m_tokenSigner(tokenSigner)
{
	m_connectTime = connectDelay;
	generateToken();
	setEventSink(this);

    // blocked list
    mBlockListHasChanged = false;
    mBlockedList.clear();
}

SwarmConnection::~SwarmConnection (void)
{
}

void SwarmConnection::generateToken()
{
	m_tokenString = MakeToken(m_userId, m_channelId, m_serverAddress.c_str(), m_serverPort, m_config.hostPrefix);

	if (m_tokenSigner)
	{
		sonar::Token token;
		token.parse(m_tokenString);
		m_tokenString = m_tokenSigner->sign(token);
	}
}

void SwarmConnection::poll(long long timestamp, long long frame)
{
	Connection::poll();

	if (isDisconnected())
	{
		if (timestamp > m_connectTime)
		{
			if (m_tokenString.empty())
			{
				generateToken();
			}

			if (this->connect(m_tokenString))
				fprintf (stderr, "ERROR: connect method failed\n");
		}

		return;
	}

	if (!isConnected())
	{
		m_tokenString.clear();
		return;
	}

	assert (m_talkModStart != -1);

	if (m_config.talkFrames > 0)
	{

		long long modFrame = frame % 3000LL;

		if (modFrame == m_talkModStart)
		{
			m_talkStart = frame;
			m_talkEnd = frame + m_config.talkFrames;
		}

		if (frame >= m_talkStart && m_talkStart != -1)
		{
			static char payload[] = "TALKTALKTALKTALKTALKTALKTALKTALKTALKTALK";

			assert (sizeof(payload) - 1 == BYTES_PER_FRAME);

			sendAudioPayload(payload, BYTES_PER_FRAME);

			if (frame == m_talkEnd)
			{
				m_talkStart = -1;
				sendAudioStop();
			}
		}
	}

	if (timestamp > m_disconnectTime && m_disconnectTime != -1)
	{
		//if (rand () % 4 == 0)
		//	this->drop();
		//else
        {
            //std::cerr
            //    << std::setw(8) << sonar::common::getTimeAsMSEC()
            //    << " Disconnecting cid=" << getCID()
            //    << std::endl;
    	    this->disconnect("LEAVING", "");
        }
	}

}

void SwarmConnection::onDisconnect(sonar::CString &reasonType, sonar::CString &reasonDesc)
{
	if (m_config.showDisconnectReason /*|| reasonType.find("CONNECTION_LOST") != sonar::CString::npos*/)
		fprintf (stderr, "DISCONNECT: %s/%s\n", reasonType.c_str(), reasonDesc.c_str());

	m_connectTime = m_swarm->getTime() + rand() % (m_config.disconnectBackoff * 1000);
	m_swarm->onDisconnect(this, reasonType, reasonDesc);
}

void SwarmConnection::onConnect(sonar::CString &channelId, sonar::CString &channelDesc)
{
	m_talkModStart = rand () % FRAMES_PER_MINUTE;

	if (m_config.connectionLengthSec > 0)
		m_disconnectTime = m_swarm->getTime() + (rand() % (m_config.connectionLengthSec * 2)) * 1000;
	else
		m_disconnectTime = -1;

	m_swarm->onConnect(this);
}

void SwarmConnection::drop(void)
{
	setCID(0);
	disconnect("LOCAL_CONNECTION_LOST", "drop");
}

const sonar::BlockedList& SwarmConnection::blockList() const
{
    return mBlockedList;
}

bool SwarmConnection::hasChanges() const
{
    return mBlockListHasChanged;
}

void SwarmConnection::clearChanges()
{
    mBlockListHasChanged = false;
}

void SwarmConnection::addBlockedUser(char* userIdString)
{
    mBlockedList.push_back(userIdString);
    mBlockListHasChanged = true;
}
