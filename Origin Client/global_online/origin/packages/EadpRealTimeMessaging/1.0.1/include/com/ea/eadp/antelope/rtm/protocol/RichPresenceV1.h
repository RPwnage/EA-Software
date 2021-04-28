// GENERATED CODE (Source: presence_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/PlatformV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/RichPresenceType.h>

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

class EADPGENERATED_API RichPresenceV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit RichPresenceV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     *  What game the player is currently playing
     */
    const ::eadp::foundation::String& getGame() const;
    void setGame(const ::eadp::foundation::String& value);
    void setGame(const char8_t* value);
    /**@}*/

    /**@{
     *  Which platform is the game being played, this is an optional field that the clients do not need to send with every update but we will be returning this field as a part of the presence returned from the server
     * (we will add the platform to the new login request as well)
     */
    com::ea::eadp::antelope::rtm::protocol::PlatformV1 getPlatform() const;
    void setPlatform(com::ea::eadp::antelope::rtm::protocol::PlatformV1 value);
    /**@}*/

    /**@{
     *  The mode type for the current game session
     */
    const ::eadp::foundation::String& getGameModeType() const;
    void setGameModeType(const ::eadp::foundation::String& value);
    void setGameModeType(const char8_t* value);
    /**@}*/

    /**@{
     *  The game mode for the current game session
     */
    const ::eadp::foundation::String& getGameMode() const;
    void setGameMode(const ::eadp::foundation::String& value);
    void setGameMode(const char8_t* value);
    /**@}*/

    /**@{
     *  The current game session info, that has all the encoded info to allow other players to join the game
     */
    const ::eadp::foundation::String& getGameSessionData() const;
    void setGameSessionData(const ::eadp::foundation::String& value);
    void setGameSessionData(const char8_t* value);
    /**@}*/

    /**@{
     *  Rich presence type is an enum that represents the client current rich presence which can be one of LFG or BROADCASTING currently
     */
    com::ea::eadp::antelope::rtm::protocol::RichPresenceType getRichPresenceType() const;
    void setRichPresenceType(com::ea::eadp::antelope::rtm::protocol::RichPresenceType value);
    /**@}*/

    /**@{
     *  Start timestamp to display fields like xx:xx time elapsed in the current state
     */
    const ::eadp::foundation::String& getStartTimestamp() const;
    void setStartTimestamp(const ::eadp::foundation::String& value);
    void setStartTimestamp(const char8_t* value);
    /**@}*/

    /**@{
     *  End timestamp to display fields like xx:xx time remaining in the current state
     */
    const ::eadp::foundation::String& getEndTimestamp() const;
    void setEndTimestamp(const ::eadp::foundation::String& value);
    void setEndTimestamp(const char8_t* value);
    /**@}*/

    /**@{
     *  Generic string field to allow custom game specific rich presence data to be sent as some encoding
     */
    const ::eadp::foundation::String& getCustomRichPresenceData() const;
    void setCustomRichPresenceData(const ::eadp::foundation::String& value);
    void setCustomRichPresenceData(const char8_t* value);
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
    ::eadp::foundation::String m_game;
    int m_platform;
    ::eadp::foundation::String m_gameModeType;
    ::eadp::foundation::String m_gameMode;
    ::eadp::foundation::String m_gameSessionData;
    int m_richPresenceType;
    ::eadp::foundation::String m_startTimestamp;
    ::eadp::foundation::String m_endTimestamp;
    ::eadp::foundation::String m_customRichPresenceData;

};

}
}
}
}
}
}
