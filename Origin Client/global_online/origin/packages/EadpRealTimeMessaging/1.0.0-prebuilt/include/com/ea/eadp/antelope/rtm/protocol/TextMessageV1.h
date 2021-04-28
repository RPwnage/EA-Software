// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/Player.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class Player;  } } } } } }

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

/**
 *  equivalent of TextMessage in SocialProtocol to be used in RTM Protocol
 */
class EADPGENERATED_API TextMessageV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit TextMessageV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     */
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR const ::eadp::foundation::String& getFromPersonaId() const;
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR void setFromPersonaId(const ::eadp::foundation::String& value);
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR void setFromPersonaId(const char8_t* value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getFromDisplayName() const;
    void setFromDisplayName(const ::eadp::foundation::String& value);
    void setFromDisplayName(const char8_t* value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getTimestamp() const;
    void setTimestamp(const ::eadp::foundation::String& value);
    void setTimestamp(const char8_t* value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getText() const;
    void setText(const ::eadp::foundation::String& value);
    void setText(const char8_t* value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getOriginalText() const;
    void setOriginalText(const ::eadp::foundation::String& value);
    void setOriginalText(const char8_t* value);
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> getPlayer();
    void setPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player>);
    bool hasPlayer();
    void releasePlayer();
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
    ::eadp::foundation::String m_fromPersonaId;
    ::eadp::foundation::String m_fromDisplayName;
    ::eadp::foundation::String m_timestamp;
    ::eadp::foundation::String m_text;
    ::eadp::foundation::String m_originalText;
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> m_player;

};

}
}
}
}
}
}
