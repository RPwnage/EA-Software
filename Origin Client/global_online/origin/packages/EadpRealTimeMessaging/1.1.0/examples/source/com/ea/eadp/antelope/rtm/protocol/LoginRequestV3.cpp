// GENERATED CODE (Source: rtm_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/LoginRequestV3.h>
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

LoginRequestV3::LoginRequestV3(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.LoginRequestV3"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_token(m_allocator_.emptyString())
, m_reconnect(false)
, m_heartbeat(false)
, m_userType(1)
, m_productId(m_allocator_.emptyString())
, m_platform(1)
, m_clientVersion(m_allocator_.emptyString())
, m_sessionKey(m_allocator_.emptyString())
, m_forceDisconnectSessionKey(m_allocator_.emptyString())
{
}

void LoginRequestV3::clear()
{
    m_cachedByteSize_ = 0;
    m_token.clear();
    m_reconnect = false;
    m_heartbeat = false;
    m_userType = 1;
    m_productId.clear();
    m_platform = 1;
    m_clientVersion.clear();
    m_sessionKey.clear();
    m_forceDisconnectSessionKey.clear();
}

size_t LoginRequestV3::getByteSize()
{
    size_t total_size = 0;
    if (!m_token.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_token);
    }
    if (m_reconnect != false)
    {
        total_size += 1 + 1;
    }
    if (m_heartbeat != false)
    {
        total_size += 1 + 1;
    }
    if (m_userType != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_userType);
    }
    if (!m_productId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_productId);
    }
    if (m_platform != 1)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_platform);
    }
    if (!m_clientVersion.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_clientVersion);
    }
    if (!m_sessionKey.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_sessionKey);
    }
    if (!m_forceDisconnectSessionKey.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_forceDisconnectSessionKey);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* LoginRequestV3::onSerialize(uint8_t* target) const
{
    if (!getToken().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getToken(), target);
    }
    if (getReconnect() != false)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBool(2, getReconnect(), target);
    }
    if (getHeartbeat() != false)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBool(3, getHeartbeat(), target);
    }
    if (m_userType != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(4, static_cast<int32_t>(getUserType()), target);
    }
    if (!getProductId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(5, getProductId(), target);
    }
    if (m_platform != 1)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(6, static_cast<int32_t>(getPlatform()), target);
    }
    if (!getClientVersion().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(7, getClientVersion(), target);
    }
    if (!getSessionKey().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(8, getSessionKey(), target);
    }
    if (!getForceDisconnectSessionKey().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(9, getForceDisconnectSessionKey(), target);
    }
    return target;
}

