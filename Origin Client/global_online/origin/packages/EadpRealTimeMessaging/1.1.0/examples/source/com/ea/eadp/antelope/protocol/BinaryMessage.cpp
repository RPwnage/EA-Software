// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/BinaryMessage.h>
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

BinaryMessage::BinaryMessage(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.BinaryMessage"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_fromPersonaId(m_allocator_.emptyString())
, m_fromDisplayName(m_allocator_.emptyString())
, m_blob(m_allocator_.emptyString())
, m_fromPlayer(nullptr)
, m_fromNickName(m_allocator_.emptyString())
{
}

void BinaryMessage::clear()
{
    m_cachedByteSize_ = 0;
    m_fromPersonaId.clear();
    m_fromDisplayName.clear();
    m_blob.clear();
    m_fromPlayer.reset();
    m_fromNickName.clear();
}

size_t BinaryMessage::getByteSize()
{
    size_t total_size = 0;
    if (!m_fromPersonaId.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_fromPersonaId);
    }
    if (!m_fromDisplayName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_fromDisplayName);
    }
    if (!m_blob.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getBytesSize(m_blob);
    }
    if (m_fromPlayer != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_fromPlayer);
    }
    if (!m_fromNickName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_fromNickName);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* BinaryMessage::onSerialize(uint8_t* target) const
{
    if (!getFromPersonaId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getFromPersonaId(), target);
    }
    if (!getFromDisplayName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getFromDisplayName(), target);
    }
    if (!getBlob().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBytes(3, getBlob(), target);
    }
    if (m_fromPlayer != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(4, *m_fromPlayer, target);
    }
    if (!getFromNickName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(5, getFromNickName(), target);
    }
    return target;
}

bool BinaryMessage::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_fromPersonaId))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_fromDisplayName;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_fromDisplayName:
                    if (!input->readString(&m_fromDisplayName))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_blob;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_blob:
                    if (!input->readBytes(&m_blob))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_fromPlayer;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_fromPlayer:
                    if (!input->readMessage(getFromPlayer().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_fromNickName;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_fromNickName:
                    if (!input->readString(&m_fromNickName))
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
::eadp::foundation::String BinaryMessage::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_fromPersonaId.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fromPersonaId: \"%s\",\n", indent.c_str(), m_fromPersonaId.c_str());
    }
    if (!m_fromDisplayName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fromDisplayName: \"%s\",\n", indent.c_str(), m_fromDisplayName.c_str());
    }
    if (!m_blob.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  blob: \"%s\",\n", indent.c_str(), m_blob.c_str());
    }
    if (m_fromPlayer != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fromPlayer: %s,\n", indent.c_str(), m_fromPlayer->toString(indent + "  ").c_str());
    }
    if (!m_fromNickName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fromNickName: \"%s\",\n", indent.c_str(), m_fromNickName.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& BinaryMessage::getFromPersonaId() const
{
    return m_fromPersonaId;
}

void BinaryMessage::setFromPersonaId(const ::eadp::foundation::String& value)
{
    m_fromPersonaId = value;
}

void BinaryMessage::setFromPersonaId(const char8_t* value)
{
    m_fromPersonaId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& BinaryMessage::getFromDisplayName() const
{
    return m_fromDisplayName;
}

void BinaryMessage::setFromDisplayName(const ::eadp::foundation::String& value)
{
    m_fromDisplayName = value;
}

void BinaryMessage::setFromDisplayName(const char8_t* value)
{
    m_fromDisplayName = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& BinaryMessage::getBlob() const
{
    return m_blob;
}

void BinaryMessage::setBlob(const ::eadp::foundation::String& value)
{
    m_blob = value;
}

void BinaryMessage::setBlob(const char8_t* value)
{
    m_blob = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Player> BinaryMessage::getFromPlayer()
{
    if (m_fromPlayer == nullptr)
    {
        m_fromPlayer = m_allocator_.makeShared<com::ea::eadp::antelope::protocol::Player>(m_allocator_);
    }
    return m_fromPlayer;
}

void BinaryMessage::setFromPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Player> val)
{
    m_fromPlayer = val;
}

bool BinaryMessage::hasFromPlayer()
{
    return m_fromPlayer != nullptr;
}

void BinaryMessage::releaseFromPlayer()
{
    m_fromPlayer.reset();
}


const ::eadp::foundation::String& BinaryMessage::getFromNickName() const
{
    return m_fromNickName;
}

void BinaryMessage::setFromNickName(const ::eadp::foundation::String& value)
{
    m_fromNickName = value;
}

void BinaryMessage::setFromNickName(const char8_t* value)
{
    m_fromNickName = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
