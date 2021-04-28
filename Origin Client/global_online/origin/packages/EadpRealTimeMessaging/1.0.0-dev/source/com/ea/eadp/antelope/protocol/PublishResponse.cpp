// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/protocol/PublishResponse.h>
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

PublishResponse::PublishResponse(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.protocol.PublishResponse"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_success(false)
, m_errorCode(m_allocator_.emptyString())
, m_errorMessage(m_allocator_.emptyString())
{
}

void PublishResponse::clear()
{
    m_cachedByteSize_ = 0;
    m_success = false;
    m_errorCode.clear();
    m_errorMessage.clear();
}

size_t PublishResponse::getByteSize()
{
    size_t total_size = 0;
    if (m_success != false)
    {
        total_size += 1 + 1;
    }
    if (!m_errorCode.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_errorCode);
    }
    if (!m_errorMessage.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_errorMessage);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* PublishResponse::onSerialize(uint8_t* target) const
{
    if (getSuccess() != false)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeBool(1, getSuccess(), target);
    }
    if (!getErrorCode().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getErrorCode(), target);
    }
    if (!getErrorMessage().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(3, getErrorMessage(), target);
    }
    return target;
}

bool PublishResponse::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readBool(&m_success))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_errorCode;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_errorCode:
                    if (!input->readString(&m_errorCode))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_errorMessage;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_errorMessage:
                    if (!input->readString(&m_errorMessage))
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
::eadp::foundation::String PublishResponse::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (m_success != false)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  success: %d,\n", indent.c_str(), m_success);
    }
    if (!m_errorCode.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  errorCode: \"%s\",\n", indent.c_str(), m_errorCode.c_str());
    }
    if (!m_errorMessage.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  errorMessage: \"%s\",\n", indent.c_str(), m_errorMessage.c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

bool PublishResponse::getSuccess() const
{
    return m_success;
}

void PublishResponse::setSuccess(bool value)
{
    m_success = value;
}


const ::eadp::foundation::String& PublishResponse::getErrorCode() const
{
    return m_errorCode;
}

void PublishResponse::setErrorCode(const ::eadp::foundation::String& value)
{
    m_errorCode = value;
}

void PublishResponse::setErrorCode(const char8_t* value)
{
    m_errorCode = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& PublishResponse::getErrorMessage() const
{
    return m_errorMessage;
}

void PublishResponse::setErrorMessage(const ::eadp::foundation::String& value)
{
    m_errorMessage = value;
}

void PublishResponse::setErrorMessage(const char8_t* value)
{
    m_errorMessage = m_allocator_.make<::eadp::foundation::String>(value);
}


}
}
}
}
}
