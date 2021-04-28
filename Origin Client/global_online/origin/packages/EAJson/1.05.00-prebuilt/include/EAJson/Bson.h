///////////////////////////////////////////////////////////////////////////////
// Bson.h
// 
// Copyright (c) 2011 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_BSON_H
#define EAJSON_BSON_H


#include <EAJson/internal/Config.h>
#include <EAJson/Json.h>


namespace EA
{
    namespace Json
    {

        enum BsonEventType
        {
            kBETNone,                    /// 
            kBETError,                   /// The reader is in an error state.

            // Simple types
            kBETInteger,                 /// Represented as int64_t.
            kBETInt32,                   /// Represented as int32_t.
            kBETInt16,                   /// Represented as int16_t.
            kBETInt8,                    /// Represented as int8_t.
            kBETDouble,                  /// Represented as double.
            kBETFloat,                   /// Represented as float.
            kBETBool,                    /// Represented as bool.
            kBETString,                  /// Represented as a string, with surrounding quotes removed.
            kBETNull,                    /// Has no representation.
            kBETBinary,                  /// Blob of binary user-defined data.
            kBETUTCDatetime,             /// int64_t milliseconds since 1970 epoch in UTC. i.e. milliseconds since Jan 1, 1970 in Greenwich.
            kBETRegex,                   /// 
            kBETJavaScript,              /// 
            kBETObjectId,                /// Represented as a string of 12 uint8_t.
            kBETDBPointer,               /// Represented as a string of 12 uint8_t.
            kBETSymbol,                  /// 
            kBETTimestamp,               /// 
          //kBETMaxKey,                  /// 
          //kBETMinKey,                  /// 

            // Complex types
            kBETBeginDocument,           /// The initial state before reading begins.
            kBETEndDocument,             /// Always eventually follows kETEndDocument.
            kBETBeginObject,             /// An object is a container zero or more of name:value pairs. 
            kBETEndObject,               /// Always eventually follows kETBeginObject.
            kBETBeginJavaScriptObject,   /// See BSON "JavaScript code w/ scope"
            kBETEndJavaScriptObject,     /// Always eventually follows kBETEndJavaScriptObject.
            kBETBeginObjectValue,        /// Also known as "key" or "name" in some JSON APIs. A name string is associated with this event. Use GetName to get the name of the object value.
            kBETBeginArray,              /// An array of zero or more values will follow, where a value is any of Object, Array, Integer, Double, Bool, String, Null.
            kBETEndArray                 /// Always eventually follows kETBeginArray.
        };


        enum BsonBinarySubtype
        {
            kBBSGeneric     = 0x00,     /// 
            kBBSFunction    = 0x01,     /// 
            kBBSBinaryOld   = 0x02,     /// 
            kBBSUuid        = 0x03,     /// 
            kBBSMd5         = 0x05,     /// 
            kBBSUserDefined = 0x80      /// Includes any value >= 0x80.
        };


        /// Defines an interface that the user provides for writing data.
        /// This interface is nearly the same as the Write function from the EAIO IStream interface.
        class IBsonWriteStream : public IWriteStream
        {
        public:
            virtual bool     Write(const void* pData, size_type nSize) = 0;     // This can fail, such as the case of exhausting space or memory.
            virtual off_type GetPosition() const = 0;                           // This must always succeed.
            virtual bool     SetPosition(off_type position) = 0;                // This must always succeed for positions within the existing file size.
        };


        /// enum Endian
        /// Defines endian-ness. This is appropriate for working with binary numerical data. 
        /// This is the same as the EAIO endian enumeration.
        enum Endian
        {
            kEndianBig       = 0,            /// Big endian.
            kEndianLittle    = 1,            /// Little endian.
            #ifdef EA_SYSTEM_BIG_ENDIAN
                kEndianLocal = kEndianBig    /// Whatever endian is native to the machine.
            #else 
                kEndianLocal = kEndianLittle /// Whatever endian is native to the machine.
            #endif
        };


    } // namespace Json

} // namespace EA


#endif // Header include guard








