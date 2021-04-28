// GENERATED CODE (Source: social_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/protocol/MutedBy.h>

namespace com
{

namespace ea
{

namespace eadp
{

namespace antelope
{

namespace protocol
{

class EADPGENERATED_API MutedUser : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit MutedUser(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     * Having persona is sufficient here as this is only used in LoginV0
     */
    const ::eadp::foundation::String& getPersonaId() const;
    void setPersonaId(const ::eadp::foundation::String& value);
    void setPersonaId(const char8_t* value);
    /**@}*/

    /**@{
     */
    ::eadp::foundation::Vector< com::ea::eadp::antelope::protocol::MutedBy>& getMutedBy();
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
    ::eadp::foundation::Vector< com::ea::eadp::antelope::protocol::MutedBy> m_mutedBy;
    int32_t m_mutedBy__cachedByteSize;

};

}
}
}
}
}
