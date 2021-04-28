#include "Operator.h"
#include "Channel.h"

#include <assert.h>

namespace sonar
{

Operator::Operator(CString &id)
	: m_id(id)
{
}

ClientConnection *Operator::getClient(CString &userId)
{
	UserIdToClientMap::iterator iter = m_userIdMap.find(userId);
	if (iter == m_userIdMap.end())
	{
		return NULL;
	}

	return iter->second;
}

void Operator::mapChannel(Channel *channel)
{
	assert(m_channelIdMap.find(channel->getId()) == m_channelIdMap.end());
	m_channelIdMap[channel->getId()] = channel;
}

void Operator::unmapChannel(Channel *channel)
{
	assert (channel->getClientCount() == 0);
	
	ChannelIdToChannelMap::iterator iter = m_channelIdMap.find(channel->getId());
	assert (iter != m_channelIdMap.end());
	assert (iter->second == channel);
	m_channelIdMap.erase(iter);
}

void Operator::mapClient(ClientConnection *client)
{
	assert (m_userIdMap.find(client->getUserId()) == m_userIdMap.end());
	m_userIdMap[client->getUserId()] = client;
}

void Operator::unmapClient(ClientConnection *client)
{
	assert (m_userIdMap.find(client->getUserId()) != m_userIdMap.end());

	UserIdToClientMap::iterator iter = m_userIdMap.find(client->getUserId());
	assert (iter != m_userIdMap.end());
	assert (iter->second == client);

	m_userIdMap.erase(iter);
}

Channel *Operator::getChannel(CString &channelId)
{
	ChannelIdToChannelMap::iterator iter = m_channelIdMap.find(channelId);

	if (iter == m_channelIdMap.end())
	{
		return NULL;
	}

	return iter->second;
}

const Operator::UserIdToClientMap &Operator::getUserMap(void)
{
	return m_userIdMap;
}

const Operator::ChannelIdToChannelMap &Operator::getChannelMap(void)
{
	return m_channelIdMap;
}

CString &Operator::getId()
{
	return m_id;
}


}