// GENERATED CODE (Source: error_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#include <com/ea/eadp/antelope/rtm/protocol/ErrorV1.h>
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

ErrorV1::ErrorV1(const ::eadp::foundation::Allocator& allocator)
: ::eadp::foundation::Internal::ProtobufMessage(allocator.make<::eadp::foundation::String>("com.ea.eadp.antelope.rtm.protocol.ErrorV1"))
, m_allocator_(allocator)
, m_cachedByteSize_(0)
, m_errorCode(m_allocator_.emptyString())
, m_errorMessage(m_allocator_.emptyString())
, m_bodyCase(BodyCase::kNotSet)
{
}

void ErrorV1::clear()
{
    m_cachedByteSize_ = 0;
    m_errorCode.clear();
    m_errorMessage.clear();
    clearBody();
}

size_t ErrorV1::getByteSize()
{
    size_t total_size = 0;
    if (!m_errorCode.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_errorCode);
    }
    if (!m_errorMessage.empty())
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getStringSize(m_errorMessage);
    }
    if (hasChatMembersRequestError() && m_body.m_chatMembersRequestError)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_chatMembersRequestError);
    }
    if (hasValidationError() && m_body.m_validationError)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_validationError);
    }
    if (hasAssignError() && m_body.m_assignError)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_assignError);
    }
    if (hasChatTypingEventRequestError() && m_body.m_chatTypingEventRequestError)
    {
        total_size += 1 + ::eadp::foundation::Internal::ProtobufWireFormat::getMessageSize(*m_body.m_chatTypingEventRequestError);
    }
    m_cachedByteSize_ = total_size;
    return total_size;
}

uint8_t* ErrorV1::onSerialize(uint8_t* target) const
{
    if (!getErrorCode().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(1, getErrorCode(), target);
    }
    if (!getErrorMessage().empty())
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeString(2, getErrorMessage(), target);
    }
    if (hasChatMembersRequestError() && m_body.m_chatMembersRequestError)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(3, *m_body.m_chatMembersRequestError, target);
    }
    if (hasValidationError() && m_body.m_validationError)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(4, *m_body.m_validationError, target);
    }
    if (hasAssignError() && m_body.m_assignError)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(5, *m_body.m_assignError, target);
    }
    if (hasChatTypingEventRequestError() && m_body.m_chatTypingEventRequestError)
    {
        target = ::eadp::foundation::Internal::ProtobufWireFormat::writeMessage(6, *m_body.m_chatTypingEventRequestError, target);
    }
    return target;
}

bool ErrorV1::onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input)
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
                    if (!input->readString(&m_errorCode))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(18)) goto parse_errorMessage;
                break;
            }

            case 2:
            {
                if (tag == 18)
                {
                    parse_errorMessage:
                    if (!input->readString(&m_errorMessage))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(26)) goto parse_chatMembersRequestError;
                break;
            }

            case 3:
            {
                if (tag == 26)
                {
                    parse_chatMembersRequestError:
                    clearBody();
                    if (!input->readMessage(getChatMembersRequestError().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(34)) goto parse_validationError;
                break;
            }

            case 4:
            {
                if (tag == 34)
                {
                    parse_validationError:
                    clearBody();
                    if (!input->readMessage(getValidationError().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(42)) goto parse_assignError;
                break;
            }

            case 5:
            {
                if (tag == 42)
                {
                    parse_assignError:
                    clearBody();
                    if (!input->readMessage(getAssignError().get()))
                    {
                        return false;
                    }
                } else {
                    goto handle_unusual;
                }
                if (input->expectTag(50)) goto parse_chatTypingEventRequestError;
                break;
            }

            case 6:
            {
                if (tag == 50)
                {
                    parse_chatTypingEventRequestError:
                    clearBody();
                    if (!input->readMessage(getChatTypingEventRequestError().get()))
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
::eadp::foundation::String ErrorV1::toString(const ::eadp::foundation::String& indent) const
{
    ::eadp::foundation::String result = m_allocator_.make<::eadp::foundation::String>();
    result.append(indent);
    result.append("{\n");
    if (!m_errorCode.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  errorCode: \"%s\",\n", indent.c_str(), m_errorCode.c_str());
    }
    if (!m_errorMessage.empty())
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  errorMessage: \"%s\",\n", indent.c_str(), m_errorMessage.c_str());
    }
    if (hasChatMembersRequestError() && m_body.m_chatMembersRequestError)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatMembersRequestError: %s,\n", indent.c_str(), m_body.m_chatMembersRequestError->toString(indent + "  ").c_str());
    }
    if (hasValidationError() && m_body.m_validationError)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  validationError: %s,\n", indent.c_str(), m_body.m_validationError->toString(indent + "  ").c_str());
    }
    if (hasAssignError() && m_body.m_assignError)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  assignError: %s,\n", indent.c_str(), m_body.m_assignError->toString(indent + "  ").c_str());
    }
    if (hasChatTypingEventRequestError() && m_body.m_chatTypingEventRequestError)
    {
        ::eadp::foundation::Internal::Utils::appendSprintf(result, "%s  chatTypingEventRequestError: %s,\n", indent.c_str(), m_body.m_chatTypingEventRequestError->toString(indent + "  ").c_str());
    }
    result.append(indent);
    result.append("}");
    return result;
}

