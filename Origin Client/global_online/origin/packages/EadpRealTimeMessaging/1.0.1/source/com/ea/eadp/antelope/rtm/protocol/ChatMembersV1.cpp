// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChatMembersV1.h>
#include <eadp/foundation/internal/ProtobufWireFormat.h>
#include <eadp/foundation/internal/Utils.h>
#include <eadp/foundation/Hub.h>

namespace com
{

namespace ea
{

namespace eadp
{

namespace antelope
{

namespace rtm
{

namespace protocol
{

ChatMembersV1::ChatMembersV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChatMembersV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_users(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1>>>())
, m_totalMemberCount(0)
, m_players(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo>>>())
{
}

void ChatMembersV1::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_users.clear();
    m_totalMemberCount = 0;
    m_players.clear();
}

size_t ChatMembersV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    total_size += 1 * m_users.size();
    for (EADPSDK_SIZE_T i = 0; i < m_users.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_users[i]);
    }
    if (m_totalMemberCount != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_totalMemberCount);
    }
    total_size += 1 * m_players.size();
    for (EADPSDK_SIZE_T i = 0; i < m_players.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_players[i]);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChatMembersV1::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_users.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_users[i], target);
    }
    if (getTotalMemberCount() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(3, getTotalMemberCount(), target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_players.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(4, *m_players[i], target);
    }
    return target;
}

bool ChatMembersV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
{
    clear();
    uint32_t tag;
    for (;;)
    {
        tag = input->readTag();
        if (tag > 127) goto handle_unusual;
        switch (::eadp::foundation::Internal::ProtobufWireFormat::getTagFieldNumber(tag))
        {
            case 1:
            {
                if (tag == 10)
                {
                    if (!input->readString(&m_channelId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_users;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_users:
                    parse_loop_users:
                    auto tmpUsers = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PersonaV1>(m_allocator_);
                    if (!input->readMessage(tmpUsers.get()))
                    {
                        return false;
                    }
                    m_users.push_back(tmpUsers);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_loop_users;
                if (input->expectTag(24)) goto parse_totalMemberCount;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_totalMemberCount:
                    if (!input->readInt32(&m_totalMemberCount))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_players;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_players:
                    parse_loop_players:
                    auto tmpPlayers = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PlayerInfo>(m_allocator_);
                    if (!input->readMessage(tmpPlayers.get()))
                    {
                        return false;
                    }
                    m_players.push_back(tmpPlayers);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_loop_players;
                if (input->expectAtEnd()) return true;
                break;
            }

            default: {
            handle_unusual:
                if (tag == 0)
                {
                    return true;
                }
                if(!input->skipField(tag)) return false;
                break;
            }
        }
    }
}
::eadp::foundation::String ChatMembersV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (m_users.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  users: [\n", indent.c_str());
        for (auto iter = m_users.begin(); iter != m_users.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    if (m_totalMemberCount != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  totalMemberCount: %d,\n", indent.c_str(), m_totalMemberCount);
    }
    if (m_players.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  players: [\n", indent.c_str());
        for (auto iter = m_players.begin(); iter != m_players.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& ChatMembersV1::getChannelId() const
{
    return m_channelId;
}

void ChatMembersV1::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void ChatMembersV1::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1>>& ChatMembersV1::getUsers()
{
    return m_users;
}

int32_t ChatMembersV1::getTotalMemberCount() const
{
    return m_totalMemberCount;
}

void ChatMembersV1::setTotalMemberCount(int32_t value)
{
    m_totalMemberCount = value;
}


::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo>>& ChatMembersV1::getPlayers()
{
    return m_players;
}

}
}
}
}
}
}
