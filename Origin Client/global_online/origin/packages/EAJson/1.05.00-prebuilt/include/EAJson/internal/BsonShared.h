///////////////////////////////////////////////////////////////////////////////
// BsonShared.h
// 
// Copyright (c) 2011 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_INTERNAL_BSONSHARED_H
#define EAJSON_INTERNAL_BSONSHARED_H


#include <EAJson/internal/Config.h>
#include <EAJson/Json.h>


namespace EA
{
    namespace Json
    {
        namespace Internal
        {
            // BsonElementType
            // This enum is similar to the Json::EventType enum, but this enum is used for 
            // tagging fields in the binary image format. This enum also identifies some
            // of the extension types that BSON defines by JSON does not. We have decided
            // to simply make this be a separate enum from Json::EventType.
            // See http://bsonspec.org/#/specification for definitions of the element types.
            //
            // To consider: Make this enum internal to Bson, as the user doesn't need to see it.
            //
            // To consider: Use Json::EventType instead of this, which will involve adding
            // the extensions below to EventType (and documenting them as being BSON-specific).
            // This will also involve defining the hex values below through a little lookup
            // table keyed off of EventType instead of directly with the enum name as below.
            enum BsonElementType
            {
                kBLTNone        = 0x00,
                kBLTObject      = 0x03,    /// The BSON spec calls this "Document", which is confusing and misleading and inconsistent with the mother JSON spec.
                kBLTArray       = 0x04,
                kBLTInt64       = 0x12,
                kBLTDouble      = 0x01,
                kBLTBool        = 0x08,
                kBLTString      = 0x02,
                kBLTNull        = 0x0a,

                // BSON extension to JSON. A couple of the Mongo-specifiec types and a couple deprecated types aren't listed here.
                kBLTBinary      = 0x05,    /// User-defined blob.
                kBLTUTCDatetime = 0x09,    /// Milliseconds since 1970 UTC.
                kBLTRegex       = 0x0b,
                kBLTJavaScript  = 0x0d,
                kBLTObjectId    = 0x07,
                kBLTDBPointer   = 0x0c,
                kBLTSymbol      = 0x0e,
                kBLTJavaScriptS = 0x0f,    /// JavaScript with scope.
                kBLTInt32       = 0x10,
                kBLTTimestamp   = 0x11,    /// Stored as int64_t.
                kBLTInt16       = 0x20,    /// Not part of the Mongo BSON spec.
                kBLTInt8        = 0x21,    /// Not part of the Mongo BSON spec.
                kBLTFloat       = 0x22     /// Not part of the Mongo BSON spec.
              //kBLTMaxKey      = 0x7f,    /// Taken as an int8_t, this is the max possible element type value (127). Question: Can this actually appear in a document?
              //kBLTMinKey      = 0xff     /// Taken as an int8_t, this is the min possible element type value (-128). Question: Can this actually appear in a document?
            };

        } // namespace Internal

    } // namespace Json

} // namespace EA


#endif // Header include guard








