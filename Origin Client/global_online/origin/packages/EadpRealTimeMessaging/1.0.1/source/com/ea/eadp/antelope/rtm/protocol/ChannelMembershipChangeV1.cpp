// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChannelMembershipChangeV1.h>
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

ChannelMembershipChangeV1::ChannelMembershipChangeV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChannelMembershipChangeV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_type(1)
, m_users(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1>>>())
, m_players(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo>>>())
{
}

void ChannelMembershipChangeV1::clear()
{
    m_cachedByteSize_ = 0;
    m_type = 1;
    m_users.clear();
    m_players.clear();
}

size_t ChannelMembershipChangeV1::getByteSize()
{
    size_t total_size = 0;
    if (m_type != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_type);
    }
    total_size += 1 * m_users.size();
    for (EADPSDK_SIZE_T i = 0; i < m_users.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_users[i]);
    }
    total_size += 1 * m_players.size();
    for (EADPSDK_SIZE_T i = 0; i < m_players.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_players[i]);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChannelMembershipChangeV1::onSerialize(uint8_t* target) const
{
    if (m_type != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(1, static_cast<int32_t>(getType()), target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_users.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_users[i], target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_players.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_players[i], target);
    }
    return target;
}

bool ChannelMembershipChangeV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (tag == 8)
                {
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setType(static_cast<com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeType>(value));
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
                if (input->expectTag(26)) goto parse_loop_players;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
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
                if (input->expectTag(26)) goto parse_loop_players;
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
::eadp::foundation::String ChannelMembershipChangeV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_type != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  type: %d,\n", indent.c_str(), m_type);
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

com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeType ChannelMembershipChangeV1::getType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeType>(m_type);
}
void ChannelMembershipChangeV1::setType(com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeType value)
{
    m_type = static_cast<int>(value);
}

::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1>>& ChannelMembershipChangeV1::getUsers()
{
    return m_users;
}

::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo>>& ChannelMembershipChangeV1::getPlayers()
{
    return m_players;
}

}
}
}
}
}
}
