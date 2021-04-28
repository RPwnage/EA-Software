// GENERATED CODE (Source: rtm_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/LoginRequestV2.h>
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

LoginRequestV2::LoginRequestV2(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.LoginRequestV2"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_token(m_allocator_.emptyString())
, m_reconnect(false)
, m_heartbeat(false)
, m_userType(1)
, m_productId(m_allocator_.emptyString())
, m_singleSessionForceLogout(true)
{
}

void LoginRequestV2::clear()
{
    m_cachedByteSize_ = 0;
    m_token.clear();
    m_reconnect = false;
    m_heartbeat = false;
    m_userType = 1;
    m_productId.clear();
    m_singleSessionForceLogout = true;
}

size_t LoginRequestV2::getByteSize()
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
    if (m_singleSessionForceLogout != true)
    {
        total_size += 1 + 1;
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* LoginRequestV2::onSerialize(uint8_t* target) const
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
    if (getSingleSessionForceLogout() != true)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBool(6, getSingleSessionForceLogout(), target);
    }
    return target;
}

bool LoginRequestV2::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(48)) goto parse_singleSessionForceLogout;
                break;
            }

            case 6:
            {
                if (tag == 48)
                {
                    parse_singleSessionForceLogout:
                    if (!input->readBool(&m_singleSessionForceLogout))
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
::eadp::foundation::String LoginRequestV2::toString(const ::eadp::foundation::String& indent) const
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
    if (m_singleSessionForceLogout != true)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  singleSessionForceLogout: %d,\n", indent.c_str(), m_singleSessionForceLogout);
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& LoginRequestV2::getToken() const
{
    return m_token;
}

void LoginRequestV2::setToken(const ::eadp::foundation::String& value)
{
    m_token = value;
}

void LoginRequestV2::setToken(const char8_t* value)
{
    m_token = m_allocator_.make<::eadp::foundation::String>(value);
}


bool LoginRequestV2::getReconnect() const
{
    return m_reconnect;
}

void LoginRequestV2::setReconnect(bool value)
{
    m_reconnect = value;
}


bool LoginRequestV2::getHeartbeat() const
{
    return m_heartbeat;
}

void LoginRequestV2::setHeartbeat(bool value)
{
    m_heartbeat = value;
}


com::ea::eadp::antelope::rtm::protocol::UserType LoginRequestV2::getUserType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::UserType>(m_userType);
}
void LoginRequestV2::setUserType(com::ea::eadp::antelope::rtm::protocol::UserType value)
{
    m_userType = static_cast<int>(value);
}

const ::eadp::foundation::String& LoginRequestV2::getProductId() const
{
    return m_productId;
}

void LoginRequestV2::setProductId(const ::eadp::foundation::String& value)
{
    m_productId = value;
}

void LoginRequestV2::setProductId(const char8_t* value)
{
    m_productId = m_allocator_.make<::eadp::foundation::String>(value);
}


bool LoginRequestV2::getSingleSessionForceLogout() const
{
    return m_singleSessionForceLogout;
}

void LoginRequestV2::setSingleSessionForceLogout(bool value)
{
    m_singleSessionForceLogout = value;
}


}
}
}
}
}
}
