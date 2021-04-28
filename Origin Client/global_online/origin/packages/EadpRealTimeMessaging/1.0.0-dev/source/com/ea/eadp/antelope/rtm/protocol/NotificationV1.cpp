// GENERATED CODE (Source: notification_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/NotificationV1.h>
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

NotificationV1::NotificationV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.NotificationV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_type(m_allocator_.emptyString())
, m_payload(m_allocator_.emptyString())
{
}

void NotificationV1::clear()
{
    m_cachedByteSize_ = 0;
    m_type.clear();
    m_payload.clear();
}

size_t NotificationV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_type.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_type);
    }
    if (!m_payload.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_payload);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* NotificationV1::onSerialize(uint8_t* target) const
{
    if (!getType().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getType(), target);
    }
    if (!getPayload().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getPayload(), target);
    }
    return target;
}

bool NotificationV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_type))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_payload;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_payload:
                    if (!input->readString(&m_payload))
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
::eadp::foundation::String NotificationV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_type.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  type: \"%s\",\n", indent.c_str(), m_type.c_str());
    }
    if (!m_payload.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  payload: \"%s\",\n", indent.c_str(), m_payload.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& NotificationV1::getType() const
{
    return m_type;
}

void NotificationV1::setType(const ::eadp::foundation::String& value)
{
    m_type = value;
}

void NotificationV1::setType(const char8_t* value)
{
    m_type = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& NotificationV1::getPayload() const
{
    return m_payload;
}

void NotificationV1::setPayload(const ::eadp::foundation::String& value)
{
    m_payload = value;
}

void NotificationV1::setPayload(const char8_t* value)
{
    m_payload = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