const ::eadp::foundation::String& ErrorV1::getErrorCode() const
{
    return m_errorCode;
}

void ErrorV1::setErrorCode(const ::eadp::foundation::String& value)
{
    m_errorCode = value;
}

void ErrorV1::setErrorCode(const char8_t* value)
{
    m_errorCode = m_allocator_.make<::eadp::foundation::String>(value);
}


const ::eadp::foundation::String& ErrorV1::getErrorMessage() const
{
    return m_errorMessage;
}

void ErrorV1::setErrorMessage(const ::eadp::foundation::String& value)
{
    m_errorMessage = value;
}

void ErrorV1::setErrorMessage(const char8_t* value)
{
    m_errorMessage = m_allocator_.make<::eadp::foundation::String>(value);
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestErrorV1> ErrorV1::getChatMembersRequestError()
{
    if (!hasChatMembersRequestError())
    {
        setChatMembersRequestError(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestErrorV1>(m_allocator_));
    }
    return m_body.m_chatMembersRequestError;
}

bool ErrorV1::hasChatMembersRequestError() const
{
    return m_bodyCase == BodyCase::kChatMembersRequestError;
}

void ErrorV1::setChatMembersRequestError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestErrorV1> value)
{
    clearBody();
    new(&m_body.m_chatMembersRequestError) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestErrorV1>(value);
    m_bodyCase = BodyCase::kChatMembersRequestError;
}

void ErrorV1::releaseChatMembersRequestError()
{
    if (hasChatMembersRequestError())
    {
        m_body.m_chatMembersRequestError.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ValidationErrorV1> ErrorV1::getValidationError()
{
    if (!hasValidationError())
    {
        setValidationError(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ValidationErrorV1>(m_allocator_));
    }
    return m_body.m_validationError;
}

bool ErrorV1::hasValidationError() const
{
    return m_bodyCase == BodyCase::kValidationError;
}

void ErrorV1::setValidationError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ValidationErrorV1> value)
{
    clearBody();
    new(&m_body.m_validationError) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ValidationErrorV1>(value);
    m_bodyCase = BodyCase::kValidationError;
}

void ErrorV1::releaseValidationError()
{
    if (hasValidationError())
    {
        m_body.m_validationError.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AssignErrorV1> ErrorV1::getAssignError()
{
    if (!hasAssignError())
    {
        setAssignError(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::AssignErrorV1>(m_allocator_));
    }
    return m_body.m_assignError;
}

bool ErrorV1::hasAssignError() const
{
    return m_bodyCase == BodyCase::kAssignError;
}

void ErrorV1::setAssignError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AssignErrorV1> value)
{
    clearBody();
    new(&m_body.m_assignError) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AssignErrorV1>(value);
    m_bodyCase = BodyCase::kAssignError;
}

void ErrorV1::releaseAssignError()
{
    if (hasAssignError())
    {
        m_body.m_assignError.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestErrorV1> ErrorV1::getChatTypingEventRequestError()
{
    if (!hasChatTypingEventRequestError())
    {
        setChatTypingEventRequestError(m_allocator_.makeShared<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestErrorV1>(m_allocator_));
    }
    return m_body.m_chatTypingEventRequestError;
}

bool ErrorV1::hasChatTypingEventRequestError() const
{
    return m_bodyCase == BodyCase::kChatTypingEventRequestError;
}

void ErrorV1::setChatTypingEventRequestError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestErrorV1> value)
{
    clearBody();
    new(&m_body.m_chatTypingEventRequestError) ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestErrorV1>(value);
    m_bodyCase = BodyCase::kChatTypingEventRequestError;
}

void ErrorV1::releaseChatTypingEventRequestError()
{
    if (hasChatTypingEventRequestError())
    {
        m_body.m_chatTypingEventRequestError.reset();
        m_bodyCase = BodyCase::kNotSet;
    }
}


void ErrorV1::clearBody()
{
    switch(getBodyCase())
    {
        case BodyCase::kChatMembersRequestError:
            m_body.m_chatMembersRequestError.reset();
            break;
        case BodyCase::kValidationError:
            m_body.m_validationError.reset();
            break;
        case BodyCase::kAssignError:
            m_body.m_assignError.reset();
            break;
        case BodyCase::kChatTypingEventRequestError:
            m_body.m_chatTypingEventRequestError.reset();
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
