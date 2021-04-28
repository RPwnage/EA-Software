// GENERATED CODE (Source: preference_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/TranslationPreferenceV1.h>
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

TranslationPreferenceV1::TranslationPreferenceV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.TranslationPreferenceV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_autoTranslate(false)
, m_language(m_allocator_.emptyString())
{
}

void TranslationPreferenceV1::clear()
{
    m_cachedByteSize_ = 0;
    m_autoTranslate = false;
    m_language.clear();
}

size_t TranslationPreferenceV1::getByteSize()
{
    size_t total_size = 0;
    if (m_autoTranslate != false)
    {
        total_size += 1 + 1;
    }
    if (!m_language.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_language);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* TranslationPreferenceV1::onSerialize(uint8_t* target) const
{
    if (getAutoTranslate() != false)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBool(1, getAutoTranslate(), target);
    }
    if (!getLanguage().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getLanguage(), target);
    }
    return target;
}

bool TranslationPreferenceV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readBool(&m_autoTranslate))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_language;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_language:
                    if (!input->readString(&m_language))
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
::eadp::foundation::String TranslationPreferenceV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_autoTranslate != false)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  autoTranslate: %d,\n", indent.c_str(), m_autoTranslate);
    }
    if (!m_language.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  language: \"%s\",\n", indent.c_str(), m_language.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

bool TranslationPreferenceV1::getAutoTranslate() const
{
    return m_autoTranslate;
}

void TranslationPreferenceV1::setAutoTranslate(bool value)
{
    m_autoTranslate = value;
}


const ::eadp::foundation::String& TranslationPreferenceV1::getLanguage() const
{
    return m_language;
}

void TranslationPreferenceV1::setLanguage(const ::eadp::foundation::String& value)
{
    m_language = value;
}

void TranslationPreferenceV1::setLanguage(const char8_t* value)
{
    m_language = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
