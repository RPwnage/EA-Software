// GENERATED CODE (Source: protobuf/any.proto) - DO NOT EDIT DIRECTLY.
// Copyright(C) 2016-2019 Electronic Arts, Inc. All rights reserved.

#pragma once

#include <eadp/foundation/Allocator.h>
#include <eadp/foundation/Service.h>
#include <eadp/foundation/internal/ProtobufMessage.h>

namespace eadp
{

namespace foundation
{

namespace protobuf
{

/**
 *  `Any` contains an arbitrary serialized protocol buffer message along with a
 *  URL that describes the type of the serialized message.
 * 
 *  Protobuf library provides support to pack/unpack Any values in the form
 *  of utility functions or additional generated methods of the Any type.
 * 
 *  Example 1: Pack and unpack a message in C++.
 * 
 *      Foo foo = ...;
 *      Any any;
 *      any.PackFrom(foo);
 *      ...
 *      if (any.UnpackTo(&foo)) {
 *        ...
 *      }
 * 
 *  Example 2: Pack and unpack a message in Java.
 * 
 *      Foo foo = ...;
 *      Any any = Any.pack(foo);
 *      ...
 *      if (any.is(Foo.class)) {
 *        foo = any.unpack(Foo.class);
 *      }
 * 
 *   Example 3: Pack and unpack a message in Python.
 * 
 *      foo = Foo(...)
 *      any = Any()
 *      any.Pack(foo)
 *      ...
 *      if any.Is(Foo.DESCRIPTOR):
 *        any.Unpack(foo)
 *        ...
 * 
 *  The pack methods provided by protobuf library will by default use
 *  'type.googleapis.com/full.type.name' as the type URL and the unpack
 *  methods only use the fully qualified type name after the last '/'
 *  in the type URL, for example "foo.bar.com/x/y.z" will yield type
 *  name "y.z".
 * 
 * 
 *  JSON
 *  ====
 *  The JSON representation of an `Any` value uses the regular
 *  representation of the deserialized, embedded message, with an
 *  additional field `@type` which contains the type URL. Example:
 * 
 *      package google.profile;
 *      message Person {
 *        string first_name = 1;
 *        string last_name = 2;
 *      }
 * 
 *      {
 *        "@type": "type.googleapis.com/google.profile.Person",
 *        "firstName": <string>,
 *        "lastName": <string>
 *      }
 * 
 *  If the embedded message type is well-known and has a custom JSON
 *  representation, that representation will be embedded adding a field
 *  `value` which holds the custom JSON in addition to the `@type`
 *  field. Example (for message [google.protobuf.Duration][]):
 * 
 *      {
 *        "@type": "type.googleapis.com/google.protobuf.Duration",
 *        "value": "1.212s"
 *      }
 * 
 */
class EADPSDK_API Any : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit Any(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     *  A URL/resource name whose content describes the type of the
     *  serialized protocol buffer message.
     * 
     *  For URLs which use the scheme `http`, `https`, or no scheme, the
     *  following restrictions and interpretations apply:
     * 
     *  * If no scheme is provided, `https` is assumed.
     *  * The last segment of the URL's path must represent the fully
     *    qualified name of the type (as in `path/google.protobuf.Duration`).
     *    The name should be in a canonical form (e.g., leading "." is
     *    not accepted).
     *  * An HTTP GET on the URL must yield a [google.protobuf.Type][]
     *    value in binary format, or produce an error.
     *  * Applications are allowed to cache lookup results based on the
     *    URL, or have them precompiled into a binary to avoid any
     *    lookup. Therefore, binary compatibility needs to be preserved
     *    on changes to types. (Use versioned type names to manage
     *    breaking changes.)
     * 
     *  Schemes other than `http`, `https` (or the empty scheme) might be
     *  used with implementation specific semantics.
     * 
     */
    const ::eadp::foundation::String& getTypeUrl() const;
    void setTypeUrl(const ::eadp::foundation::String& value);
    void setTypeUrl(const char8_t* value);
    /**@}*/

    /**@{
     *  Must be a valid serialized protocol buffer of the above specified type.
     */
    const ::eadp::foundation::String& getValue() const;
    void setValue(const ::eadp::foundation::String& value);
    void setValue(const char8_t* value);
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

    /*!
     * Packs the specified message using the default url prefix.
     */
    void packFrom(::eadp::foundation::Internal::ProtobufMessage& message);

    /*!
     * Packs the specified message using the specified url prefix.
     */
    void packFrom(::eadp::foundation::Internal::ProtobufMessage& message, const char8_t* type_url_prefix);

    /*!
     * Unpacks into the specified message.
     *
     * @return true on success, false if deserialization fails or the message is the incorrect type.
     */
    bool unpackTo(::eadp::foundation::Internal::ProtobufMessage* message);

private:
    ::eadp::foundation::Allocator m_allocator_;
    size_t m_cachedByteSize_;
    ::eadp::foundation::String m_typeUrl;
    ::eadp::foundation::String m_value;

};

}
}
}
