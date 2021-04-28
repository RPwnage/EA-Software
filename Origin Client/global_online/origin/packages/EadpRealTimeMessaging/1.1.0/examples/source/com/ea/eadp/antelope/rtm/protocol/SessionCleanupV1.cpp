// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/SessionCleanupV1.h>
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

SessionCleanupV1::SessionCleanupV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.SessionCleanupV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_sessionKey(m_allocator_.emptyString())
{
}

void SessionCleanupV1::clear()
{
    m_cachedByteSize_ = 0;
    m_sessionKey.clear();
}

size_t SessionCleanupV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_sessionKey.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_sessionKey);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* SessionCleanupV1::onSerialize(uint8_t* target) const
{
    if (!getSessionKey().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getSessionKey(), target);
    }
    return target;
}

bool SessionCleanupV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
::eadp::foundation::String SessionCleanupV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_sessionKey.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  sessionKey: \"%s\",\n", indent.c_str(), m_sessionKey.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& SessionCleanupV1::getSessionKey() const
{
    return m_sessionKey;
}

void SessionCleanupV1::setSessionKey(const ::eadp::foundation::String& value)
{
    m_sessionKey = value;
}

void SessionCleanupV1::setSessionKey(const char8_t* value)
{
    m_sessionKey = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
