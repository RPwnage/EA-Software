// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChannelV1.h>
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

ChannelV1::ChannelV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChannelV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_channelName(m_allocator_.emptyString())
, m_type(1)
, m_unreadMessageCount(0)
{
}

void ChannelV1::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_channelName.clear();
    m_type = 1;
    m_unreadMessageCount = 0;
}

size_t ChannelV1::getByteSize()
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
    if (m_unreadMessageCount != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_unreadMessageCount);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChannelV1::onSerialize(uint8_t* target) const
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
    if (getUnreadMessageCount() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(4, getUnreadMessageCount(), target);
    }
    return target;
}

bool ChannelV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(32)) goto parse_unreadMessageCount;
                break;
            }

            case 4:
            {
                if (tag == 32)
                {
                    parse_unreadMessageCount:
                    if (!input->readInt32(&m_unreadMessageCount))
                    {
                        return false;
                    }
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
::eadp::foundation::String ChannelV1::toString(const ::eadp::foundation::String& indent) const
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
    if (m_unreadMessageCount != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  unreadMessageCount: %d,\n", indent.c_str(), m_unreadMessageCount);
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& ChannelV1::getChannelId() const
{
    return m_channelId;
}

void ChannelV1::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void ChannelV1::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& ChannelV1::getChannelName() const
{
    return m_channelName;
}

void ChannelV1::setChannelName(const ::eadp::foundation::String& value)
{
    m_channelName = value;
}

void ChannelV1::setChannelName(const char8_t* value)
{
    m_channelName = m_allocator_.make<::eadp::foundation::String>(value);
}


com::ea::eadp::antelope::rtm::protocol::ChannelType ChannelV1::getType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::ChannelType>(m_type);
}
void ChannelV1::setType(com::ea::eadp::antelope::rtm::protocol::ChannelType value)
{
    m_type = static_cast<int>(value);
}

int32_t ChannelV1::getUnreadMessageCount() const
{
    return m_unreadMessageCount;
}

void ChannelV1::setUnreadMessageCount(int32_t value)
{
    m_unreadMessageCount = value;
}


}
}
}
}
}
}
