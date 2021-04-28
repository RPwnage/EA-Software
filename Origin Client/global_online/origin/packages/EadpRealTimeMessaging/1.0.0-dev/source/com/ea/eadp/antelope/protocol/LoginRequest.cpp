// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/LoginRequest.h>
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

LoginRequest::LoginRequest(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.LoginRequest"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_token(m_allocator_.emptyString())
, m_reconnect(false)
, m_heartbeat(false)
{
}

void LoginRequest::clear()
{
    m_cachedByteSize_ = 0;
    m_token.clear();
    m_reconnect = false;
    m_heartbeat = false;
}

size_t LoginRequest::getByteSize()
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
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* LoginRequest::onSerialize(uint8_t* target) const
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
    return target;
}

bool LoginRequest::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
::eadp::foundation::String LoginRequest::toString(const ::eadp::foundation::String& indent) const
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
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& LoginRequest::getToken() const
{
    return m_token;
}

void LoginRequest::setToken(const ::eadp::foundation::String& value)
{
    m_token = value;
}

void LoginRequest::setToken(const char8_t* value)
{
    m_token = m_allocator_.make<::eadp::foundation::String>(value);
}


bool LoginRequest::getReconnect() const
{
    return m_reconnect;
}

void LoginRequest::setReconnect(bool value)
{
    m_reconnect = value;
}


bool LoginRequest::getHeartbeat() const
{
    return m_heartbeat;
}

void LoginRequest::setHeartbeat(bool value)
{
    m_heartbeat = value;
}


}
}
}
}
}
