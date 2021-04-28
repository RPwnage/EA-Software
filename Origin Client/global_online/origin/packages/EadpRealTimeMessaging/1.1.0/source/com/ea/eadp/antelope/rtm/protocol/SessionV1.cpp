// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/SessionV1.h>
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

SessionV1::SessionV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.SessionV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_sessionKey(m_allocator_.emptyString())
, m_platform(1)
, m_clientVersion(m_allocator_.emptyString())
, m_timestamp(m_allocator_.emptyString())
{
}

void SessionV1::clear()
{
    m_cachedByteSize_ = 0;
    m_sessionKey.clear();
    m_platform = 1;
    m_clientVersion.clear();
    m_timestamp.clear();
}

size_t SessionV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_sessionKey.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_sessionKey);
    }
    if (m_platform != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_platform);
    }
    if (!m_clientVersion.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_clientVersion);
    }
    if (!m_timestamp.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_timestamp);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* SessionV1::onSerialize(uint8_t* target) const
{
    if (!getSessionKey().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getSessionKey(), target);
    }
    if (m_platform != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(2, static_cast<int32_t>(getPlatform()), target);
    }
    if (!getClientVersion().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getClientVersion(), target);
    }
    if (!getTimestamp().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(4, getTimestamp(), target);
    }
    return target;
}

bool SessionV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_sessionKey))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(16)) goto parse_platform;
                break;
            }

            case 2:
            {
                if (tag == 16)
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
                if (input->expectTag(26)) goto parse_clientVersion;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_clientVersion:
                    if (!input->readString(&m_clientVersion))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_timestamp;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_timestamp:
                    if (!input->readString(&m_timestamp))
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
::eadp::foundation::String SessionV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_sessionKey.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  sessionKey: \"%s\",\n", indent.c_str(), m_sessionKey.c_str());
    }
    if (m_platform != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  platform: %d,\n", indent.c_str(), m_platform);
    }
    if (!m_clientVersion.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  clientVersion: \"%s\",\n", indent.c_str(), m_clientVersion.c_str());
    }
    if (!m_timestamp.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  timestamp: \"%s\",\n", indent.c_str(), m_timestamp.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& SessionV1::getSessionKey() const
{
    return m_sessionKey;
}

void SessionV1::setSessionKey(const ::eadp::foundation::String& value)
{
    m_sessionKey = value;
}

void SessionV1::setSessionKey(const char8_t* value)
{
    m_sessionKey = m_allocator_.make<::eadp::foundation::String>(value);
}


com::ea::eadp::antelope::rtm::protocol::PlatformV1 SessionV1::getPlatform() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::PlatformV1>(m_platform);
}
void SessionV1::setPlatform(com::ea::eadp::antelope::rtm::protocol::PlatformV1 value)
{
    m_platform = static_cast<int>(value);
}

const ::eadp::foundation::String& SessionV1::getClientVersion() const
{
    return m_clientVersion;
}

void SessionV1::setClientVersion(const ::eadp::foundation::String& value)
{
    m_clientVersion = value;
}

void SessionV1::setClientVersion(const char8_t* value)
{
    m_clientVersion = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& SessionV1::getTimestamp() const
{
    return m_timestamp;
}

void SessionV1::setTimestamp(const ::eadp::foundation::String& value)
{
    m_timestamp = value;
}

void SessionV1::setTimestamp(const char8_t* value)
{
    m_timestamp = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
