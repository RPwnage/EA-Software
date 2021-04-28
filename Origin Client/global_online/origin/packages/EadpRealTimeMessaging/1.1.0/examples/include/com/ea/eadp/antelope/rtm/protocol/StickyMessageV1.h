// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/TextMessageV1.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class TextMessageV1;  } } } } } }

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

class EADPGENERATED_API StickyMessageV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit StickyMessageV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     */
    const ::eadp::foundation::String& getStickyAuthorPersonaId() const;
    void setStickyAuthorPersonaId(const ::eadp::foundation::String& value);
    void setStickyAuthorPersonaId(const char8_t* value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getStickyAuthorDisplayName() const;
    void setStickyAuthorDisplayName(const ::eadp::foundation::String& value);
    void setStickyAuthorDisplayName(const char8_t* value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getTimestamp() const;
    void setTimestamp(const ::eadp::foundation::String& value);
    void setTimestamp(const char8_t* value);
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TextMessageV1> getTextMessage();
    void setTextMessage(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TextMessageV1>);
    bool hasTextMessage();
    void releaseTextMessage();
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getMessageId() const;
    void setMessageId(const ::eadp::foundation::String& value);
    void setMessageId(const char8_t* value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getStickyAuthorNickName() const;
    void setStickyAuthorNickName(const ::eadp::foundation::String& value);
    void setStickyAuthorNickName(const char8_t* value);
    /**@}*/

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
    ::eadp::foundation::String m_stickyAuthorPersonaId;
    ::eadp::foundation::String m_stickyAuthorDisplayName;
    ::eadp::foundation::String m_timestamp;
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TextMessageV1> m_textMessage;
    ::eadp::foundation::String m_messageId;
    ::eadp::foundation::String m_stickyAuthorNickName;

};

}
}
}
}
}
}
