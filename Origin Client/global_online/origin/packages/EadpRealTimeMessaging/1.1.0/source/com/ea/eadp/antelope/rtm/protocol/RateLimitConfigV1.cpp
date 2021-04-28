// GENERATED CODE (Source: config_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/RateLimitConfigV1.h>
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

RateLimitConfigV1::RateLimitConfigV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.RateLimitConfigV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelUpdateLimit(0.0)
, m_typingEventLimit(0.0)
{
}

void RateLimitConfigV1::clear()
{
    m_cachedByteSize_ = 0;
    m_channelUpdateLimit = 0.0;
    m_typingEventLimit = 0.0;
}

size_t RateLimitConfigV1::getByteSize()
{
    size_t total_size = 0;
    if (m_channelUpdateLimit != 0.0)
    {
        total_size += 1 + 8;
    }
    if (m_typingEventLimit != 0.0)
    {
        total_size += 1 + 8;
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* RateLimitConfigV1::onSerialize(uint8_t* target) const
{
    if (getChannelUpdateLimit() != 0.0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeDouble(1, getChannelUpdateLimit(), target);
    }
    if (getTypingEventLimit() != 0.0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeDouble(2, getTypingEventLimit(), target);
    }
    return target;
}

bool RateLimitConfigV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (tag == 9)
                {
                    if (!input->readDouble(&m_channelUpdateLimit))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(17)) goto parse_typingEventLimit;
                break;
            }

            case 2:
            {
                if (tag == 17)
                {
                    parse_typingEventLimit:
                    if (!input->readDouble(&m_typingEventLimit))
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
::eadp::foundation::String RateLimitConfigV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_channelUpdateLimit != 0.0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelUpdateLimit: %f,\n", indent.c_str(), m_channelUpdateLimit);
    }
    if (m_typingEventLimit != 0.0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  typingEventLimit: %f,\n", indent.c_str(), m_typingEventLimit);
    }
    result.append(indent);
    result.append("}");
    return result;
}

double RateLimitConfigV1::getChannelUpdateLimit() const
{
    return m_channelUpdateLimit;
}

void RateLimitConfigV1::setChannelUpdateLimit(double value)
{
    m_channelUpdateLimit = value;
}


double RateLimitConfigV1::getTypingEventLimit() const
{
    return m_typingEventLimit;
}

void RateLimitConfigV1::setTypingEventLimit(double value)
{
    m_typingEventLimit = value;
}


}
}
}
}
}
}
