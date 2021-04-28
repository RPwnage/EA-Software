// GENERATED CODE (Source: notification_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/SessionNotificationV1.h>
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

SessionNotificationV1::SessionNotificationV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.SessionNotificationV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_type(1)
, m_message(m_allocator_.emptyString())
{
}

void SessionNotificationV1::clear()
{
    m_cachedByteSize_ = 0;
    m_type = 1;
    m_message.clear();
}

size_t SessionNotificationV1::getByteSize()
{
    size_t total_size = 0;
    if (m_type != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_type);
    }
    if (!m_message.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_message);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* SessionNotificationV1::onSerialize(uint8_t* target) const
{
    if (m_type != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(1, static_cast<int32_t>(getType()), target);
    }
    if (!getMessage().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getMessage(), target);
    }
    return target;
}

bool SessionNotificationV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    setType(static_cast<com::ea::eadp::antelope::rtm::protocol::SessionNotificationTypeV1>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_message;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_message:
                    if (!input->readString(&m_message))
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
::eadp::foundation::String SessionNotificationV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_type != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  type: %d,\n", indent.c_str(), m_type);
    }
    if (!m_message.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  message: \"%s\",\n", indent.c_str(), m_message.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

com::ea::eadp::antelope::rtm::protocol::SessionNotificationTypeV1 SessionNotificationV1::getType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::SessionNotificationTypeV1>(m_type);
}
void SessionNotificationV1::setType(com::ea::eadp::antelope::rtm::protocol::SessionNotificationTypeV1 value)
{
    m_type = static_cast<int>(value);
}

const ::eadp::foundation::String& SessionNotificationV1::getMessage() const
{
    return m_message;
}

void SessionNotificationV1::setMessage(const ::eadp::foundation::String& value)
{
    m_message = value;
}

void SessionNotificationV1::setMessage(const char8_t* value)
{
    m_message = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
