// GENERATED CODE (Source: rtm_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ReconnectRequestV1.h>
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

ReconnectRequestV1::ReconnectRequestV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ReconnectRequestV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
{
}

void ReconnectRequestV1::clear()
{
    m_cachedByteSize_ = 0;
}

size_t ReconnectRequestV1::getByteSize()
{
    size_t total_size = 0;
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ReconnectRequestV1::onSerialize(uint8_t* target) const
{
    return target;
}

bool ReconnectRequestV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
::eadp::foundation::String ReconnectRequestV1::toString(const ::eadp::foundation::String& indent) const
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
}
