// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/ChannelMuteList.h>
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

namespace protocol
{

ChannelMuteList::ChannelMuteList(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.ChannelMuteList"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_mutedUsers(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::MutedUser>>>())
{
}

void ChannelMuteList::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_mutedUsers.clear();
}

size_t ChannelMuteList::getByteSize()
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
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChannelMuteList::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_mutedUsers.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_mutedUsers[i], target);
    }
    return target;
}

bool ChannelMuteList::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    auto tmpMutedUsers = m_allocator_.makeShared<com::ea::eadp::antelope::protocol::MutedUser>(m_allocator_);
                    if (!input->readMessage(tmpMutedUsers.get()))
                    {
                        return false;
                    }
                    m_mutedUsers.push_back(tmpMutedUsers);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_loop_mutedUsers;
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
::eadp::foundation::String ChannelMuteList::toString(const ::eadp::foundation::String& indent) const
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
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& ChannelMuteList::getChannelId() const
{
    return m_channelId;
}

void ChannelMuteList::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void ChannelMuteList::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::MutedUser>>& ChannelMuteList::getMutedUsers()
{
    return m_mutedUsers;
}

}
}
}
}
}
