// GENERATED CODE (Source: common_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/MutedSetV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/PersonaV1.h>
#include <com/ea/eadp/antelope/rtm/protocol/Player.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class MutedSetV1;  } } } } } }
namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class PersonaV1;  } } } } } }
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

class EADPGENERATED_API ChannelMuteListV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit ChannelMuteListV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     */
    const ::eadp::foundation::String& getChannelId() const;
    void setChannelId(const ::eadp::foundation::String& value);
    void setChannelId(const char8_t* value);
    /**@}*/

    /**@{
     */
    EADP_FOUNDATION_PROTOBUF_DEPRECATED_ATTR ::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1>>& getMutedUsers();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::MutedSetV1>>& getMutedBy();
    /**@}*/

    /**@{
     */
    ::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player>>& getMutedPlayers();
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
    ::eadp::foundation::String m_channelId;
    ::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::PersonaV1>> m_mutedUsers;
    ::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::MutedSetV1>> m_mutedBy;
    ::eadp::foundation::Vector< ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::Player>> m_mutedPlayers;

};

}
}
}
}
}
}
