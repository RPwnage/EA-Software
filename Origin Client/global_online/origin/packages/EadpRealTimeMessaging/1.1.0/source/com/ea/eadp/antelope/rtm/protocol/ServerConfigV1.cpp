// GENERATED CODE (Source: config_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ServerConfigV1.h>
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

ServerConfigV1::ServerConfigV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ServerConfigV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_rateLimitConfig(nullptr)
{
}

void ServerConfigV1::clear()
{
    m_cachedByteSize_ = 0;
    m_rateLimitConfig.reset();
}

size_t ServerConfigV1::getByteSize()
{
    size_t total_size = 0;
    if (m_rateLimitConfig != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_rateLimitConfig);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ServerConfigV1::onSerialize(uint8_t* target) const
{
    if (m_rateLimitConfig != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_rateLimitConfig, target);
    }
    return target;
}

bool ServerConfigV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readMessage(getRateLimitConfig().get())) return false;
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
::eadp::foundation::String ServerConfigV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_rateLimitConfig != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  rateLimitConfig: %s,\n", indent.c_str(), m_rateLimitConfig->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RateLimitConfigV1> ServerConfigV1::getRateLimitConfig()
{
    if (m_rateLimitConfig == nullptr)
    {
        m_rateLimitConfig = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::RateLimitConfigV1>(m_allocator_);
    }
    return m_rateLimitConfig;
}

void ServerConfigV1::setRateLimitConfig(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RateLimitConfigV1> val)
{
    m_rateLimitConfig = val;
}

bool ServerConfigV1::hasRateLimitConfig()
{
    return m_rateLimitConfig != nullptr;
}

void ServerConfigV1::releaseRateLimitConfig()
{
    m_rateLimitConfig.reset();
}


}
}
}
}
}
}
