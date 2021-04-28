// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChatMembersRequestV1.h>
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

ChatMembersRequestV1::ChatMembersRequestV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChatMembersRequestV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channelId(m_allocator_.make<::eadp::foundation::Vector<::eadp::foundation::String>>())
, m_channelType(2)
, m_fromIndex(0)
, m_pageSize(25)
{
}

void ChatMembersRequestV1::clear()
{
    m_cachedByteSize_ = 0;
    m_channelId.clear();
    m_channelType = 2;
    m_fromIndex = 0;
    m_pageSize = 25;
}

size_t ChatMembersRequestV1::getByteSize()
{
    size_t total_size = 0;
    total_size += 1 * m_channelId.size();
    for (EADPSDK_SIZE_T i = 0; i < m_channelId.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_channelId[i]);
    }
    if (m_channelType != 2)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(m_channelType);
    }
    if (m_fromIndex != 0)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_fromIndex);
    }
    if (m_pageSize != 25)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(m_pageSize);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChatMembersRequestV1::onSerialize(uint8_t* target) const
{
    for (EADPSDK_SIZE_T i = 0; i < m_channelId.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, m_channelId[i], target);
    }
    if (m_channelType != 2)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(2, static_cast<int32_t>(getChannelType()), target);
    }
    if (getFromIndex() != 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(3, getFromIndex(), target);
    }
    if (getPageSize() != 25)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(4, getPageSize(), target);
    }
    return target;
}

bool ChatMembersRequestV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    parse_channelId:
                    if (!input->readString(m_channelId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(10)) goto parse_channelId;
                if (input->expectTag(16)) goto parse_channelType;
                break;
            }

            case 2:
            {
                if (tag == 16)
                {
                    parse_channelType:
                    int value;
                    if (!input->readEnum(&value))
                    {
                        return false;
                    }
                    setChannelType(static_cast<com::ea::eadp::antelope::rtm::protocol::ChannelType>(value));
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(24)) goto parse_fromIndex;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_fromIndex:
                    if (!input->readInt32(&m_fromIndex))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(32)) goto parse_pageSize;
                break;
            }

            case 4:
            {
                if (tag == 32)
                {
                    parse_pageSize:
                    if (!input->readInt32(&m_pageSize))
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
::eadp::foundation::String ChatMembersRequestV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_channelId.size() > 0)
    {
        result += indent + "  channelId: [\n";
        for (EADPSDK_SIZE_T i = 0; i < m_channelId.size(); i++)
        {
            result += indent + "    \"" + m_channelId[i] + "\",";
        }
        result += indent + "  ],\n";
    }
    if (m_channelType != 2)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channelType: %d,\n", indent.c_str(), m_channelType);
    }
    if (m_fromIndex != 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fromIndex: %d,\n", indent.c_str(), m_fromIndex);
    }
    if (m_pageSize != 25)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  pageSize: %d,\n", indent.c_str(), m_pageSize);
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::Vector< ::eadp::foundation::String>& ChatMembersRequestV1::getChannelId()
{
    return m_channelId;
}
void ChatMembersRequestV1::setChannelId(int index, const char8_t* value)
{
    m_channelId[index] = m_allocator_.make<::eadp::foundation::String>(value);
}
void ChatMembersRequestV1::addChannelId(const char8_t* value)
{
    m_channelId.push_back(m_allocator_.make<::eadp::foundation::String>(value));
}

com::ea::eadp::antelope::rtm::protocol::ChannelType ChatMembersRequestV1::getChannelType() const
{
    return static_cast<com::ea::eadp::antelope::rtm::protocol::ChannelType>(m_channelType);
}
void ChatMembersRequestV1::setChannelType(com::ea::eadp::antelope::rtm::protocol::ChannelType value)
{
    m_channelType = static_cast<int>(value);
}

int32_t ChatMembersRequestV1::getFromIndex() const
{
    return m_fromIndex;
}

void ChatMembersRequestV1::setFromIndex(int32_t value)
{
    m_fromIndex = value;
}


int32_t ChatMembersRequestV1::getPageSize() const
{
    return m_pageSize;
}

void ChatMembersRequestV1::setPageSize(int32_t value)
{
    m_pageSize = value;
}


}
}
}
}
}
}
