// GENERATED CODE (Source: protobuf/struct.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>
#include <eadp/foundation/protobuf/ListValue.h>
#include <eadp/foundation/protobuf/NullValue.h>
#include <eadp/foundation/protobuf/Struct.h>

namespace eadp { namespace foundation { namespace protobuf { class ListValue;  } } }
namespace eadp { namespace foundation { namespace protobuf { class Struct;  } } }

namespace eadp
{

namespace foundation
{

namespace protobuf
{

/**
 *  `Value` represents a dynamically typed value which can be either
 *  null, a number, a string, a boolean, a recursive struct value, or a
 *  list of values. A producer of value is expected to set one of that
 *  variants, absence of any variant indicates an error.
 * 
 *  The JSON representation for `Value` is JSON value.
 */
class EADPSDK_API Value : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit Value(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    ~Value()
    {
        clearKind();
    }

    /**@{
     *  Represents a null value.
     */
    eadp::foundation::protobuf::NullValue getNullValue() const;
    bool hasNullValue() const;
    void setNullValue(eadp::foundation::protobuf::NullValue value);
    /**@}*/

    /**@{
     *  Represents a double value.
     */
    double getNumberValue() const;
    bool hasNumberValue() const;
    void setNumberValue(double value);
    /**@}*/

    /**@{
     *  Represents a string value.
     */
    const ::eadp::foundation::String& getStringValue();
    bool hasStringValue() const;
    void setStringValue(const ::eadp::foundation::String& value);
    void setStringValue(const char8_t* value);
    /**@}*/

    /**@{
     *  Represents a boolean value.
     */
    bool getBoolValue() const;
    bool hasBoolValue() const;
    void setBoolValue(bool value);
    /**@}*/

    /**@{
     *  Represents a structured value.
     */
    ::eadp::foundation::SharedPtr<eadp::foundation::protobuf::Struct> getStructValue();
    void setStructValue(::eadp::foundation::SharedPtr<eadp::foundation::protobuf::Struct>);
    bool hasStructValue() const;
    void releaseStructValue();
    /**@}*/

    /**@{
     *  Represents a repeated `Value`.
     */
    ::eadp::foundation::SharedPtr<eadp::foundation::protobuf::ListValue> getListValue();
    void setListValue(::eadp::foundation::SharedPtr<eadp::foundation::protobuf::ListValue>);
    bool hasListValue() const;
    void releaseListValue();
    /**@}*/

    /*!
     * Case values for the fields potentially set on the associated union.
     */
    enum class KindCase
    {
        kNotSet = 0,
        kNullValue = 1,
        kNumberValue = 2,
        kStringValue = 3,
        kBoolValue = 4,
        kStructValue = 5,
        kListValue = 6,
    };

    /*!
     * Returns the case corresponding to the currently set union field.
     */
    KindCase getKindCase() { return static_cast<KindCase>(m_kindCase); }

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

    KindCase m_kindCase;

    union KindUnion
    {
        eadp::foundation::protobuf::NullValue m_nullValue;
        double m_numberValue;
        ::eadp::foundation::String m_stringValue;
        bool m_boolValue;
        ::eadp::foundation::SharedPtr<eadp::foundation::protobuf::Struct> m_structValue;
        ::eadp::foundation::SharedPtr<eadp::foundation::protobuf::ListValue> m_listValue;

        KindUnion() {}
        ~KindUnion() {}
    } m_kind;

    void clearKind();

};

}
}
}
