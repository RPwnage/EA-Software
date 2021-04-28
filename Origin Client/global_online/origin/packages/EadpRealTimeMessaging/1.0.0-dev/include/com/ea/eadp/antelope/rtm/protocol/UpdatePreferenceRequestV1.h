// GENERATED CODE (Source: preference_protocol.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <com/ea/eadp/antelope/rtm/protocol/TranslationPreferenceV1.h>

namespace com { namespace ea { namespace eadp { namespace antelope { namespace rtm { namespace protocol { class TranslationPreferenceV1;  } } } } } }

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

class EADPGENERATED_API UpdatePreferenceRequestV1 : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit UpdatePreferenceRequestV1(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    ~UpdatePreferenceRequestV1()
    {
        clearBody();
    }

    /**@{
     */
    ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1> getTranslation();
    void setTranslation(::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1>);
    bool hasTranslation() const;
    void releaseTranslation();
    /**@}*/

    /*!
     * Case values for the fields potentially set on the associated union.
     */
    enum class BodyCase
    {
        kNotSet = 0,
        kTranslation = 1,
    };

    /*!
     * Returns the case corresponding to the currently set union field.
     */
    BodyCase getBodyCase() { return static_cast<BodyCase>(m_bodyCase); }

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

    BodyCase m_bodyCase;

    union BodyUnion
    {
        ::eadp::foundation::SharedPtr<com::ea::eadp::antelope::rtm::protocol::TranslationPreferenceV1> m_translation;

        BodyUnion() {}
        ~BodyUnion() {}
    } m_body;

    void clearBody();

};

}
}
}
}
}
}
