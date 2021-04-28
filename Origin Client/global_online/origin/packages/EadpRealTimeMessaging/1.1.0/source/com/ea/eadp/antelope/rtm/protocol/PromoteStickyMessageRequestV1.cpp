// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/PromoteStickyMessageRequestV1.h>
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

PromoteStickyMessageRequestV1::PromoteStickyMessageRequestV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.PromoteStickyMessageRequestV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_messageId(m_allocator_.emptyString())
, m_timestamp(m_allocator_.emptyString())
, m_index(0)
{
}

void PromoteStickyMessageRequestV1::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_messageId.clear();
    m_timestamp.clear();
    m_index = 0;
}

size_t PromoteStickyMessageRequestV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    if (!m_messageId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_messageId);
    }
    if (!m_timestamp.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_timestamp);
    }
    if (m_index != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_index);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* PromoteStickyMessageRequestV1::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    if (!getMessageId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getMessageId(), target);
    }
    if (!getTimestamp().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getTimestamp(), target);
    }
    if (getIndex() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(4, getIndex(), target);
    }
    return target;
}

bool PromoteStickyMessageRequestV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(18)) goto parse_messageId;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_messageId:
                    if (!input->readString(&m_messageId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_timestamp;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_timestamp:
                    if (!input->readString(&m_timestamp))
                    {
                        return false;
                    }
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
::eadp::foundation::String PromoteStickyMessageRequestV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (!m_messageId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  messageId: \"%s\",\n", indent.c_str(), m_messageId.c_str());
    }
    if (!m_timestamp.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  timestamp: \"%s\",\n", indent.c_str(), m_timestamp.c_str());
    }
    if (m_index != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  index: %d,\n", indent.c_str(), m_index);
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& PromoteStickyMessageRequestV1::getChannelId() const
{
    return m_channelId;
}

void PromoteStickyMessageRequestV1::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void PromoteStickyMessageRequestV1::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& PromoteStickyMessageRequestV1::getMessageId() const
{
    return m_messageId;
}

void PromoteStickyMessageRequestV1::setMessageId(const ::eadp::foundation::String& value)
{
    m_messageId = value;
}

void PromoteStickyMessageRequestV1::setMessageId(const char8_t* value)
{
    m_messageId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& PromoteStickyMessageRequestV1::getTimestamp() const
{
    return m_timestamp;
}

void PromoteStickyMessageRequestV1::setTimestamp(const ::eadp::foundation::String& value)
{
    m_timestamp = value;
}

void PromoteStickyMessageRequestV1::setTimestamp(const char8_t* value)
{
    m_timestamp = m_allocator_.make<::eadp::foundation::String>(value);
}


int32_t PromoteStickyMessageRequestV1::getIndex() const
{
    return m_index;
}

void PromoteStickyMessageRequestV1::setIndex(int32_t value)
{
    m_index = value;
}


}
}
}
}
}
}
