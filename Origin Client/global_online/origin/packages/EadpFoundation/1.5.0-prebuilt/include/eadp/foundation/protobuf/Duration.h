// GENERATED CODE (Source: protobuf/duration.proto) - DO NOT EDIT DIRECTLY.
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
 *  A Duration represents a signed, fixed-length span of time represented
 *  as a count of seconds and fractions of seconds at nanosecond
 *  resolution. It is independent of any calendar and concepts like "day"
 *  or "month". It is related to Timestamp in that the difference between
 *  two Timestamp values is a Duration and it can be added or subtracted
 *  from a Timestamp. Range is approximately +-10,000 years.
 * 
 *  Example 1: Compute Duration from two Timestamps in pseudo code.
 * 
 *      Timestamp start = ...;
 *      Timestamp end = ...;
 *      Duration duration = ...;
 * 
 *      duration.seconds = end.seconds - start.seconds;
 *      duration.nanos = end.nanos - start.nanos;
 * 
 *      if (duration.seconds < 0 && duration.nanos > 0) {
 *        duration.seconds += 1;
 *        duration.nanos -= 1000000000;
 *      } else if (durations.seconds > 0 && duration.nanos < 0) {
 *        duration.seconds -= 1;
 *        duration.nanos += 1000000000;
 *      }
 * 
 *  Example 2: Compute Timestamp from Timestamp + Duration in pseudo code.
 * 
 *      Timestamp start = ...;
 *      Duration duration = ...;
 *      Timestamp end = ...;
 * 
 *      end.seconds = start.seconds + duration.seconds;
 *      end.nanos = start.nanos + duration.nanos;
 * 
 *      if (end.nanos < 0) {
 *        end.seconds -= 1;
 *        end.nanos += 1000000000;
 *      } else if (end.nanos >= 1000000000) {
 *        end.seconds += 1;
 *        end.nanos -= 1000000000;
 *      }
 * 
 * 
 */
class EADPSDK_API Duration : public ::eadp::foundation::Internal::ProtobufMessage
{
public:

    /*!
     * Constructs an empty message. Memory allocated by the message will use the provided allocator.
     */
    explicit Duration(const ::eadp::foundation::Allocator& allocator);

    /*!
     * Clear a message, so that all fields are reset to their default values.
     */
    void clear();

    /**@{
     *  Signed seconds of the span of time. Must be from -315,576,000,000
     *  to +315,576,000,000 inclusive.
     */
    int64_t getSeconds() const;
    void setSeconds(int64_t value);
    /**@}*/

    /**@{
     *  Signed fractions of a second at nanosecond resolution of the span
     *  of time. Durations less than one second are represented with a 0
     *  `seconds` field and a positive or negative `nanos` field. For durations
     *  of one second or more, a non-zero value for the `nanos` field must be
     *  of the same sign as the `seconds` field. Must be from -999,999,999
     *  to +999,999,999 inclusive.
     */
    int32_t getNanos() const;
    void setNanos(int32_t value);
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
    int64_t m_seconds;
    int32_t m_nanos;

};

}
}
}
