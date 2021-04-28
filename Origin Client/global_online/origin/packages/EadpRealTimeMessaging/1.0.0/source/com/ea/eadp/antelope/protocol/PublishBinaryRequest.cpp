// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/PublishBinaryRequest.h>
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

PublishBinaryRequest::PublishBinaryRequest(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.PublishBinaryRequest"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_blob(m_allocator_.emptyString())
{
}

void PublishBinaryRequest::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_blob.clear();
}

size_t PublishBinaryRequest::getByteSize()
{
    size_t total_size = 0;
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    if (!m_blob.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getBytesSize(m_blob);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* PublishBinaryRequest::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    if (!getBlob().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBytes(2, getBlob(), target);
    }
    return target;
}

bool PublishBinaryRequest::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_channelId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_blob;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_blob:
                    if (!input->readBytes(&m_blob))
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
::eadp::foundation::String PublishBinaryRequest::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (!m_blob.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  blob: \"%s\",\n", indent.c_str(), m_blob.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& PublishBinaryRequest::getChannelId() const
{
    return m_channelId;
}

void PublishBinaryRequest::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void PublishBinaryRequest::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& PublishBinaryRequest::getBlob() const
{
    return m_blob;
}

void PublishBinaryRequest::setBlob(const ::eadp::foundation::String& value)
{
    m_blob = value;
}

void PublishBinaryRequest::setBlob(const char8_t* value)
{
    m_blob = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