bool LoginRequestV3::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_token))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(16)) goto parse_reconnect;
                break;
            }

            case 2:
            {
                if (tag == 16)
                {
                    parse_reconnect:
                    if (!input->readBool(&m_reconnect))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(24)) goto parse_heartbeat;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_heartbeat:
                    if (!input->readBool(&m_heartbeat))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(32)) goto parse_userType;
                break;
            }

            case 4:
            {
                if (tag == 32)
                {
                    parse_userType:
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setUserType(static_cast<com::ea::eadp::antelope::rtm::protocol::UserType>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_productId;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_productId:
                    if (!input->readString(&m_productId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(48)) goto parse_platform;
                break;
            }

            case 6:
            {
                if (tag == 48)
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
                if (input->expectTag(58)) goto parse_clientVersion;
                break;
            }

            case 7:
            {
                if (tag == 58)
                {
                    parse_clientVersion:
                    if (!input->readString(&m_clientVersion))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(66)) goto parse_sessionKey;
                break;
            }

            case 8:
            {
                if (tag == 66)
                {
                    parse_sessionKey:
                    if (!input->readString(&m_sessionKey))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(74)) goto parse_forceDisconnectSessionKey;
                break;
            }

            case 9:
            {
                if (tag == 74)
                {
                    parse_forceDisconnectSessionKey:
                    if (!input->readString(&m_forceDisconnectSessionKey))
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
::eadp::foundation::String LoginRequestV3::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_token.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  token: \"%s\",\n", indent.c_str(), m_token.c_str());
    }
    if (m_reconnect != false)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  reconnect: %d,\n", indent.c_str(), m_reconnect);
    }
    if (m_heartbeat != false)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  heartbeat: %d,\n", indent.c_str(), m_heartbeat);
    }
    if (m_userType != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  userType: %d,\n", indent.c_str(), m_userType);
    }
    if (!m_productId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  productId: \"%s\",\n", indent.c_str(), m_productId.c_str());
    }
    if (m_platform != 1)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  platform: %d,\n", indent.c_str(), m_platform);
    }
    if (!m_clientVersion.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  clientVersion: \"%s\",\n", indent.c_str(), m_clientVersion.c_str());
    }
    if (!m_sessionKey.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  sessionKey: \"%s\",\n", indent.c_str(), m_sessionKey.c_str());
    }
    if (!m_forceDisconnectSessionKey.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  forceDisconnectSessionKey: \"%s\",\n", indent.c_str(), m_forceDisconnectSessionKey.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& LoginRequestV3::getToken() const
{
    return m_token;
}

void LoginRequestV3::setToken(const ::eadp::foundation::String& value)
{
    m_token = value;
}

void LoginRequestV3::setToken(const char8_t* value)
{
    m_token = m_allocator_.make<::eadp::foundation::String>(value);
}


bool LoginRequestV3::getReconnect() const
{
    return m_reconnect;
}

void LoginRequestV3::setReconnect(bool value)
{
    m_reconnect = value;
}


bool LoginRequestV3::getHeartbeat() const
{
    return m_heartbeat;
}

void LoginRequestV3::setHeartbeat(bool value)
{
    m_heartbeat = value;
}


com::ea::eadp::antelope::rtm::protocol::UserType LoginRequestV3::getUserType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::UserType>(m_userType);
}
void LoginRequestV3::setUserType(com::ea::eadp::antelope::rtm::protocol::UserType value)
{
    m_userType = static_cast<int>(value);
}

const ::eadp::foundation::String& LoginRequestV3::getProductId() const
{
    return m_productId;
}

void LoginRequestV3::setProductId(const ::eadp::foundation::String& value)
{
    m_productId = value;
}

void LoginRequestV3::setProductId(const char8_t* value)
{
    m_productId = m_allocator_.make<::eadp::foundation::String>(value);
}


com::ea::eadp::antelope::rtm::protocol::PlatformV1 LoginRequestV3::getPlatform() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::PlatformV1>(m_platform);
}
void LoginRequestV3::setPlatform(com::ea::eadp::antelope::rtm::protocol::PlatformV1 value)
{
    m_platform = static_cast<int>(value);
}

const ::eadp::foundation::String& LoginRequestV3::getClientVersion() const
{
    return m_clientVersion;
}

void LoginRequestV3::setClientVersion(const ::eadp::foundation::String& value)
{
    m_clientVersion = value;
}

void LoginRequestV3::setClientVersion(const char8_t* value)
{
    m_clientVersion = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& LoginRequestV3::getSessionKey() const
{
    return m_sessionKey;
}

void LoginRequestV3::setSessionKey(const ::eadp::foundation::String& value)
{
    m_sessionKey = value;
}

void LoginRequestV3::setSessionKey(const char8_t* value)
{
    m_sessionKey = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& LoginRequestV3::getForceDisconnectSessionKey() const
{
    return m_forceDisconnectSessionKey;
}

void LoginRequestV3::setForceDisconnectSessionKey(const ::eadp::foundation::String& value)
{
    m_forceDisconnectSessionKey = value;
}

void LoginRequestV3::setForceDisconnectSessionKey(const char8_t* value)
{
    m_forceDisconnectSessionKey = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
