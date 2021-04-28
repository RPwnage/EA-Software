// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChatChannelsRequestV2.h>
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

ChatChannelsRequestV2::ChatChannelsRequestV2(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChatChannelsRequestV2"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_includeLastMessageInfo(false)
, m_includeUnreadMessageInfo(false)
, m_unreadMessageTypes(m_allocator_.make<::eadp::foundation::Vector< com::ea::eadp::antelope::rtm::protocol::ChannelMessageType>>())
, m_includeCustomProperties(false)
{
}

void ChatChannelsRequestV2::clear()
{
    m_cachedByteSize_ = 0;
    m_includeLastMessageInfo = false;
    m_includeUnreadMessageInfo = false;
    m_unreadMessageTypes.clear();
    m_includeCustomProperties = false;
}

size_t ChatChannelsRequestV2::getByteSize()
{
    size_t total_size = 0;
    if (m_includeLastMessageInfo != false)
    {
        total_size += 1 + 1;
    }
    if (m_includeUnreadMessageInfo != false)
    {
        total_size += 1 + 1;
    }
    {
        int dataSize = 0;
        for (EADPSDK_SIZE_T i = 0; i < m_unreadMessageTypes.size(); i++)
        {
            dataSize += ::eadp::foundation::Internal::ProtobufWireFormat::getEnumSize(static_cast<int32_t>(m_unreadMessageTypes[i]));
        }
        if (dataSize > 0)
        {
            total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getInt32Size(dataSize);
        }
        m_unreadMessageTypes__cachedByteSize = dataSize;
        total_size += dataSize;
    }
    if (m_includeCustomProperties != false)
    {
        total_size += 1 + 1;
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChatChannelsRequestV2::onSerialize(uint8_t* target) const
{
    if (getIncludeLastMessageInfo() != false)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBool(1, getIncludeLastMessageInfo(), target);
    }
    if (getIncludeUnreadMessageInfo() != false)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBool(2, getIncludeUnreadMessageInfo(), target);
    }
    if (m_unreadMessageTypes.size() > 0)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeTag(3, ::eadp::foundation::Internal::ProtobufWireFormat::WireType::LENGTH_DELIMITED, target);
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeInt32(m_unreadMessageTypes__cachedByteSize, target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_unreadMessageTypes.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeEnum(static_cast<int32_t>(m_unreadMessageTypes[i]), target);
    }
    if (getIncludeCustomProperties() != false)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBool(4, getIncludeCustomProperties(), target);
    }
    return target;
}

bool ChatChannelsRequestV2::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readBool(&m_includeLastMessageInfo))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(16)) goto parse_includeUnreadMessageInfo;
                break;
            }

            case 2:
            {
                if (tag == 16)
                {
                    parse_includeUnreadMessageInfo:
                    if (!input->readBool(&m_includeUnreadMessageInfo))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(24)) goto parse_unreadMessageTypes;
                break;
            }

            case 3:
            {
                if (tag == 24)
                {
                    parse_unreadMessageTypes:
                    int elem;
                    if (!input->readEnum(&elem))
                    {
                        return false;
                    }
                    m_unreadMessageTypes.push_back(static_cast<com::ea::eadp::antelope::rtm::protocol::ChannelMessageType>(elem));
                } else if (tag == 26) {
                    if (!input->readPackedEnum<com::ea::eadp::antelope::rtm::protocol::ChannelMessageType>(&m_unreadMessageTypes)){
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(24)) goto parse_unreadMessageTypes;
                if (input->expectTag(32)) goto parse_includeCustomProperties;
                break;
            }

            case 4:
            {
                if (tag == 32)
                {
                    parse_includeCustomProperties:
                    if (!input->readBool(&m_includeCustomProperties))
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
::eadp::foundation::String ChatChannelsRequestV2::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_includeLastMessageInfo != false)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  includeLastMessageInfo: %d,\n", indent.c_str(), m_includeLastMessageInfo);
    }
    if (m_includeUnreadMessageInfo != false)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  includeUnreadMessageInfo: %d,\n", indent.c_str(), m_includeUnreadMessageInfo);
    }
    if (m_unreadMessageTypes.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  unreadMessageTypes: [\n", indent.c_str());
        for (auto iter = m_unreadMessageTypes.begin(); iter != m_unreadMessageTypes.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %d,\n", indent.c_str(), *iter);
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    if (m_includeCustomProperties != false)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  includeCustomProperties: %d,\n", indent.c_str(), m_includeCustomProperties);
    }
    result.append(indent);
    result.append("}");
    return result;
}

bool ChatChannelsRequestV2::getIncludeLastMessageInfo() const
{
    return m_includeLastMessageInfo;
}

void ChatChannelsRequestV2::setIncludeLastMessageInfo(bool value)
{
    m_includeLastMessageInfo = value;
}


bool ChatChannelsRequestV2::getIncludeUnreadMessageInfo() const
{
    return m_includeUnreadMessageInfo;
}

void ChatChannelsRequestV2::setIncludeUnreadMessageInfo(bool value)
{
    m_includeUnreadMessageInfo = value;
}


::eadp::foundation::Vector< com::ea::eadp::antelope::rtm::protocol::ChannelMessageType>& ChatChannelsRequestV2::getUnreadMessageTypes()
{
    return m_unreadMessageTypes;
}

bool ChatChannelsRequestV2::getIncludeCustomProperties() const
{
    return m_includeCustomProperties;
}

void ChatChannelsRequestV2::setIncludeCustomProperties(bool value)
{
    m_includeCustomProperties = value;
}


}
}
}
}
}
}
