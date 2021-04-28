// GENERATED CODE (Source: presence_protocol.proto) - DO NOT EDIT DIRECTLY.
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

class EADPGENERATED_API PresenceSubscriptionErrorV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit PresenceSubscriptionErrorV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     */
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR const ::eadp::foundation::String& getPersonaId() const;
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR void setPersonaId(const ::eadp::foundation::String& value);
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR void setPersonaId(const char8_t* value);
    /**@}*/

    /**@{
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
    ::eadp::foundation::String m_personaId;
    ::eadp::foundation::String m_errorCode;
    ::eadp::foundation::String m_errorMessage;
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> m_player;

};

}
}
}
}
}
}
