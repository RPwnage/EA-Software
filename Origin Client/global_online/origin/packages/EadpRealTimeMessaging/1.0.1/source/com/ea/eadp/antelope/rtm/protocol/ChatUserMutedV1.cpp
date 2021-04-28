// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChatUserMutedV1.h>
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

ChatUserMutedV1::ChatUserMutedV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChatUserMutedV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_mutedUser(nullptr)
, m_mutedByUser(nullptr)
, m_mutedPlayer(nullptr)
, m_mutedByPlayer(nullptr)
{
}

void ChatUserMutedV1::clear()
{
    m_cachedByteSize_ = 0;
    m_mutedUser.reset();
    m_mutedByUser.reset();
    m_mutedPlayer.reset();
    m_mutedByPlayer.reset();
}

size_t ChatUserMutedV1::getByteSize()
{
    size_t total_size = 0;
    if (m_mutedUser != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_mutedUser);
    }
    if (m_mutedByUser != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_mutedByUser);
    }
    if (m_mutedPlayer != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_mutedPlayer);
    }
    if (m_mutedByPlayer != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_mutedByPlayer);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChatUserMutedV1::onSerialize(uint8_t* target) const
{
    if (m_mutedUser != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_mutedUser, target);
    }
    if (m_mutedByUser != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_mutedByUser, target);
    }
    if (m_mutedPlayer != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_mutedPlayer, target);
    }
    if (m_mutedByPlayer != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(4, *m_mutedByPlayer, target);
    }
    return target;
}

bool ChatUserMutedV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readMessage(getMutedUser().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_mutedByUser;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_mutedByUser:
                    if (!input->readMessage(getMutedByUser().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_mutedPlayer;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_mutedPlayer:
                    if (!input->readMessage(getMutedPlayer().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_mutedByPlayer;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_mutedByPlayer:
                    if (!input->readMessage(getMutedByPlayer().get())) return false;
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
::eadp::foundation::String ChatUserMutedV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_mutedUser != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  mutedUser: %s,\n", indent.c_str(), m_mutedUser->toString(indent + "  ").c_str());
    }
    if (m_mutedByUser != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  mutedByUser: %s,\n", indent.c_str(), m_mutedByUser->toString(indent + "  ").c_str());
    }
    if (m_mutedPlayer != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  mutedPlayer: %s,\n", indent.c_str(), m_mutedPlayer->toString(indent + "  ").c_str());
    }
    if (m_mutedByPlayer != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  mutedByPlayer: %s,\n", indent.c_str(), m_mutedByPlayer->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> ChatUserMutedV1::getMutedUser()
{
    if (m_mutedUser == nullptr)
    {
        m_mutedUser = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PersonaV1>(m_allocator_);
    }
    return m_mutedUser;
}

void ChatUserMutedV1::setMutedUser(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> val)
{
    m_mutedUser = val;
}

bool ChatUserMutedV1::hasMutedUser()
{
    return m_mutedUser != nullptr;
}

void ChatUserMutedV1::releaseMutedUser()
{
    m_mutedUser.reset();
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> ChatUserMutedV1::getMutedByUser()
{
    if (m_mutedByUser == nullptr)
    {
        m_mutedByUser = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PersonaV1>(m_allocator_);
    }
    return m_mutedByUser;
}

void ChatUserMutedV1::setMutedByUser(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> val)
{
    m_mutedByUser = val;
}

bool ChatUserMutedV1::hasMutedByUser()
{
    return m_mutedByUser != nullptr;
}

void ChatUserMutedV1::releaseMutedByUser()
{
    m_mutedByUser.reset();
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> ChatUserMutedV1::getMutedPlayer()
{
    if (m_mutedPlayer == nullptr)
    {
        m_mutedPlayer = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PlayerInfo>(m_allocator_);
    }
    return m_mutedPlayer;
}

void ChatUserMutedV1::setMutedPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> val)
{
    m_mutedPlayer = val;
}

bool ChatUserMutedV1::hasMutedPlayer()
{
    return m_mutedPlayer != nullptr;
}

void ChatUserMutedV1::releaseMutedPlayer()
{
    m_mutedPlayer.reset();
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> ChatUserMutedV1::getMutedByPlayer()
{
    if (m_mutedByPlayer == nullptr)
    {
        m_mutedByPlayer = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PlayerInfo>(m_allocator_);
    }
    return m_mutedByPlayer;
}

void ChatUserMutedV1::setMutedByPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> val)
{
    m_mutedByPlayer = val;
}

bool ChatUserMutedV1::hasMutedByPlayer()
{
    return m_mutedByPlayer != nullptr;
}

void ChatUserMutedV1::releaseMutedByPlayer()
{
    m_mutedByPlayer.reset();
}


}
}
}
}
}
}
