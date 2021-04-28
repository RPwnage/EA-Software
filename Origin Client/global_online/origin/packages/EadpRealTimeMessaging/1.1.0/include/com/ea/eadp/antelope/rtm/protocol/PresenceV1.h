// GENERATED CODE (Source: presence_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/BasicPresenceType.h>
#include <com/ea/eadp/antelope/rtm/protocol/PlatformV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/Player.h>
#include <com/ea/eadp/antelope/rtm/protocol/RichPresenceV1.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class Player;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class RichPresenceV1;  } } } } } }

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
 * Responses
 */
class EADPGENERATED_API PresenceV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit PresenceV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     * existing fields
     */
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR const ::eadp::foundation::String& getPersonaId() const;
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR void setPersonaId(const ::eadp::foundation::String& value);
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR void setPersonaId(const char8_t* value);
    /**@}*/

    /**@{
     *  game specific generic extensible string
     */
    const ::eadp::foundation::String& getStatus() const;
    void setStatus(const ::eadp::foundation::String& value);
    void setStatus(const char8_t* value);
    /**@}*/

    /**@{
     *  timestamp when the status was updated by the client in UTC
     */
    const ::eadp::foundation::String& getTimestamp() const;
    void setTimestamp(const ::eadp::foundation::String& value);
    void setTimestamp(const char8_t* value);
    /**@}*/

    /**@{
     * (playerId + productId)
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> getPlayer();
    void setPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player>);
    bool hasPlayer();
    void releasePlayer();
    /**@}*/

    /**@{
     *  New fields
     *  This enum represents a standard basic presence type for the player
     */
    com::ea::eadp::antelope::rtm::protocol::BasicPresenceType getBasicPresenceType() const;
    void setBasicPresenceType(com::ea::eadp::antelope::rtm::protocol::BasicPresenceType value);
    /**@}*/

    /**@{
     *  Allows user to set his presence to a custom string, we will define a max cap of 50 characters
     */
    const ::eadp::foundation::String& getUserDefinedPresence() const;
    void setUserDefinedPresence(const ::eadp::foundation::String& value);
    void setUserDefinedPresence(const char8_t* value);
    /**@}*/

    /**@{
     *  This struct contains all information about the rich presence status of the player
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RichPresenceV1> getRichPresence();
    void setRichPresence(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RichPresenceV1>);
    bool hasRichPresence();
    void releaseRichPresence();
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getSessionKey() const;
    void setSessionKey(const ::eadp::foundation::String& value);
    void setSessionKey(const char8_t* value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getClientVersion() const;
    void setClientVersion(const ::eadp::foundation::String& value);
    void setClientVersion(const char8_t* value);
    /**@}*/

    /**@{
     */
    com::ea::eadp::antelope::rtm::protocol::PlatformV1 getPlatform() const;
    void setPlatform(com::ea::eadp::antelope::rtm::protocol::PlatformV1 value);
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
    ::eadp::foundation::String m_status;
    ::eadp::foundation::String m_timestamp;
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> m_player;
    int m_basicPresenceType;
    ::eadp::foundation::String m_userDefinedPresence;
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RichPresenceV1> m_richPresence;
    ::eadp::foundation::String m_sessionKey;
    ::eadp::foundation::String m_clientVersion;
    int m_platform;

};

}
}
}
}
}
}
