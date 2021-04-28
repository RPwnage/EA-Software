// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/LogoutRequest.h>
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

LogoutRequest::LogoutRequest(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.LogoutRequest"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
{
}

void LogoutRequest::clear()
{
    m_cachedByteSize_ = 0;
}

size_t LogoutRequest::getByteSize()
{
    size_t total_size = 0;
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* LogoutRequest::onSerialize(uint8_t* target) const
{
    return target;
}

bool LogoutRequest::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
{
    clear();
    uint32_t tag;
    for (;;)
    {
        tag = input->readTag();
        if (tag > 127) goto handle_unusual;
    handle_unusual:
        if (tag == 0)
        {
            return true;
        }
        if(!input->skipField(tag)) return false;
    }
}
::eadp::foundation::String LogoutRequest::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    result.append(indent);
    result.append("}");
    return result;
}

}
}
}
}
}
