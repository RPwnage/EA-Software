// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/Player.h>
#include <com/ea/eadp/antelope/rtm/protocol/TypingEventType.h>

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

class EADPGENERATED_API ChatTypingEventV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit ChatTypingEventV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     */
    com::ea::eadp::antelope::rtm::protocol::TypingEventType getEvent() const;
    void setEvent(com::ea::eadp::antelope::rtm::protocol::TypingEventType value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getCustomTypingEvent() const;
    void setCustomTypingEvent(const ::eadp::foundation::String& value);
    void setCustomTypingEvent(const char8_t* value);
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> getFromPlayer();
    void setFromPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player>);
    bool hasFromPlayer();
    void releaseFromPlayer();
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getFromDisplayName() const;
    void setFromDisplayName(const ::eadp::foundation::String& value);
    void setFromDisplayName(const char8_t* value);
    /**@}*/

    /**@{
     */
    const ::eadp::foundation::String& getFromNickName() const;
    void setFromNickName(const ::eadp::foundation::String& value);
    void setFromNickName(const char8_t* value);
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
    int m_event;
    ::eadp::foundation::String m_customTypingEvent;
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player> m_fromPlayer;
    ::eadp::foundation::String m_fromDisplayName;
    ::eadp::foundation::String m_fromNickName;

};

}
}
}
}
}
}
