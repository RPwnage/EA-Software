// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/HistoryRequest.h>
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

HistoryRequest::HistoryRequest(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.HistoryRequest"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.emptyString())
, m_timestamp(m_allocator_.emptyString())
, m_recordCount(0)
{
}

void HistoryRequest::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_timestamp.clear();
    m_recordCount = 0;
}

size_t HistoryRequest::getByteSize()
{
    size_t total_size = 0;
    if (!m_channelId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId);
    }
    if (!m_timestamp.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_timestamp);
    }
    if (m_recordCount != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_recordCount);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* HistoryRequest::onSerialize(uint8_t* target) const
{
    if (!getChannelId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getChannelId(), target);
    }
    if (!getTimestamp().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getTimestamp(), target);
    }
    if (getRecordCount() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(3, getRecordCount(), target);
    }
    return target;
}

bool HistoryRequest::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(18)) goto parse_timestamp;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_timestamp:
                    if (!input->readString(&m_timestamp))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(24)) goto parse_recordCount;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_recordCount:
                    if (!input->readInt32(&m_recordCount))
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
::eadp::foundation::String HistoryRequest::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_channelId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelId: \"%s\",\n", indent.c_str(), m_channelId.c_str());
    }
    if (!m_timestamp.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  timestamp: \"%s\",\n", indent.c_str(), m_timestamp.c_str());
    }
    if (m_recordCount != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  recordCount: %d,\n", indent.c_str(), m_recordCount);
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& HistoryRequest::getChannelId() const
{
    return m_channelId;
}

void HistoryRequest::setChannelId(const ::eadp::foundation::String& value)
{
    m_channelId = value;
}

void HistoryRequest::setChannelId(const char8_t* value)
{
    m_channelId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& HistoryRequest::getTimestamp() const
{
    return m_timestamp;
}

void HistoryRequest::setTimestamp(const ::eadp::foundation::String& value)
{
    m_timestamp = value;
}

void HistoryRequest::setTimestamp(const char8_t* value)
{
    m_timestamp = m_allocator_.make<::eadp::foundation::String>(value);
}


int32_t HistoryRequest::getRecordCount() const
{
    return m_recordCount;
}

void HistoryRequest::setRecordCount(int32_t value)
{
    m_recordCount = value;
}


}
}
}
}
}
