// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/WorldChatShard.h>
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

WorldChatShard::WorldChatShard(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.WorldChatShard"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_index(0)
, m_size(0)
{
}

void WorldChatShard::clear()
{
    m_cachedByteSize_ = 0;
    m_index = 0;
    m_size = 0;
}

size_t WorldChatShard::getByteSize()
{
    size_t total_size = 0;
    if (m_index != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_index);
    }
    if (m_size != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_size);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* WorldChatShard::onSerialize(uint8_t* target) const
{
    if (getIndex() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(1, getIndex(), target);
    }
    if (getSize() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(2, getSize(), target);
    }
    return target;
}

bool WorldChatShard::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (tag == 8)
                {
                    if (!input->readInt32(&m_index))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(16)) goto parse_size;
                break;
            }

            case 2:
            {
                if (tag == 16)
                {
                    parse_size:
                    if (!input->readInt32(&m_size))
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
::eadp::foundation::String WorldChatShard::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_index != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  index: %d,\n", indent.c_str(), m_index);
    }
    if (m_size != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  size: %d,\n", indent.c_str(), m_size);
    }
    result.append(indent);
    result.append("}");
    return result;
}

int32_t WorldChatShard::getIndex() const
{
    return m_index;
}

void WorldChatShard::setIndex(int32_t value)
{
    m_index = value;
}


int32_t WorldChatShard::getSize() const
{
    return m_size;
}

void WorldChatShard::setSize(int32_t value)
{
    m_size = value;
}


}
}
}
}
}
}
