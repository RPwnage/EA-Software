// GENERATED CODE (Source: error_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/FieldViolationV1.h>
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

FieldViolationV1::FieldViolationV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.FieldViolationV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_fieldName(m_allocator_.emptyString())
, m_violationMessage(m_allocator_.emptyString())
{
}

void FieldViolationV1::clear()
{
    m_cachedByteSize_ = 0;
    m_fieldName.clear();
    m_violationMessage.clear();
}

size_t FieldViolationV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_fieldName.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_fieldName);
    }
    if (!m_violationMessage.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_violationMessage);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* FieldViolationV1::onSerialize(uint8_t* target) const
{
    if (!getFieldName().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getFieldName(), target);
    }
    if (!getViolationMessage().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getViolationMessage(), target);
    }
    return target;
}

bool FieldViolationV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_fieldName))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_violationMessage;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_violationMessage:
                    if (!input->readString(&m_violationMessage))
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
::eadp::foundation::String FieldViolationV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_fieldName.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  fieldName: \"%s\",\n", indent.c_str(), m_fieldName.c_str());
    }
    if (!m_violationMessage.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  violationMessage: \"%s\",\n", indent.c_str(), m_violationMessage.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& FieldViolationV1::getFieldName() const
{
    return m_fieldName;
}

void FieldViolationV1::setFieldName(const ::eadp::foundation::String& value)
{
    m_fieldName = value;
}

void FieldViolationV1::setFieldName(const char8_t* value)
{
    m_fieldName = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& FieldViolationV1::getViolationMessage() const
{
    return m_violationMessage;
}

void FieldViolationV1::setViolationMessage(const ::eadp::foundation::String& value)
{
    m_violationMessage = value;
}

void FieldViolationV1::setViolationMessage(const char8_t* value)
{
    m_violationMessage = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
}
