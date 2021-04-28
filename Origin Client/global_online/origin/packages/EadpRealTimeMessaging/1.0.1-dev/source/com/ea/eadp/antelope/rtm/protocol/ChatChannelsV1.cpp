// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChatChannelsV1.h>
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

ChatChannelsV1::ChatChannelsV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChatChannelsV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_channels(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelV1>>>())
, m_muteList(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelMuteListV1>>>())
, m_lastOnlineTimestamp(m_allocator_.emptyString())
{
}

void ChatChannelsV1::clear()
{
    m_cachedByteSize_ = 0;
    m_channels.clear();
    m_muteList.clear();
    m_lastOnlineTimestamp.clear();
}

size_t ChatChannelsV1::getByteSize()
{
    size_t total_size = 0;
    total_size += 1 * m_channels.size();
    for (EADPSDK_SIZE_T i = 0; i < m_channels.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_channels[i]);
    }
    total_size += 1 * m_muteList.size();
    for (EADPSDK_SIZE_T i = 0; i < m_muteList.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_muteList[i]);
    }
    if (!m_lastOnlineTimestamp.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_lastOnlineTimestamp);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChatChannelsV1::onSerialize(uint8_t* target) const
{
    for (EADPSDK_SIZE_T i = 0; i < m_channels.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_channels[i], target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_muteList.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_muteList[i], target);
    }
    if (!getLastOnlineTimestamp().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getLastOnlineTimestamp(), target);
    }
    return target;
}

bool ChatChannelsV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    parse_loop_channels:
                    auto tmpChannels = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChannelV1>(m_allocator_);
                    if (!input->readMessage(tmpChannels.get()))
                    {
                        return false;
                    }
                    m_channels.push_back(tmpChannels);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(10)) goto parse_loop_channels;
                if (input->expectTag(18)) goto parse_loop_muteList;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_loop_muteList:
                    auto tmpMuteList = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChannelMuteListV1>(m_allocator_);
                    if (!input->readMessage(tmpMuteList.get()))
                    {
                        return false;
                    }
                    m_muteList.push_back(tmpMuteList);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_loop_muteList;
                if (input->expectTag(26)) goto parse_lastOnlineTimestamp;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_lastOnlineTimestamp:
                    if (!input->readString(&m_lastOnlineTimestamp))
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
::eadp::foundation::String ChatChannelsV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_channels.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  channels: [\n", indent.c_str());
        for (auto iter = m_channels.begin(); iter != m_channels.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    if (m_muteList.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  muteList: [\n", indent.c_str());
        for (auto iter = m_muteList.begin(); iter != m_muteList.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    if (!m_lastOnlineTimestamp.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  lastOnlineTimestamp: \"%s\",\n", indent.c_str(), m_lastOnlineTimestamp.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelV1>>& ChatChannelsV1::getChannels()
{
    return m_channels;
}

::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChannelMuteListV1>>& ChatChannelsV1::getMuteList()
{
    return m_muteList;
}

const ::eadp::foundation::String& ChatChannelsV1::getLastOnlineTimestamp() const
{
    return m_lastOnlineTimestamp;
}

void ChatChannelsV1::setLastOnlineTimestamp(const ::eadp::foundation::String& value)
{
    m_lastOnlineTimestamp = value;
}

void ChatChannelsV1::setLastOnlineTimestamp(const char8_t* value)
{
    m_lastOnlineTimestamp = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
