///////////////////////////////////////////////////////////////////////////////
// Json.h
// 
// Copyright (c) 2009 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
///////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_JSON_H
#define EAJSON_JSON_H


#include <EAJson/internal/Config.h>
#include <coreallocator/icoreallocator_interface.h>
#include <EASTL/core_allocator_adapter.h>
#include <EASTL/string.h>


#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4251) // 'x' needs to have dll-interface to be used by clients of class 'y'.
#endif


namespace EA
{
    namespace Json
    {
        enum EventType
        {
            kETNone,                    /// 
            kETError,                   /// The reader is in an error state.

            // Simple types
            kETInteger,                 /// Represented as int64_t.
            kETDouble,                  /// Represented as double.
            kETBool,                    /// Represented as bool.
            kETString,                  /// Represented as a string, with surrounding quotes removed.
            kETNull,                    /// Has no representation.

            // Complex types
            kETBeginDocument,           /// The initial state before reading begins.
            kETEndDocument,             /// Always eventually follows kETEndDocument.
            kETBeginObject,             /// An object is a container zero or more of name:value pairs. 
            kETEndObject,               /// Always eventually follows kETBeginObject.
            kETBeginObjectValue,        /// Also known as "key" or "name" in some JSON APIs. A name string is associated with this event. Use GetName to get the name of the object value.
            kETBeginArray,              /// An array of zero or more values will follow, where a value is any of Object, Array, Integer, Double, Bool, String, Null.
            kETEndArray                 /// Always eventually follows kETBeginArray.
        };


        enum Result
        {
            kSuccess            = 0x00000000,   /// 
            kErrorInternal      = 0x2a8c0000,   /// decimal 713818112. Internal error. The library maintainer should be notified.
            kErrorEOF           = 0x2a8c0001,   /// decimal 713818113. The end of the source read stream was reached before the JSON content was complete.
            kErrorStream        = 0x2a8c0002,   /// decimal 713818114. The source read string returned an error.
            kErrorSyntax        = 0x2a8c0003,   /// decimal 713818115. The Json was malformed / invalid.
            kErrorMemory        = 0x2a8c0004,   /// decimal 713818116. Memory allocation failure.
            kErrorEncoding      = 0x2a8c0005    /// decimal 713818117. A string used an invalid character sequence (usually invalid UTF8).
            // If you add a new value here, make sure to update GetJsonReaderResultString. 
        };


        /// Allocator typedefs.
        typedef EA::Allocator::CoreAllocatorAdapter<EA::Allocator::ICoreAllocator> EASTLCoreAllocator;

        typedef eastl::basic_string<char8_t,  EASTLCoreAllocator> String8;
        typedef eastl::basic_string<char16_t, EASTLCoreAllocator> String16;


        /// Used as a unique argument for functions with the meaning of 'don't initialize this class'.
        struct NoInit{};


        /// Defines a read interface that the user provides for reading and writing data.
        /// This interface is the same as the Read function from the EAIO IStream interface.
        class EAJSON_API IReadStream
        {
        public:
            virtual ~IReadStream() { }

            /// Returns the number of bytes read.
            /// A return value of zero means the end of file was reached.
            /// A return value of (size_type)-1 means error.
            virtual size_type Read(void* pData, size_type nSize) = 0;
        };


        /// Defines an interface that the user provides for writing data.
        /// This interface is the same as the Write function from the EAIO IStream interface.
        class IWriteStream
        {
        public:
            virtual ~IWriteStream() { }

            virtual bool Write(const void* pData, size_type nSize) = 0;
        };


        /// Defines a write stream which writes to an STL or EASTL string.
        /// Example usage:
        ///     StringWriteStream<eastl::string> stream;
        ///     JsonWriter writer;
        ///     writer.SetStream(&stream);
        template <typename String>
        class StringWriteStream : public IWriteStream
        {
        public:
            StringWriteStream(){}
           ~StringWriteStream(){}

            bool Write(const void* pData, size_type nSize)
            {
                typedef typename String::size_type string_size_type;
                mString.append((const char8_t*)pData, (string_size_type)nSize);
                return true;
            }

            String mString;
        };


        /// Reads the IReadStream input JSON and writes equivalent JSON to pOutput with memory usage reduced.
        /// It accomplishes this by stripping whitespace, removing redundant numerical values (e.g. trailing fractional 0 digits), etc.
        /// Returns if the conversion can be done, which will be so if all of the data could be read from pInput and written to pOutput.
        /// This function requires that pInput represent valid JSON.
        bool WriteOptimizedJson(IReadStream* pInput, IWriteStream* pOutput, EA::Allocator::ICoreAllocator* pAllocator = NULL);


    } // namespace Json

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard








