///////////////////////////////////////////////////////////////////////////////
// BsonWriter.h
// 
// Copyright (c) 2011 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////


#ifndef EAJSON_BSONWRITER_H
#define EAJSON_BSONWRITER_H


#include <EAJson/internal/Config.h>
#include <EAJson/internal/BsonShared.h>
#include <EAJson/BsonReader.h>
#include <EAJson/JsonWriter.h>
#include <EASTL/fixed_vector.h>
#include <EASTL/fixed_string.h>


#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4251) // 'x' needs to have dll-interface to be used by clients of class 'y'.
#endif


namespace EA
{
    namespace Json
    {
        ///////////////////////////////////////////////////////////////////////
        // BsonWriter
        //
        // Implements a BSON-encoded writer.
        // This class allocates no memory, though the user-provided IWriteStream
        // may possibly do so. Actually it's possible that it could allocate 
        // memory in the case of using very long object names or very deep 
        // element nesting.
        //
        // Any failed return value means that the BsonWriter is henceforth in 
        // an unrecoverable state for the current document and most be abandoned
        // or Reset for a new use.
        //
        // Example usage:
        // This writes the JSON equivalent of the following doc:
        //     {
        //         "name" : "Joe",
        //         "age" : 33
        //     }
        //
        //     BsonWriter writer;
        //     writer.SetStream(pSomeStream);       // 
        //     writer.BeginDocument();              // 
        //     writer.BeginObject();                // {
        //     writer.BeginObjectValue("name");     // "name" : 
        //     writer.String("Joe");                // "Joe",
        //     writer.BeginObjectValue("age");      // "age" :
        //     writer.Int32(33);                    // 33
        //     writer.EndObject();                  // }
        //     writer.EndDocument();                // 
        // 
        // Example usage:
        // This writes the JSON equivalent of the following doc:
        //     {
        //         "BSON" : ["awesome", 5.05, 1986]
        //     }
        // 
        //     BsonWriter writer;
        //     writer.SetStream(pSomeStream);       // 
        //     writer.BeginDocument();              // 
        //     writer.BeginObject();                // {
        //     writer.BeginObjectValue("BSON");     // "BSON" :
        //     writer.BeginArray();                 // [
        //     writer.String("awesome");            // "awesome",
        //     writer.Double(5.05);                 // 5.05,
        //     writer.Int32(1986);                  // 1986
        //     writer.EndArray();                   // ]
        //     writer.EndObject();                  // }
        //     writer.EndDocument();                //  
        ///////////////////////////////////////////////////////////////////////

        class EAJSON_API BsonWriter
        {
        public:
            BsonWriter(EA::Allocator::ICoreAllocator* pAllocator = NULL);
            virtual ~BsonWriter();

            EA::Allocator::ICoreAllocator* GetAllocator() const;
            void SetAllocator(EA::Allocator::ICoreAllocator* pAllocator);

            // There is no need to call Reset unless you plan on writing another document.
            void Reset();

            enum FormatOption
            {
                kFormatOptionEndian = 0,    /// One of enum EA::Json::Endian. Specifies the endian-ness of written values. Default is kEndianLittle, as that's what the BSON spec calls for.
                kFormatOptionCount  = 1
            };

            void SetFormatOption(int option, int value);
            int  GetFormatOption(int option) const;

            void SetStream(IBsonWriteStream* pStream);

            bool BeginDocument();
            bool EndDocument();

            bool BeginObject();
            bool BeginObjectValue(const char8_t* pName, size_t nStrlen = (size_t)-1); // You are not expected to provide the outer quotes for pName; they will be written for you. This function should be followed by Integer, String, Bool, Null, BeginObject, or BeginArray.
            bool EndObject();

            bool BeginArray();
            bool EndArray();

            // BeginValue is called before Integer, Double, Float Bool, String, etc., BeginObject, BeginArray.
            bool Integer(int64_t value);
            bool Double(double value);
            bool Bool(bool value);
            bool String(const char8_t* pValue, size_t nStrlen = (size_t)-1); // You are not expected to provide the enclosing " chars. You are not required to encode the string, but you can.
            bool Null();

            // Extension types to JSON.
            bool Binary(uint8_t id, const uint8_t* pData, size_t nSize);
            bool Float(float value);
            bool Int32(int32_t value);
            bool Int16(int16_t value);
            bool Int8(int8_t value);
          //bool UTCDatetime(int64_t timeMS);                                    // To do. See http://bsonspec.org/#/specification
          //bool Regex(const char8_t* pValue, size_t nStrlen = (size_t)-1);      // To do.
          //bool JavaScript(const char8_t* pValue, size_t nStrlen = (size_t)-1); // To do.

        protected:
            struct StackEntry
            {
                Internal::BsonElementType mElementType;     /// The type of the current level (kBETObject or kBETArray).
                int32_t                   mBeginPosition;   /// Where in the stream this element begins. Needed so we can know where to write the size field when we've completed writing an array or object.
                int32_t                   mArrayIndex;      /// Used in the case that mElementType == kBETArray.

                StackEntry(Internal::BsonElementType et = Internal::kBLTNone, int32_t streamPosition = -1) : mElementType(et), mBeginPosition(streamPosition), mArrayIndex(0) { }
            };

            typedef eastl::fixed_vector<StackEntry, 64, true, EASTLCoreAllocator> StackEntryArray; 
            typedef eastl::fixed_string<char8_t, 32, true, EASTLCoreAllocator> FixedString; 

        protected:
            bool Begin(Internal::BsonElementType eventType);
            bool BeginValue(Internal::BsonElementType eventType, const char8_t* pName, size_t nStrlen);
            bool End(Internal::BsonElementType eventType);
            bool WriteName();
            bool WriteNameBase();
            bool WriteInt16(int16_t value);
            bool WriteInt32(int32_t value);
            bool WriteInt64(int64_t value);
            bool WriteDouble(double value);
            bool WriteFloat(float value);

        protected:
            EA::Allocator::ICoreAllocator*  mpAllocator;        /// Allocation pool we're drawing from.
            StackEntryArray                 mStack;             /// Used for keeping track of where the prefix size value will go when we finish objects and arrays.
            IBsonWriteStream*               mpStream;           /// 
            int32_t                         mBeginPosition;     /// 
            FixedString                     mSavedName;         /// Used to save the name specified by BeginObjectValue until it needs to be written.
            Endian                          mEndian;            /// Endian-ness of stream we write.
        };

    } // namespace Json

} // namespace EA




#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard
