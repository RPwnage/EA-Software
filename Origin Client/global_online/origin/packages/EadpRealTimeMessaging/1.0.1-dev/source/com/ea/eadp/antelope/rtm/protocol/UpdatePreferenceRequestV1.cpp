// GENERATED CODE (Source: preference_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/UpdatePreferenceRequestV1.h>
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

UpdatePreferenceRequestV1::UpdatePreferenceRequestV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.UpdatePreferenceRequestV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_bodyCase(BodyCase::kNotSet)
{
}

void UpdatePreferenceRequestV1::clear()
{
    m_cachedByteSize_ = 0;
    clearBody();
}

size_t UpdatePreferenceRequestV1::getByteSize()
{
    size_t total_size = 0;
    if (hasTranslation() && m_body.m_translation)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_translation);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* UpdatePreferenceRequestV1::onSerialize(uint8_t* target) const
{
    if (hasTranslation() && m_body.m_translation)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_body.m_translation, target);
    }
    return target;
}

bool UpdatePreferenceRequestV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    clearBody();
                    if (!input->readMessage(getTranslation().get()))
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
::eadp::foundation::String UpdatePreferenceRequestV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (hasTranslation() && m_body.m_translation)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  translation: %s,\n", indent.c_str(), m_body.m_translation->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1> UpdatePreferenceRequestV1::getTranslation()
{
    if (!hasTranslation())
    {
        setTranslation(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1>(m_allocator_));
    }
    return m_body.m_translation;
}

bool UpdatePreferenceRequestV1::hasTranslation() const
{
    return m_bodyCase == BodyCase::kTranslation;
}

void UpdatePreferenceRequestV1::setTranslation(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1> value)
{
    clearBody();
    new(&m_body.m_translation) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1>(value);
    m_bodyCase = BodyCase::kTranslation;
}

void UpdatePreferenceRequestV1::releaseTranslation()
{
    if (hasTranslation())
    {
        m_body.m_translation.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


void UpdatePreferenceRequestV1::clearBody()
{
    switch(getBodyCase())
    {
        case BodyCase::kTranslation:
            m_body.m_translation.reset();
            break;
        case BodyCase::kNotSet:
        default:
            break;
    }
    m_bodyCase = BodyCase::kNotSet;
}


}
}
}
}
}
}
