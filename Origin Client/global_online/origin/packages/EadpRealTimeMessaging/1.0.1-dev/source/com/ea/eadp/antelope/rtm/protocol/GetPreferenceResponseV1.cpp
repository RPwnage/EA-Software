// GENERATED CODE (Source: preference_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/GetPreferenceResponseV1.h>
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

GetPreferenceResponseV1::GetPreferenceResponseV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.GetPreferenceResponseV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_translationPreference(nullptr)
{
}

void GetPreferenceResponseV1::clear()
{
    m_cachedByteSize_ = 0;
    m_translationPreference.reset();
}

size_t GetPreferenceResponseV1::getByteSize()
{
    size_t total_size = 0;
    if (m_translationPreference != nullptr)
    {
    total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_translationPreference);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* GetPreferenceResponseV1::onSerialize(uint8_t* target) const
{
    if (m_translationPreference != nullptr)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_translationPreference, target);
    }
    return target;
}

bool GetPreferenceResponseV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readMessage(getTranslationPreference().get())) return false;
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
::eadp::foundation::String GetPreferenceResponseV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_translationPreference != nullptr)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  translationPreference: %s,\n", indent.c_str(), m_translationPreference->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1> GetPreferenceResponseV1::getTranslationPreference()
{
    if (m_translationPreference == nullptr)
    {
        m_translationPreference = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1>(m_allocator_);
    }
    return m_translationPreference;
}

void GetPreferenceResponseV1::setTranslationPreference(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1> val)
{
    m_translationPreference = val;
}

bool GetPreferenceResponseV1::hasTranslationPreference()
{
    return m_translationPreference != nullptr;
}

void GetPreferenceResponseV1::releaseTranslationPreference()
{
    m_translationPreference.reset();
}


}
}
}
}
}
}
