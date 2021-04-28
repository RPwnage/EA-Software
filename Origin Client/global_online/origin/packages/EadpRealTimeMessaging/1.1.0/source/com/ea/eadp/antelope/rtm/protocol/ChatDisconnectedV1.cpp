// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChatDisconnectedV1.h>
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

ChatDisconnectedV1::ChatDisconnectedV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChatDisconnectedV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_user(nullptr)
, m_players(nullptr)
{
}

void ChatDisconnectedV1::clear()
{
    m_cachedByteSize_ = 0;
    m_user.reset();
    m_players.reset();
}

size_t ChatDisconnectedV1::getByteSize()
{
    size_t total_size = 0;
    if (m_user != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_user);
    }
    if (m_players != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_players);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChatDisconnectedV1::onSerialize(uint8_t* target) const
{
    if (m_user != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_user, target);
    }
    if (m_players != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_players, target);
    }
    return target;
}

bool ChatDisconnectedV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readMessage(getUser().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_players;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_players:
                    if (!input->readMessage(getPlayers().get())) return false;
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
::eadp::foundation::String ChatDisconnectedV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_user != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  user: %s,\n", indent.c_str(), m_user->toString(indent + "  ").c_str());
    }
    if (m_players != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  players: %s,\n", indent.c_str(), m_players->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> ChatDisconnectedV1::getUser()
{
    if (m_user == nullptr)
    {
        m_user = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PersonaV1>(m_allocator_);
    }
    return m_user;
}

void ChatDisconnectedV1::setUser(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> val)
{
    m_user = val;
}

bool ChatDisconnectedV1::hasUser()
{
    return m_user != nullptr;
}

void ChatDisconnectedV1::releaseUser()
{
    m_user.reset();
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> ChatDisconnectedV1::getPlayers()
{
    if (m_players == nullptr)
    {
        m_players = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PlayerInfo>(m_allocator_);
    }
    return m_players;
}

void ChatDisconnectedV1::setPlayers(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> val)
{
    m_players = val;
}

bool ChatDisconnectedV1::hasPlayers()
{
    return m_players != nullptr;
}

void ChatDisconnectedV1::releasePlayers()
{
    m_players.reset();
}


}
}
}
}
}
}
