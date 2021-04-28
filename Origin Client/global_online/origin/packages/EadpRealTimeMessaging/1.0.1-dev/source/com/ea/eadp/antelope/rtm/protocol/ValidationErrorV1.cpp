// GENERATED CODE (Source: error_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ValidationErrorV1.h>
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

ValidationErrorV1::ValidationErrorV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ValidationErrorV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_fieldViolation(m_allocator_.make<::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::FieldViolationV1>>>())
{
}

void ValidationErrorV1::clear()
{
    m_cachedByteSize_ = 0;
    m_fieldViolation.clear();
}

size_t ValidationErrorV1::getByteSize()
{
    size_t total_size = 0;
    total_size += 1 * m_fieldViolation.size();
    for (EADPSDK_SIZE_T i = 0; i < m_fieldViolation.size(); i++)
    {
        total_size += ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_fieldViolation[i]);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ValidationErrorV1::onSerialize(uint8_t* target) const
{
    for (EADPSDK_SIZE_T i = 0; i < m_fieldViolation.size(); i++)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(1, *m_fieldViolation[i], target);
    }
    return target;
}

bool ValidationErrorV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    parse_loop_fieldViolation:
                    auto tmpFieldViolation = m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::FieldViolationV1>(m_allocator_);
                    if (!input->readMessage(tmpFieldViolation.get()))
                    {
                        return false;
                    }
                    m_fieldViolation.push_back(tmpFieldViolation);
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(10)) goto parse_loop_fieldViolation;
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
::eadp::foundation::String ValidationErrorV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_fieldViolation.size() > 0)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fieldViolation: [\n", indent.c_str());
        for (auto iter = m_fieldViolation.begin(); iter != m_fieldViolation.end(); iter++)
        {
            ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s    %s,\n", indent.c_str(), (*iter)->toString(indent + "  ").c_str());
        }
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  ],\n", indent.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::FieldViolationV1>>& ValidationErrorV1::getFieldViolation()
{
    return m_fieldViolation;
}

}
}
}
}
}
}
