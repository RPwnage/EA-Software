// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/TextMessage.h>
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

TextMessage::TextMessage(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.TextMessage"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_fromPersonaId(m_allocator_.emptyString())
, m_fromDisplayName(m_allocator_.emptyString())
, m_text(m_allocator_.emptyString())
, m_originalText(m_allocator_.emptyString())
, m_fromPlayer(nullptr)
{
}

void TextMessage::clear()
{
    m_cachedByteSize_ = 0;
    m_fromPersonaId.clear();
    m_fromDisplayName.clear();
    m_text.clear();
    m_originalText.clear();
    m_fromPlayer.reset();
}

size_t TextMessage::getByteSize()
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
    if (!m_text.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_text);
    }
    if (!m_originalText.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_originalText);
    }
    if (m_fromPlayer != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_fromPlayer);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* TextMessage::onSerialize(uint8_t* target) const
{
    if (!getFromPersonaId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getFromPersonaId(), target);
    }
    if (!getFromDisplayName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getFromDisplayName(), target);
    }
    if (!getText().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getText(), target);
    }
    if (!getOriginalText().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(4, getOriginalText(), target);
    }
    if (m_fromPlayer != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(5, *m_fromPlayer, target);
    }
    return target;
}

bool TextMessage::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(26)) goto parse_text;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_text:
                    if (!input->readString(&m_text))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_originalText;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_originalText:
                    if (!input->readString(&m_originalText))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_fromPlayer;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_fromPlayer:
                    if (!input->readMessage(getFromPlayer().get())) return false;
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
::eadp::foundation::String TextMessage::toString(const ::eadp::foundation::String& indent) const
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
    if (!m_text.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  text: \"%s\",\n", indent.c_str(), m_text.c_str());
    }
    if (!m_originalText.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  originalText: \"%s\",\n", indent.c_str(), m_originalText.c_str());
    }
    if (m_fromPlayer != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fromPlayer: %s,\n", indent.c_str(), m_fromPlayer->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& TextMessage::getFromPersonaId() const
{
    return m_fromPersonaId;
}

void TextMessage::setFromPersonaId(const ::eadp::foundation::String& value)
{
    m_fromPersonaId = value;
}

void TextMessage::setFromPersonaId(const char8_t* value)
{
    m_fromPersonaId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& TextMessage::getFromDisplayName() const
{
    return m_fromDisplayName;
}

void TextMessage::setFromDisplayName(const ::eadp::foundation::String& value)
{
    m_fromDisplayName = value;
}

void TextMessage::setFromDisplayName(const char8_t* value)
{
    m_fromDisplayName = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& TextMessage::getText() const
{
    return m_text;
}

void TextMessage::setText(const ::eadp::foundation::String& value)
{
    m_text = value;
}

void TextMessage::setText(const char8_t* value)
{
    m_text = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& TextMessage::getOriginalText() const
{
    return m_originalText;
}

void TextMessage::setOriginalText(const ::eadp::foundation::String& value)
{
    m_originalText = value;
}

void TextMessage::setOriginalText(const char8_t* value)
{
    m_originalText = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Player> TextMessage::getFromPlayer()
{
    if (m_fromPlayer == nullptr)
    {
        m_fromPlayer = m_allocator_.makeShared<com::ea::eadp::antelope::protocol::Player>(m_allocator_);
    }
    return m_fromPlayer;
}

void TextMessage::setFromPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Player> val)
{
    m_fromPlayer = val;
}

bool TextMessage::hasFromPlayer()
{
    return m_fromPlayer != nullptr;
}

void TextMessage::releaseFromPlayer()
{
    m_fromPlayer.reset();
}


}
}
}
}
}
