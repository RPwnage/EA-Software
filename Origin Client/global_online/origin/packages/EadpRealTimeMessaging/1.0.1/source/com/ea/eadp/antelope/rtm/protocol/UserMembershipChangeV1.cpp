// GENERATED CODE (Source: notification_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/UserMembershipChangeV1.h>
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

UserMembershipChangeV1::UserMembershipChangeV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.UserMembershipChangeV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_type(1)
, m_channel(nullptr)
{
}

void UserMembershipChangeV1::clear()
{
    m_cachedByteSize_ = 0;
    m_type = 1;
    m_channel.reset();
}

size_t UserMembershipChangeV1::getByteSize()
{
    size_t total_size = 0;
    if (m_type != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_type);
    }
    if (m_channel != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_channel);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* UserMembershipChangeV1::onSerialize(uint8_t* target) const
{
    if (m_type != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(1, static_cast<int32_t>(getType()), target);
    }
    if (m_channel != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_channel, target);
    }
    return target;
}

bool UserMembershipChangeV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(18)) goto parse_channel;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_channel:
                    if (!input->readMessage(getChannel().get())) return false;
                } else {
                    goto handle_unusual;
                }
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
::eadp::foundation::String UserMembershipChangeV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_type != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  type: %d,\n", indent.c_str(), m_type);
    }
    if (m_channel != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channel: %s,\n", indent.c_str(), m_channel->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeType UserMembershipChangeV1::getType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeType>(m_type);
}
void UserMembershipChangeV1::setType(com::ea::eadp::antelope::rtm::protocol::ChannelMembershipChangeType value)
{
    m_type = static_cast<int>(value);
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelV1> UserMembershipChangeV1::getChannel()
{
    if (m_channel == nullptr)
    {
        m_channel = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChannelV1>(m_allocator_);
    }
    return m_channel;
}

void UserMembershipChangeV1::setChannel(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelV1> val)
{
    m_channel = val;
}

bool UserMembershipChangeV1::hasChannel()
{
    return m_channel != nullptr;
}

void UserMembershipChangeV1::releaseChannel()
{
    m_channel.reset();
}


}
}
}
}
}
}
