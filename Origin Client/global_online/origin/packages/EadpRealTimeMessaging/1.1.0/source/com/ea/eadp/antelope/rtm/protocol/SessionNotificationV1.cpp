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
, m_platform(1)
, m_sessionLimitRemaining(0)
, m_connectedSessions(nullptr)
{
}

void SessionNotificationV1::clear()
{
    m_cachedByteSize_ = 0;
    m_type = 1;
    m_message.clear();
    m_platform = 1;
    m_sessionLimitRemaining = 0;
    m_connectedSessions.reset();
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
    if (m_platform != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_platform);
    }
    if (m_sessionLimitRemaining != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_sessionLimitRemaining);
    }
    if (m_connectedSessions != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_connectedSessions);
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
    if (m_platform != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(3, static_cast<int32_t>(getPlatform()), target);
    }
    if (getSessionLimitRemaining() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(4, getSessionLimitRemaining(), target);
    }
    if (m_connectedSessions != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(5, *m_connectedSessions, target);
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
                if (input->expectTag(24)) goto parse_platform;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_platform:
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setPlatform(static_cast<com::ea::eadp::antelope::rtm::protocol::PlatformV1>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(32)) goto parse_sessionLimitRemaining;
                break;
            }

            case 4:
            {
                if (tag == 32)
                {
                    parse_sessionLimitRemaining:
                    if (!input->readInt32(&m_sessionLimitRemaining))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_connectedSessions;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_connectedSessions:
                    if (!input->readMessage(getConnectedSessions().get())) return false;
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
    if (m_platform != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  platform: %d,\n", indent.c_str(), m_platform);
    }
    if (m_sessionLimitRemaining != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  sessionLimitRemaining: %d,\n", indent.c_str(), m_sessionLimitRemaining);
    }
    if (m_connectedSessions != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  connectedSessions: %s,\n", indent.c_str(), m_connectedSessions->toString(indent + "  ").c_str());
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


com::ea::eadp::antelope::rtm::protocol::PlatformV1 SessionNotificationV1::getPlatform() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::PlatformV1>(m_platform);
}
void SessionNotificationV1::setPlatform(com::ea::eadp::antelope::rtm::protocol::PlatformV1 value)
{
    m_platform = static_cast<int>(value);
}

int32_t SessionNotificationV1::getSessionLimitRemaining() const
{
    return m_sessionLimitRemaining;
}

void SessionNotificationV1::setSessionLimitRemaining(int32_t value)
{
    m_sessionLimitRemaining = value;
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionV1> SessionNotificationV1::getConnectedSessions()
{
    if (m_connectedSessions == nullptr)
    {
        m_connectedSessions = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::SessionV1>(m_allocator_);
    }
    return m_connectedSessions;
}

void SessionNotificationV1::setConnectedSessions(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::SessionV1> val)
{
    m_connectedSessions = val;
}

bool SessionNotificationV1::hasConnectedSessions()
{
    return m_connectedSessions != nullptr;
}

void SessionNotificationV1::releaseConnectedSessions()
{
    m_connectedSessions.reset();
}


}
}
}
}
}
}
