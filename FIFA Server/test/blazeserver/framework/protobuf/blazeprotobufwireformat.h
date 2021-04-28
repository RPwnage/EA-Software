/*************************************************************************************************/
/*!
\file   blazeprotobufwireformat.h

\attention
(c) Electronic Arts Inc. 2017
*/
/*************************************************************************************************/

/*************************************************************************************************/
/*!
\class WireFormatLite

Handles Protobuf Wire Format implementation for custom Blaze types. This code mostly mimics the implementation
of similar data types in the Protocol Buffers source code.
*/

#ifndef BLAZE_PROTOBUF_WIRE_FORMAT_H__
#define BLAZE_PROTOBUF_WIRE_FORMAT_H__

#include <google/protobuf/stubs/port.h>
#include <google/protobuf/wire_format_lite_inl.h>
#include <google/protobuf/io/coded_stream.h>

#include <EATDF/tdfstring.h>
#include <EATDF/tdfblob.h>

namespace Blaze {
namespace protobuf {
namespace internal {
    
class WireFormatLite {
public:
    // EA::TDF::String support
    static inline int StringSize(const EA::TDF::TdfString& str);
    static bool ReadString(google::protobuf::io::CodedInputStream* input, EA::TDF::TdfString& str);
    static void WriteString(int field_number, const EA::TDF::TdfString& str, google::protobuf::io::CodedOutputStream* output);
    static google::protobuf::uint8* WriteStringToArray(int field_number, const EA::TDF::TdfString& str, google::protobuf::uint8* target);

    // EA::TDF::TdfBlob support
    static inline int BytesSize(const EA::TDF::TdfBlob& blob);
    static bool ReadBytes(google::protobuf::io::CodedInputStream* input, EA::TDF::TdfBlob& blob);
    static void WriteBytes(int field_number, const EA::TDF::TdfBlob& blob, google::protobuf::io::CodedOutputStream* output);
    static google::protobuf::uint8* WriteBytesToArray(int field_number, const EA::TDF::TdfBlob& blob, google::protobuf::uint8* target);

    // Primitive list/vector/repeated field support
    template <typename CType, enum google::protobuf::internal::WireFormatLite::FieldType DeclaredType, typename Container>
    static bool ReadPackedPrimitive(google::protobuf::io::CodedInputStream* input, Container& values);
    

};

inline int WireFormatLite::StringSize(const EA::TDF::TdfString& str) {
    return static_cast<int>(
      google::protobuf::io::CodedOutputStream::VarintSize32(static_cast<google::protobuf::uint32>(str.length())) +
      str.length());
}

inline int WireFormatLite::BytesSize(const EA::TDF::TdfBlob& blob) {
    return static_cast<int>(
        google::protobuf::io::CodedOutputStream::VarintSize32(static_cast<google::protobuf::uint32>(blob.getCount())) +
        blob.getCount());
}


template <typename CType, enum google::protobuf::internal::WireFormatLite::FieldType DeclaredType, typename Container>
bool WireFormatLite::ReadPackedPrimitive(google::protobuf::io::CodedInputStream* input, Container& values)
{
    int length;
    if (!input->ReadVarintSizeAsInt(&length)) return false;
    google::protobuf::io::CodedInputStream::Limit limit = input->PushLimit(length);
    while (input->BytesUntilLimit() > 0) {
        CType value;
        if (!google::protobuf::internal::WireFormatLite::ReadPrimitive<CType, DeclaredType>(input, &value))
            return false;
        values.push_back(static_cast<typename Container::value_type>(value));
    }
    input->PopLimit(limit);
    
    return true;

}

}  // namespace internal
}  // namespace protobuf

}  // namespace Blaze
#endif  // BLAZE_PROTOBUF_WIRE_FORMAT_H__
