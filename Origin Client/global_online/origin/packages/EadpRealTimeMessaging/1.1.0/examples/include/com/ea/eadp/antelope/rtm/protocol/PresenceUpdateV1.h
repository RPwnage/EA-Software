// GENERATED CODE (Source: presence_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/BasicPresenceType.h>
#include <com/ea/eadp/antelope/rtm/protocol/RichPresenceV1.h>

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

class EADPGENERATED_API PresenceUpdateV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit PresenceUpdateV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     * existing field
     *  game specific generic extensible string
     */
    const ::eadp::foundation::String& getStatus() const;
    void setStatus(const ::eadp::foundation::String& value);
    void setStatus(const char8_t* value);
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
    ::eadp::foundation::String m_status;
    int m_basicPresenceType;
    ::eadp::foundation::String m_userDefinedPresence;
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::RichPresenceV1> m_richPresence;

};

}
}
}
}
}
}
