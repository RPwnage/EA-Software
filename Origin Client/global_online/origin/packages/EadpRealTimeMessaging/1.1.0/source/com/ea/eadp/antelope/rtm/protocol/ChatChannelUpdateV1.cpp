// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChatChannelUpdateV1.h>
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

ChatChannelUpdateV1::ChatChannelUpdateV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChatChannelUpdateV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_lastReadTimestamp(m_allocator_.emptyString())
, m_customProperties(m_allocator_.emptyString())
{
}

void ChatChannelUpdateV1::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_lastReadTimestamp.clear();
    m_customProperties.clear();
}

size_t ChatChannelUpdateV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    if (!m_lastReadTimestamp.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_lastReadTimestamp);
    }
    if (!m_customProperties.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_customProperties);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChatChannelUpdateV1::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    if (!getLastReadTimestamp().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getLastReadTimestamp(), target);
    }
    if (!getCustomProperties().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getCustomProperties(), target);
    }
    return target;
}

bool ChatChannelUpdateV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(18)) goto parse_lastReadTimestamp;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_lastReadTimestamp:
                    if (!input->readString(&m_lastReadTimestamp))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_customProperties;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_customProperties:
                    if (!input->readString(&m_customProperties))
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
::eadp::foundation::String ChatChannelUpdateV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (!m_lastReadTimestamp.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  lastReadTimestamp: \"%s\",\n", indent.c_str(), m_lastReadTimestamp.c_str());
    }
    if (!m_customProperties.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  customProperties: \"%s\",\n", indent.c_str(), m_customProperties.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& ChatChannelUpdateV1::getChannelId() const
{
    return m_channelId;
}

void ChatChannelUpdateV1::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void ChatChannelUpdateV1::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& ChatChannelUpdateV1::getLastReadTimestamp() const
{
    return m_lastReadTimestamp;
}

void ChatChannelUpdateV1::setLastReadTimestamp(const ::eadp::foundation::String& value)
{
    m_lastReadTimestamp = value;
}

void ChatChannelUpdateV1::setLastReadTimestamp(const char8_t* value)
{
    m_lastReadTimestamp = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& ChatChannelUpdateV1::getCustomProperties() const
{
    return m_customProperties;
}

void ChatChannelUpdateV1::setCustomProperties(const ::eadp::foundation::String& value)
{
    m_customProperties = value;
}

void ChatChannelUpdateV1::setCustomProperties(const char8_t* value)
{
    m_customProperties = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
