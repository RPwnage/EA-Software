// GENERATED CODE (Source: chat_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/PersonaV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PlayerInfo.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class PersonaV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class PlayerInfo;  } } } } } }

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

class EADPGENERATED_API ChatUserUnmutedV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit ChatUserUnmutedV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     */
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> getUnmutedUser();
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR void setUnmutedUser(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1>);
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR bool hasUnmutedUser();
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR void releaseUnmutedUser();
    /**@}*/

    /**@{
     */
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> getUnmutedByUser();
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR void setUnmutedByUser(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1>);
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR bool hasUnmutedByUser();
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR void releaseUnmutedByUser();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> getUnmutedPlayer();
    void setUnmutedPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo>);
    bool hasUnmutedPlayer();
    void releaseUnmutedPlayer();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> getUnmutedByPlayer();
    void setUnmutedByPlayer(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo>);
    bool hasUnmutedByPlayer();
    void releaseUnmutedByPlayer();
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
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> m_unmutedUser;
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1> m_unmutedByUser;
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> m_unmutedPlayer;
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PlayerInfo> m_unmutedByPlayer;

};

}
}
}
}
}
}
