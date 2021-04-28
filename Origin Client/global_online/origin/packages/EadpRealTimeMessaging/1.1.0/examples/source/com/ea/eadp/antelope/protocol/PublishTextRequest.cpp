// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/PublishTextRequest.h>
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

PublishTextRequest::PublishTextRequest(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.PublishTextRequest"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_text(m_allocator_.emptyString())
, m_messageType(1)
, m_index(0)
{
}

void PublishTextRequest::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_text.clear();
    m_messageType = 1;
    m_index = 0;
}

size_t PublishTextRequest::getByteSize()
{
    size_t total_size = 0;
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    if (!m_text.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_text);
    }
    if (m_messageType != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_messageType);
    }
    if (m_index != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_index);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* PublishTextRequest::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    if (!getText().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getText(), target);
    }
    if (m_messageType != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(3, static_cast<int32_t>(getMessageType()), target);
    }
    if (getIndex() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(4, getIndex(), target);
    }
    return target;
}

bool PublishTextRequest::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(18)) goto parse_text;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_text:
                    if (!input->readString(&m_text))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(24)) goto parse_messageType;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_messageType:
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setMessageType(static_cast<com::ea::eadp::antelope::rtm::protocol::MessageType>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(32)) goto parse_index;
                break;
            }

            case 4:
            {
                if (tag == 32)
                {
                    parse_index:
                    if (!input->readInt32(&m_index))
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
::eadp::foundation::String PublishTextRequest::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (!m_text.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  text: \"%s\",\n", indent.c_str(), m_text.c_str());
    }
    if (m_messageType != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  messageType: %d,\n", indent.c_str(), m_messageType);
    }
    if (m_index != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  index: %d,\n", indent.c_str(), m_index);
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& PublishTextRequest::getChannelId() const
{
    return m_channelId;
}

void PublishTextRequest::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void PublishTextRequest::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& PublishTextRequest::getText() const
{
    return m_text;
}

void PublishTextRequest::setText(const ::eadp::foundation::String& value)
{
    m_text = value;
}

void PublishTextRequest::setText(const char8_t* value)
{
    m_text = m_allocator_.make<::eadp::foundation::String>(value);
}


com::ea::eadp::antelope::rtm::protocol::MessageType PublishTextRequest::getMessageType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::MessageType>(m_messageType);
}
void PublishTextRequest::setMessageType(com::ea::eadp::antelope::rtm::protocol::MessageType value)
{
    m_messageType = static_cast<int>(value);
}

int32_t PublishTextRequest::getIndex() const
{
    return m_index;
}

void PublishTextRequest::setIndex(int32_t value)
{
    m_index = value;
}


}
}
}
}
}
