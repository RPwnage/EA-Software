#pragma once

#include <SonarCommon/Common.h>

namespace sonar
{

class ClientConnection;
class Channel;

class Operator
{
public:
	typedef SonarMap<String, ClientConnection *> UserIdToClientMap;
	typedef SonarMap<String, Channel *> ChannelIdToChannelMap;

	Operator(CString &id);

	Channel *getChannel(CString &channelId);
	ClientConnection *getClient(CString &userId);

	void mapChannel(Channel *channel); 
	void unmapChannel(Channel *channel);

	void mapClient(ClientConnection *client); 
	void unmapClient(ClientConnection *client);

	const UserIdToClientMap &getUserMap(void);
	const ChannelIdToChannelMap &getChannelMap(void);

	CString &getId();


private:
	String m_id;
	UserIdToClientMap m_userIdMap;
	ChannelIdToChannelMap m_channelIdMap;
};

}