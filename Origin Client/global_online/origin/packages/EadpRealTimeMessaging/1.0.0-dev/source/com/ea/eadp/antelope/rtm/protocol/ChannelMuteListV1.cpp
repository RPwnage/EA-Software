// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChannelMuteListV1.h>
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

ChannelMuteListV1::ChannelMuteListV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChannelMuteListV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_mutedUsers(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1>>>())
, m_mutedBy(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::MutedSetV1>>>())
, m_mutedPlayers(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player>>>())
{
}

void ChannelMuteListV1::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_mutedUsers.clear();
    m_mutedBy.clear();
    m_mutedPlayers.clear();
}

size_t ChannelMuteListV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    total_size += 1 * m_mutedUsers.size();
    for (EADPSDK_SIZE_T i = 0; i < m_mutedUsers.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_mutedUsers[i]);
    }
    total_size += 1 * m_mutedBy.size();
    for (EADPSDK_SIZE_T i = 0; i < m_mutedBy.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_mutedBy[i]);
    }
    total_size += 1 * m_mutedPlayers.size();
    for (EADPSDK_SIZE_T i = 0; i < m_mutedPlayers.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_mutedPlayers[i]);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChannelMuteListV1::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_mutedUsers.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_mutedUsers[i], target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_mutedBy.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_mutedBy[i], target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_mutedPlayers.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(4, *m_mutedPlayers[i], target);
    }
    return target;
}

bool ChannelMuteListV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(18)) goto parse_mutedUsers;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_mutedUsers:
                    parse_loop_mutedUsers:
                    auto tmpMutedUsers = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PersonaV1>(m_allocator_);
                    if (!input->readMessage(tmpMutedUsers.get()))
                    {
                        return false;
                    }
                    m_mutedUsers.push_back(tmpMutedUsers);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_loop_mutedUsers;
                if (input->expectTag(26)) goto parse_loop_mutedBy;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_loop_mutedBy:
                    auto tmpMutedBy = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::MutedSetV1>(m_allocator_);
                    if (!input->readMessage(tmpMutedBy.get()))
                    {
                        return false;
                    }
                    m_mutedBy.push_back(tmpMutedBy);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_loop_mutedBy;
                if (input->expectTag(34)) goto parse_loop_mutedPlayers;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_loop_mutedPlayers:
                    auto tmpMutedPlayers = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::Player>(m_allocator_);
                    if (!input->readMessage(tmpMutedPlayers.get()))
                    {
                        return false;
                    }
                    m_mutedPlayers.push_back(tmpMutedPlayers);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_loop_mutedPlayers;
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
::eadp::foundation::String ChannelMuteListV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (m_mutedUsers.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  mutedUsers: [\n", indent.c_str());
        for (auto iter = m_mutedUsers.begin(); iter != m_mutedUsers.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    if (m_mutedBy.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  mutedBy: [\n", indent.c_str());
        for (auto iter = m_mutedBy.begin(); iter != m_mutedBy.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    if (m_mutedPlayers.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  mutedPlayers: [\n", indent.c_str());
        for (auto iter = m_mutedPlayers.begin(); iter != m_mutedPlayers.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& ChannelMuteListV1::getChannelId() const
{
    return m_channelId;
}

void ChannelMuteListV1::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void ChannelMuteListV1::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1>>& ChannelMuteListV1::getMutedUsers()
{
    return m_mutedUsers;
}

::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::MutedSetV1>>& ChannelMuteListV1::getMutedBy()
{
    return m_mutedBy;
}

::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player>>& ChannelMuteListV1::getMutedPlayers()
{
    return m_mutedPlayers;
}

}
}
}
}
}
}
