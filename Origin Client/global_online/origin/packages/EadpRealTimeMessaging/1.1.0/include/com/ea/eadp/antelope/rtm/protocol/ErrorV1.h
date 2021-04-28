// GENERATED CODE (Source: error_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/AssignErrorV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatMembersRequestErrorV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatTypingEventRequestErrorV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/LoginErrorV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/ValidationErrorV1.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class AssignErrorV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatMembersRequestErrorV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatTypingEventRequestErrorV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class LoginErrorV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ValidationErrorV1;  } } } } } }

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

class EADPGENERATED_API ErrorV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit ErrorV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    ~ErrorV1()
    {
        clearBody();
    }

    /**@{
     *  ErrorCode: {
     *   "SERVER_ERROR";
     *   "VALIDATION_FAILED";
     *   "AUTHORIZATION_ERROR";
     *   "NOT_AUTHENTICATED";
     *   "INVALID_REQUEST";
     *   "USER_MUTED";
     *   "UNDERAGE_BAN";
     *   "USER_BANNED";
     *   "SESSION_NOT_FOUND";
     *   "SESSION_ALREADY_EXIST";
     *   "RATE_LIMIT_EXCEEDED";
     *   "SESSION_LIMIT_EXCEEDED";
     *  } 
     */
    const ::eadp::foundation::String& getErrorCode() const;
    void setErrorCode(const ::eadp::foundation::String& value);
    void setErrorCode(const char8_t* value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getErrorMessage() const;
    void setErrorMessage(const ::eadp::foundation::String& value);
    void setErrorMessage(const char8_t* value);
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestErrorV1> getChatMembersRequestError();
    void setChatMembersRequestError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestErrorV1>);
    bool hasChatMembersRequestError() const;
    void releaseChatMembersRequestError();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ValidationErrorV1> getValidationError();
    void setValidationError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ValidationErrorV1>);
    bool hasValidationError() const;
    void releaseValidationError();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AssignErrorV1> getAssignError();
    void setAssignError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AssignErrorV1>);
    bool hasAssignError() const;
    void releaseAssignError();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestErrorV1> getChatTypingEventRequestError();
    void setChatTypingEventRequestError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestErrorV1>);
    bool hasChatTypingEventRequestError() const;
    void releaseChatTypingEventRequestError();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginErrorV1> getLoginError();
    void setLoginError(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginErrorV1>);
    bool hasLoginError() const;
    void releaseLoginError();
    /**@}*/

    /*!
     * Case values for the fields potentially set on the associated union.
     */
    enum class BodyCase
    {
        kNotSet = 0,
        kChatMembersRequestError = 3,
        kValidationError = 4,
        kAssignError = 5,
        kChatTypingEventRequestError = 6,
        kLoginError = 7,
    };

    /*!
     * Returns the case corresponding to the currently set union field.
     */
    BodyCase getBodyCase() { return static_cast<BodyCase>(m_bodyCase); }

    /*!
     * Serializes the message contents into a buffer.
    Called by serialize.
     */
    uint8_t* onSerialize(uint8_t* target) const override;

    /*!
     * Deserializes the message contents from an input stream.
    Called by deserialize.
     */
    bool onDeserialize(::eadp::foundation::Internal::ProtobufInputStream* input) override;

    /*!
     * Returns a previously calculated serialized size of the message in protobuf wire format.
     */
    size_t getCachedSize() const override
    {
        return m_cachedByteSize_;
    }

    /*!
     * Calculates and returns the serialized size of the message in protobuf wire format.
     */
    size_t getByteSize() override;

    /*!
     * Returns a string representing the contents of the message for debugging and logging.
     */
    ::eadp::foundation::String toString(const ::eadp::foundation::String& indent) const override;

private:
    ::eadp::foundation::Allocator m_allocator_;
    size_t m_cachedByteSize_;
    ::eadp::foundation::String m_errorCode;
    ::eadp::foundation::String m_errorMessage;

    BodyCase m_bodyCase;

    union BodyUnion
    {
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatMembersRequestErrorV1> m_chatMembersRequestError;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ValidationErrorV1> m_validationError;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AssignErrorV1> m_assignError;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatTypingEventRequestErrorV1> m_chatTypingEventRequestError;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginErrorV1> m_loginError;

        BodyUnion() {}
        ~BodyUnion() {}
    } m_body;

    void clearBody();

};

}
}
}
}
}
}
