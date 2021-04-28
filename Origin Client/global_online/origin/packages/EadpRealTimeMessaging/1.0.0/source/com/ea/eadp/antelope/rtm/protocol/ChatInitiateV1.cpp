// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChatInitiateV1.h>
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

ChatInitiateV1::ChatInitiateV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChatInitiateV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_personaIds(m_allocator_.make<::eadp::foundation::Vector<::eadp::foundation::String>>())
, m_players(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player>>>())
{
}

void ChatInitiateV1::clear()
{
    m_cachedByteSize_ = 0;
    m_personaIds.clear();
    m_players.clear();
}

size_t ChatInitiateV1::getByteSize()
{
    size_t total_size = 0;
    total_size += 1 * m_personaIds.size();
    for (EADPSDK_SIZE_T i = 0; i < m_personaIds.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_personaIds[i]);
    }
    total_size += 1 * m_players.size();
    for (EADPSDK_SIZE_T i = 0; i < m_players.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_players[i]);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChatInitiateV1::onSerialize(uint8_t* target) const
{
    for (EADPSDK_SIZE_T i = 0; i < m_personaIds.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, m_personaIds[i], target);
    }
    for (EADPSDK_SIZE_T i = 0; i < m_players.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_players[i], target);
    }
    return target;
}

bool ChatInitiateV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    parse_personaIds:
                    if (!input->readString(m_personaIds))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(10)) goto parse_personaIds;
                if (input->expectTag(18)) goto parse_players;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_players:
                    parse_loop_players:
                    auto tmpPlayers = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::Player>(m_allocator_);
                    if (!input->readMessage(tmpPlayers.get()))
                    {
                        return false;
                    }
                    m_players.push_back(tmpPlayers);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_loop_players;
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
::eadp::foundation::String ChatInitiateV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_personaIds.size() > 0)
    {
        result += indent + "  personaIds: [\n";
        for (EADPSDK_SIZE_T i = 0; i < m_personaIds.size(); i++)
        {
            result += indent + "    \"" + m_personaIds[i] + "\",";
        }
        result += indent + "  ],\n";
    }
    if (m_players.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  players: [\n", indent.c_str());
        for (auto iter = m_players.begin(); iter != m_players.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::Vector< ::eadp::foundation::String>& ChatInitiateV1::getPersonaIds()
{
    return m_personaIds;
}
void ChatInitiateV1::setPersonaIds(int index, const char8_t* value)
{
    m_personaIds[index] = m_allocator_.make<::eadp::foundation::String>(value);
}
void ChatInitiateV1::addPersonaIds(const char8_t* value)
{
    m_personaIds.push_back(m_allocator_.make<::eadp::foundation::String>(value));
}

::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player>>& ChatInitiateV1::getPlayers()
{
    return m_players;
}

}
}
}
}
}
}
