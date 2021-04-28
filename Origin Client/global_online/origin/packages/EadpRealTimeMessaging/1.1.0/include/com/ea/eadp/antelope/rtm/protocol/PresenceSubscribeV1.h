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

/**
 * Requests
 * For each player in the list, the requestor either gets back a PresenceV1 with the current presence of that player or PresenceSubscriptionErrorV1 in case of error
 */
class EADPGENERATED_API PresenceSubscribeV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit PresenceSubscribeV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     */
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR ::eadp::foundation::Vector< ::eadp::foundation::String>& getPersonaId();
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR void setPersonaId(int index, const char8_t* value);
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR void addPersonaId(const char8_t* value);
    /**@}*/

    /**@{
     */
    ::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player>>& getPlayers();
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
    ::eadp::foundation::Vector< ::eadp::foundation::String> m_personaId;
    ::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player>> m_players;

};

}
}
}
}
}
}
