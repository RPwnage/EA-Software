// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/Channel.h>
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

Channel::Channel(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.Channel"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_channelName(m_allocator_.emptyString())
, m_type(1)
{
}

void Channel::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_channelName.clear();
    m_type = 1;
}

size_t Channel::getByteSize()
{
    size_t total_size = 0;
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    if (!m_channelName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelName);
    }
    if (m_type != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_type);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* Channel::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    if (!getChannelName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getChannelName(), target);
    }
    if (m_type != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(3, static_cast<int32_t>(getType()), target);
    }
    return target;
}

bool Channel::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(18)) goto parse_channelName;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_channelName:
                    if (!input->readString(&m_channelName))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(24)) goto parse_type;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_type:
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setType(static_cast<com::ea::eadp::antelope::rtm::protocol::ChannelType>(value));
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
::eadp::foundation::String Channel::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (!m_channelName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelName: \"%s\",\n", indent.c_str(), m_channelName.c_str());
    }
    if (m_type != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  type: %d,\n", indent.c_str(), m_type);
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& Channel::getChannelId() const
{
    return m_channelId;
}

void Channel::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void Channel::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& Channel::getChannelName() const
{
    return m_channelName;
}

void Channel::setChannelName(const ::eadp::foundation::String& value)
{
    m_channelName = value;
}

void Channel::setChannelName(const char8_t* value)
{
    m_channelName = m_allocator_.make<::eadp::foundation::String>(value);
}


com::ea::eadp::antelope::rtm::protocol::ChannelType Channel::getType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::ChannelType>(m_type);
}
void Channel::setType(com::ea::eadp::antelope::rtm::protocol::ChannelType value)
{
    m_type = static_cast<int>(value);
}

}
}
}
}
}
