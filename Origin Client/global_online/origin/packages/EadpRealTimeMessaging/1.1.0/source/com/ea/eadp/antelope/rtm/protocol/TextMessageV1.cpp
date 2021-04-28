// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/TextMessageV1.h>
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

TextMessageV1::TextMessageV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.TextMessageV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_fromPersonaId(m_allocator_.emptyString())
, m_fromDisplayName(m_allocator_.emptyString())
, m_timestamp(m_allocator_.emptyString())
, m_text(m_allocator_.emptyString())
, m_originalText(m_allocator_.emptyString())
, m_player(nullptr)
, m_fromNickName(m_allocator_.emptyString())
{
}

void TextMessageV1::clear()
{
    m_cachedByteSize_ = 0;
    m_fromPersonaId.clear();
    m_fromDisplayName.clear();
    m_timestamp.clear();
    m_text.clear();
    m_originalText.clear();
    m_player.reset();
    m_fromNickName.clear();
}

size_t TextMessageV1::getByteSize()
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
    if (!m_timestamp.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_timestamp);
    }
    if (!m_text.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_text);
    }
    if (!m_originalText.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_originalText);
    }
    if (m_player != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_player);
    }
    if (!m_fromNickName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_fromNickName);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* TextMessageV1::onSerialize(uint8_t* target) const
{
    if (!getFromPersonaId().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getFromPersonaId(), target);
    }
    if (!getFromDisplayName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getFromDisplayName(), target);
    }
    if (!getTimestamp().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getTimestamp(), target);
    }
    if (!getText().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(4, getText(), target);
    }
    if (!getOriginalText().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(5, getOriginalText(), target);
    }
    if (m_player != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(6, *m_player, target);
    }
    if (!getFromNickName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(7, getFromNickName(), target);
    }
    return target;
}

bool TextMessageV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                if (input->expectTag(26)) goto parse_timestamp;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_timestamp:
                    if (!input->readString(&m_timestamp))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_text;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_text:
                    if (!input->readString(&m_text))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_originalText;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_originalText:
                    if (!input->readString(&m_originalText))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(50)) goto parse_player;
                break;
            }

            case 6:
            {
                if (tag == 50)
                {
                    parse_player:
                    if (!input->readMessage(getPlayer().get())) return false;
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(58)) goto parse_fromNickName;
                break;
            }

            case 7:
            {
                if (tag == 58)
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
::eadp::foundation::String TextMessageV1::toString(const ::eadp::foundation::String& indent) const
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
    if (!m_timestamp.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  timestamp: \"%s\",\n", indent.c_str(), m_timestamp.c_str());
    }
    if (!m_text.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  text: \"%s\",\n", indent.c_str(), m_text.c_str());
    }
    if (!m_originalText.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  originalText: \"%s\",\n", indent.c_str(), m_originalText.c_str());
    }
    if (m_player != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  player: %s,\n", indent.c_str(), m_player->toString(indent + "  ").c_str());
    }
    if (!m_fromNickName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fromNickName: \"%s\",\n", indent.c_str(), m_fromNickName.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& TextMessageV1::getFromPersonaId() const
{
    return m_fromPersonaId;
}

void TextMessageV1::setFromPersonaId(const ::eadp::foundation::String& value)
{
    m_fromPersonaId = value;
}

void TextMessageV1::setFromPersonaId(const char8_t* value)
{
    m_fromPersonaId = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& TextMessageV1::getFromDisplayName() const
{
    return m_fromDisplayName;
}

void TextMessageV1::setFromDisplayName(const ::eadp::foundation::String& value)
{
    m_fromDisplayName = value;
}

void TextMessageV1::setFromDisplayName(const char8_t* value)
{
    m_fromDisplayName = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& TextMessageV1::getTimestamp() const
{
    return m_timestamp;
}

void TextMessageV1::setTimestamp(const ::eadp::foundation::String& value)
{
    m_timestamp = value;
}

void TextMessageV1::setTimestamp(const char8_t* value)
{
    m_timestamp = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& TextMessageV1::getText() const
{
    return m_text;
}

void TextMessageV1::setText(const ::eadp::foundation::String& value)
{
    m_text = value;
}

void TextMessageV1::setText(const char8_t* value)
{
    m_text = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& TextMessageV1::getOriginalText() const
{
    return m_originalText;
}

void TextMessageV1::setOriginalText(const ::eadp::foundation::String& value)
{
    m_originalText = value;
}

void TextMessageV1::setOriginalText(const char8_t* value)
{
    m_originalText = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> TextMessageV1::getPlayer()
{
    if (m_player == nullptr)
    {
        m_player = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::Player>(m_allocator_);
    }
    return m_player;
}

void TextMessageV1::setPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> val)
{
    m_player = val;
}

bool TextMessageV1::hasPlayer()
{
    return m_player != nullptr;
}

void TextMessageV1::releasePlayer()
{
    m_player.reset();
}


const ::eadp::foundation::String& TextMessageV1::getFromNickName() const
{
    return m_fromNickName;
}

void TextMessageV1::setFromNickName(const ::eadp::foundation::String& value)
{
    m_fromNickName = value;
}

void TextMessageV1::setFromNickName(const char8_t* value)
{
    m_fromNickName = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
