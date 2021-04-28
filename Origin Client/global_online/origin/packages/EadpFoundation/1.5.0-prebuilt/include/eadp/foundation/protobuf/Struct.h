// GENERATED CODE (Source: protobuf/struct.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <eadp/foundation/protobuf/Value.h>

namespace eadp { namespace foundation { namespace protobuf { class Value;  } } }

namespace eadp
{

namespace foundation
{

namespace protobuf
{

/**
 *  `Struct` represents a structured data value, consisting of fields
 *  which map to dynamically typed values. In some languages, `Struct`
 *  might be supported by a native representation. For example, in
 *  scripting languages like JS a struct is represented as an
 *  object. The details of that representation are described together
 *  with the proto support for the language.
 * 
 *  The JSON representation for `Struct` is JSON object.
 */
class EADPSDK_API Struct : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    class EADPSDK_API FieldsEntry : public ::eadp::foundation::Internal::ProtobufMessage
    {
    public:

        /*!
         * Constructs an empty message. Memory allocated by the message will use the provided allocator.
         */
        explicit FieldsEntry(const ::eadp::foundation::Allocator& allocator);

        /*!
         * Clear a message, so that all fields are reset to their default values.
         */
        void clear();

        /**@{
         */
        const ::eadp::foundation::String& getKey() const;
        void setKey(const ::eadp::foundation::String& value);
        void setKey(const char8_t* value);
        /**@}*/

        /**@{
         */
        ::eadp::foundation::SharedPtr<eadp::foundation::protobuf::Value> getValue();
        void setValue(::eadp::foundation::SharedPtr<eadp::foundation::protobuf::Value>);
        bool hasValue();
        void releaseValue();
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
        ::eadp::foundation::String m_key;
        ::eadp::foundation::SharedPtr<eadp::foundation::protobuf::Value> m_value;

    };

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit Struct(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     *  Unordered map of dynamically typed values.
     */
    ::eadp::foundation::Map< ::eadp::foundation::String, ::eadp::foundation::SharedPtr<eadp::foundation::protobuf::Value>>& getFields();
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
    ::eadp::foundation::Map< ::eadp::foundation::String, ::eadp::foundation::SharedPtr<eadp::foundation::protobuf::Value>> m_fields;

};

}
}
}
