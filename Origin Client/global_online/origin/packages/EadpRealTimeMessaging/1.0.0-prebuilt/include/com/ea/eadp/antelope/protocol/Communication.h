// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/protocol/ChannelMessage.h>
#include <com/ea/eadp/antelope/protocol/Header.h>
#include <com/ea/eadp/antelope/protocol/HistoryRequest.h>
#include <com/ea/eadp/antelope/protocol/HistoryResponse.h>
#include <com/ea/eadp/antelope/protocol/LoginRequest.h>
#include <com/ea/eadp/antelope/protocol/LoginResponse.h>
#include <com/ea/eadp/antelope/protocol/LogoutRequest.h>
#include <com/ea/eadp/antelope/protocol/PublishBinaryRequest.h>
#include <com/ea/eadp/antelope/protocol/PublishResponse.h>
#include <com/ea/eadp/antelope/protocol/PublishTextRequest.h>
#include <com/ea/eadp/antelope/protocol/SubscribeRequest.h>
#include <com/ea/eadp/antelope/protocol/SubscribeResponse.h>
#include <com/ea/eadp/antelope/protocol/UnsubscribeRequest.h>
#include <com/ea/eadp/antelope/protocol/UnsubscribeResponse.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class ChannelMessage;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class Header;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class HistoryRequest;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class HistoryResponse;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class LoginRequest;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class LoginResponse;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class LogoutRequest;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class PublishBinaryRequest;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class PublishResponse;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class PublishTextRequest;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class SubscribeRequest;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class SubscribeResponse;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class UnsubscribeRequest;  } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace protocol { class UnsubscribeResponse;  } } } } }

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

class EADPGENERATED_API Communication : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit Communication(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    ~Communication()
    {
        clearBody();
    }

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Header> getHeader();
    void setHeader(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Header>);
    bool hasHeader();
    void releaseHeader();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LoginRequest> getLoginRequest();
    void setLoginRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LoginRequest>);
    bool hasLoginRequest() const;
    void releaseLoginRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LoginResponse> getLoginResponse();
    void setLoginResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LoginResponse>);
    bool hasLoginResponse() const;
    void releaseLoginResponse();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishTextRequest> getPublishTextRequest();
    void setPublishTextRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishTextRequest>);
    bool hasPublishTextRequest() const;
    void releasePublishTextRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishBinaryRequest> getPublishBinaryRequest();
    void setPublishBinaryRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishBinaryRequest>);
    bool hasPublishBinaryRequest() const;
    void releasePublishBinaryRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishResponse> getPublishResponse();
    void setPublishResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishResponse>);
    bool hasPublishResponse() const;
    void releasePublishResponse();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::ChannelMessage> getChannelMessage();
    void setChannelMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::ChannelMessage>);
    bool hasChannelMessage() const;
    void releaseChannelMessage();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeRequest> getSubscribeRequest();
    void setSubscribeRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeRequest>);
    bool hasSubscribeRequest() const;
    void releaseSubscribeRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeResponse> getSubscribeResponse();
    void setSubscribeResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeResponse>);
    bool hasSubscribeResponse() const;
    void releaseSubscribeResponse();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::UnsubscribeRequest> getUnsubscribeRequest();
    void setUnsubscribeRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::UnsubscribeRequest>);
    bool hasUnsubscribeRequest() const;
    void releaseUnsubscribeRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::UnsubscribeResponse> getUnsubscribeResponse();
    void setUnsubscribeResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::UnsubscribeResponse>);
    bool hasUnsubscribeResponse() const;
    void releaseUnsubscribeResponse();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::HistoryRequest> getHistoryRequest();
    void setHistoryRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::HistoryRequest>);
    bool hasHistoryRequest() const;
    void releaseHistoryRequest();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::HistoryResponse> getHistoryResponse();
    void setHistoryResponse(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::HistoryResponse>);
    bool hasHistoryResponse() const;
    void releaseHistoryResponse();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LogoutRequest> getLogoutRequest();
    void setLogoutRequest(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LogoutRequest>);
    bool hasLogoutRequest() const;
    void releaseLogoutRequest();
    /**@}*/

    /*!
     * Case values for the fields potentially set on the associated union.
     */
    enum class BodyCase
    {
        kNotSet = 0,
        kLoginRequest = 2,
        kLoginResponse = 3,
        kPublishTextRequest = 4,
        kPublishBinaryRequest = 5,
        kPublishResponse = 6,
        kChannelMessage = 7,
        kSubscribeRequest = 8,
        kSubscribeResponse = 9,
        kUnsubscribeRequest = 10,
        kUnsubscribeResponse = 11,
        kHistoryRequest = 12,
        kHistoryResponse = 13,
        kLogoutRequest = 14,
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
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::Header> m_header;

    BodyCase m_bodyCase;

    union BodyUnion
    {
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LoginRequest> m_loginRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LoginResponse> m_loginResponse;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishTextRequest> m_publishTextRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishBinaryRequest> m_publishBinaryRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::PublishResponse> m_publishResponse;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::ChannelMessage> m_channelMessage;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeRequest> m_subscribeRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::SubscribeResponse> m_subscribeResponse;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::UnsubscribeRequest> m_unsubscribeRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::UnsubscribeResponse> m_unsubscribeResponse;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::HistoryRequest> m_historyRequest;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::HistoryResponse> m_historyResponse;
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::protocol::LogoutRequest> m_logoutRequest;

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
