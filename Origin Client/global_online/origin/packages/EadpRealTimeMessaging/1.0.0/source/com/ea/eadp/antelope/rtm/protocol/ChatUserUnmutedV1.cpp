// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ChatUserUnmutedV1.h>
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

ChatUserUnmutedV1::ChatUserUnmutedV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ChatUserUnmutedV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_unmutedUser(nullptr)
, m_unmutedByUser(nullptr)
, m_unmutedPlayer(nullptr)
, m_unmutedByPlayer(nullptr)
{
}

void ChatUserUnmutedV1::clear()
{
    m_cachedByteSize_ = 0;
    m_unmutedUser.reset();
    m_unmutedByUser.reset();
    m_unmutedPlayer.reset();
    m_unmutedByPlayer.reset();
}

size_t ChatUserUnmutedV1::getByteSize()
{
    size_t total_size = 0;
    if (m_unmutedUser != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_unmutedUser);
    }
    if (m_unmutedByUser != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_unmutedByUser);
    }
    if (m_unmutedPlayer != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_unmutedPlayer);
    }
    if (m_unmutedByPlayer != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_unmutedByPlayer);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ChatUserUnmutedV1::onSerialize(uint8_t* target) const
{
    if (m_unmutedUser != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_unmutedUser, target);
    }
    if (m_unmutedByUser != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(2, *m_unmutedByUser, target);
    }
    if (m_unmutedPlayer != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_unmutedPlayer, target);
    }
    if (m_unmutedByPlayer != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(4, *m_unmutedByPlayer, target);
    }
    return target;
}

bool ChatUserUnmutedV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readMessage(getUnmutedUser().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_unmutedByUser;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_unmutedByUser:
                    if (!input->readMessage(getUnmutedByUser().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_unmutedPlayer;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_unmutedPlayer:
                    if (!input->readMessage(getUnmutedPlayer().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_unmutedByPlayer;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_unmutedByPlayer:
                    if (!input->readMessage(getUnmutedByPlayer().get())) return false;
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
::eadp::foundation::String ChatUserUnmutedV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_unmutedUser != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  unmutedUser: %s,\n", indent.c_str(), m_unmutedUser->toString(indent + "  ").c_str());
    }
    if (m_unmutedByUser != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  unmutedByUser: %s,\n", indent.c_str(), m_unmutedByUser->toString(indent + "  ").c_str());
    }
    if (m_unmutedPlayer != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  unmutedPlayer: %s,\n", indent.c_str(), m_unmutedPlayer->toString(indent + "  ").c_str());
    }
    if (m_unmutedByPlayer != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  unmutedByPlayer: %s,\n", indent.c_str(), m_unmutedByPlayer->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> ChatUserUnmutedV1::getUnmutedUser()
{
    if (m_unmutedUser == nullptr)
    {
        m_unmutedUser = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PersonaV1>(m_allocator_);
    }
    return m_unmutedUser;
}

void ChatUserUnmutedV1::setUnmutedUser(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> val)
{
    m_unmutedUser = val;
}

bool ChatUserUnmutedV1::hasUnmutedUser()
{
    return m_unmutedUser != nullptr;
}

void ChatUserUnmutedV1::releaseUnmutedUser()
{
    m_unmutedUser.reset();
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> ChatUserUnmutedV1::getUnmutedByUser()
{
    if (m_unmutedByUser == nullptr)
    {
        m_unmutedByUser = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PersonaV1>(m_allocator_);
    }
    return m_unmutedByUser;
}

void ChatUserUnmutedV1::setUnmutedByUser(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> val)
{
    m_unmutedByUser = val;
}

bool ChatUserUnmutedV1::hasUnmutedByUser()
{
    return m_unmutedByUser != nullptr;
}

void ChatUserUnmutedV1::releaseUnmutedByUser()
{
    m_unmutedByUser.reset();
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> ChatUserUnmutedV1::getUnmutedPlayer()
{
    if (m_unmutedPlayer == nullptr)
    {
        m_unmutedPlayer = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PlayerInfo>(m_allocator_);
    }
    return m_unmutedPlayer;
}

void ChatUserUnmutedV1::setUnmutedPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> val)
{
    m_unmutedPlayer = val;
}

bool ChatUserUnmutedV1::hasUnmutedPlayer()
{
    return m_unmutedPlayer != nullptr;
}

void ChatUserUnmutedV1::releaseUnmutedPlayer()
{
    m_unmutedPlayer.reset();
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> ChatUserUnmutedV1::getUnmutedByPlayer()
{
    if (m_unmutedByPlayer == nullptr)
    {
        m_unmutedByPlayer = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::PlayerInfo>(m_allocator_);
    }
    return m_unmutedByPlayer;
}

void ChatUserUnmutedV1::setUnmutedByPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> val)
{
    m_unmutedByPlayer = val;
}

bool ChatUserUnmutedV1::hasUnmutedByPlayer()
{
    return m_unmutedByPlayer != nullptr;
}

void ChatUserUnmutedV1::releaseUnmutedByPlayer()
{
    m_unmutedByPlayer.reset();
}


}
}
}
}
}
}
