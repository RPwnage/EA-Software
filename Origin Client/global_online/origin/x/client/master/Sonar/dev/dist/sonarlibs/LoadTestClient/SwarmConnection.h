#pragma once

#include <SonarConnection/common.h>
#include <SonarConnection/Connection.h>
#include "Swarm.h"

class Swarm;
class sonar::TokenSigner;
struct Swarm::Config;

namespace sonar
{
	class TokenSigner;
}

class SwarmConnection : public sonar::Connection, private sonar::IConnectionEvents, public sonar::IBlockedListProvider
{
public:
	SwarmConnection(
		int userId,
		int channelId,
		sonar::CString &serverAddress,
		int serverPort,
		long long connectDelay,
		Swarm::Config &config,
		Swarm *swarm,
		sonar::TokenSigner *tokenSigner,
		int nextPingOffset );
	~SwarmConnection (void);

	void poll(long long time, long long frame);
	void drop(void);

    // IBlockedListProvider interface
    virtual const sonar::BlockedList& blockList() const;
    virtual bool hasChanges() const;
    virtual void clearChanges();

    void addBlockedUser(char* userIdString);

    long long disconnectTime() const { return m_disconnectTime; }

private:
	//sonar::IConnectionEvents
	virtual void onConnect(sonar::CString &channelId, sonar::CString &channelDesc);
	virtual void onDisconnect(sonar::CString &reasonType, sonar::CString &reasonDesc);
	virtual void onClientJoined(int chid, sonar::CString &userId, sonar::CString &userDesc) { }
	virtual void onClientParted(int chid, sonar::CString &userId, sonar::CString &userDesc, sonar::CString &reasonType, sonar::CString &reasonDesc) { }
	virtual void onTakeBegin() { }
	virtual void onFrameBegin(long long timestamp) { }
	virtual void onFrameClientPayload(int chid, const void *payload, size_t cbPayload) { }
	virtual void onFrameEnd(long long timestamp = 0) { }
	virtual void onTakeEnd() { }
	virtual void onEchoMessage(const sonar::Message &message) { }
    virtual void onChannelInactivity(unsigned long interval) { }

	void generateToken(void);

private:
	sonar::String m_tokenString;
	long long m_talkModStart;
	long long m_talkStart;
	long long m_talkEnd;

	long long m_connectTime;
	long long m_disconnectTime;
		
	Swarm *m_swarm;
	Swarm::Config &m_config;

	int m_userId;
	int m_channelId;
	sonar::String m_serverAddress;
	int m_serverPort;
	sonar::TokenSigner *m_tokenSigner;

    // blocked list
    bool mBlockListHasChanged;
    sonar::BlockedList mBlockedList;
};
