#include "framework/blaze.h"
#include "framework/protobuf/blazeprotobufwireformat.h"
#include "framework/logger.h"

namespace Blaze {
namespace protobuf {
namespace internal {


bool WireFormatLite::ReadString(google::protobuf::io::CodedInputStream* input, EA::TDF::TdfString& str)
{
    uint32_t strLength = 0;
    if (!input->ReadVarint32(&strLength))
    {
        BLAZE_ERR_LOG(Log::GRPC, "WireFormatLite::ReadString: Failed to read string length.");
        return false;
    }

    // Special case: Normally, if strLength == 0, the string does not get serialized over the wire by protobuf. However, with maps,
    // they do. In such scenarios, GetDirectBufferPointer will throw an error.
    if (strLength == 0)
    {
        str.set("", 0);
        return true;
    }

    if (input->BufferSize() >= static_cast<int>(strLength))
    {
        // fast path
        const void* void_pointer;
        int buffer_size;

        if (input->GetDirectBufferPointer(&void_pointer, &buffer_size))
        {
            const char8_t* buffer = reinterpret_cast<const char8_t*>(void_pointer);
            str.set(buffer, strLength);
            input->Advance(strLength);
            return true;
        }
    }
    else
    {
        // slow path
        // The request is obnoxiously large and does not fit in the grpc tcp buffer.
        // We use std string here in order to use the built-in function in protobuf for this situation. We can't use TdfString because it does not
        // provide an append operation which is required in this situation. The execution of this code path is edge case. 
        std::string largeStr;
        if (input->ReadStringFallback(&largeStr, strLength))
        {
            str.set(largeStr.c_str(), largeStr.length());
            return true;
        }
    }

    BLAZE_ERR_LOG(Log::GRPC, "WireFormatLite::ReadString: Failed to read string. Incoming length(" << strLength << ").");
    return false;
}

void WireFormatLite::WriteString(int field_number, const EA::TDF::TdfString& str, google::protobuf::io::CodedOutputStream* output)
{
    ::google::protobuf::internal::WireFormatLite::WriteTag(field_number, ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED, output);
    GOOGLE_CHECK_LE(str.length(), google::protobuf::kint32max);
    output->WriteVarint32(str.length());
    output->WriteRaw(str.c_str(), str.length());
}

google::protobuf::uint8* WireFormatLite::WriteStringToArray(int field_number, const EA::TDF::TdfString& str, google::protobuf::uint8* target)
{
    target = ::google::protobuf::internal::WireFormatLite::WriteTagToArray(field_number, ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED, target);
    GOOGLE_DCHECK_LE(str.length(), google::protobuf::kuint32max);
    target = google::protobuf::io::CodedOutputStream::WriteVarint32ToArray(str.length(), target);
    return google::protobuf::io::CodedOutputStream::WriteRawToArray(str.c_str(), static_cast<int>(str.length()), target);
}


bool WireFormatLite::ReadBytes(google::protobuf::io::CodedInputStream* input, EA::TDF::TdfBlob& blob)
{
    uint32_t blobLength = 0;
    if (!input->ReadVarint32(&blobLength))
    {
        BLAZE_ERR_LOG(Log::GRPC, "WireFormatLite::ReadBytes: Failed to read blob length.");
        return false;
    }

    if (blobLength == 0)
    {
        blob.setData(nullptr, 0);
        return true;
    }


    if (input->BufferSize() >= static_cast<int>(blobLength))
    {
        // fast path
        const void* void_pointer;
        int buffer_size;

        if (input->GetDirectBufferPointer(&void_pointer, &buffer_size))
        {
            const uint8_t* buffer = reinterpret_cast<const uint8_t*>(void_pointer);
            blob.setData(buffer, blobLength);
            input->Advance(blobLength);
            return true;
        }
    }
    else
    {
        // slow path
        // The request is obnoxiously large and does not fit in the grpc tcp buffer.
        // We use std string here in order to use the built-in function in protobuf for this situation. We can't use TdfBlob because it does not
        // provide an append operation which is required in this situation. The execution of this code path is edge case. 
        std::string largeStr;
        if (input->ReadStringFallback(&largeStr, blobLength))
        {
            blob.setData(reinterpret_cast<const uint8_t*>(largeStr.c_str()), largeStr.length());
            return true;
        }
    }

    BLAZE_ERR_LOG(Log::GRPC, "WireFormatLite::ReadBytes: Failed to read bytes. Incoming length(" << blobLength << ").");
    return false;
}

void WireFormatLite::WriteBytes(int field_number, const EA::TDF::TdfBlob& blob, google::protobuf::io::CodedOutputStream* output)
{
    ::google::protobuf::internal::WireFormatLite::WriteTag(field_number, ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED, output);
    GOOGLE_CHECK_LE(blob.getCount(), google::protobuf::kint32max);
    output->WriteVarint32(blob.getCount());
    output->WriteRaw(blob.getData(), blob.getCount());
}

google::protobuf::uint8* WireFormatLite::WriteBytesToArray(int field_number, const EA::TDF::TdfBlob& blob, google::protobuf::uint8* target)
{
    target = ::google::protobuf::internal::WireFormatLite::WriteTagToArray(field_number, ::google::protobuf::internal::WireFormatLite::WIRETYPE_LENGTH_DELIMITED, target);
    GOOGLE_DCHECK_LE(blob.getCount(), google::protobuf::kuint32max);
    target = google::protobuf::io::CodedOutputStream::WriteVarint32ToArray(blob.getCount(), target);
    return google::protobuf::io::CodedOutputStream::WriteRawToArray(blob.getData(), static_cast<int>(blob.getCount()), target);
}

}  // namespace internal
}  // namespace protobuf

}  // namespace Blaze
