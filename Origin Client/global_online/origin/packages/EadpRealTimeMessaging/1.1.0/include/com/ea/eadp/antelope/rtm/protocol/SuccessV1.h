// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/ChatInitiateSuccessV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/LoginV2Success.h>
#include <com/ea/eadp/antelope/rtm/protocol/LoginV3Response.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class ChatInitiateSuccessV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class LoginV2Success;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class LoginV3Response;  } } } } } }

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

class EADPGENERATED_API SuccessV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit SuccessV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    ~SuccessV1()
    {
        clearBody();
    }

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success> getLoginV2Success();
    void setLoginV2Success(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success>);
    bool hasLoginV2Success() const;
    void releaseLoginV2Success();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateSuccessV1> getChatInitiateSuccess();
    void setChatInitiateSuccess(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateSuccessV1>);
    bool hasChatInitiateSuccess() const;
    void releaseChatInitiateSuccess();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV3Response> getLoginV3Response();
    void setLoginV3Response(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV3Response>);
    bool hasLoginV3Response() const;
    void releaseLoginV3Response();
    /**@}*/

    /*!
     * Case values for the fields potentially set on the associated union.
     */
    enum class BodyCase
    {
        kNotSet = 0,
        kLoginV2Success = 1,
        kChatInitiateSuccess = 2,
        kLoginV3Response = 3,
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

    BodyCase m_bodyCase;

    union BodyUnion
    {
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV2Success> m_loginV2Success;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::ChatInitiateSuccessV1> m_chatInitiateSuccess;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::LoginV3Response> m_loginV3Response;

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
