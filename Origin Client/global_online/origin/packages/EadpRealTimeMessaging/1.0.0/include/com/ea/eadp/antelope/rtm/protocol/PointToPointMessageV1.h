// GENERATED CODE (Source: point_to_point.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/AddressV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/CustomMessage.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class AddressV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class CustomMessage;  } } } } } }

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

class EADPGENERATED_API PointToPointMessageV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit PointToPointMessageV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    ~PointToPointMessageV1()
    {
        clearMessageContent();
    }

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AddressV1> getFrom();
    void setFrom(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AddressV1>);
    bool hasFrom();
    void releaseFrom();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AddressV1> getTo();
    void setTo(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AddressV1>);
    bool hasTo();
    void releaseTo();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CustomMessage> getCustomMessage();
    void setCustomMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CustomMessage>);
    bool hasCustomMessage() const;
    void releaseCustomMessage();
    /**@}*/

    /*!
     * Case values for the fields potentially set on the associated union.
     */
    enum class MessageContentCase
    {
        kNotSet = 0,
        kCustomMessage = 3,
    };

    /*!
     * Returns the case corresponding to the currently set union field.
     */
    MessageContentCase getMessageContentCase() { return static_cast<MessageContentCase>(m_messageContentCase); }

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
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AddressV1> m_from;
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::AddressV1> m_to;

    MessageContentCase m_messageContentCase;

    union MessageContentUnion
    {
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::CustomMessage> m_customMessage;

        MessageContentUnion() {}
        ~MessageContentUnion() {}
    } m_messageContent;

    void clearMessageContent();

};

}
}
}
}
}
}
