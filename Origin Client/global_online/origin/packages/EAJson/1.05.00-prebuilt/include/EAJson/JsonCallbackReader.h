/////////////////////////////////////////////////////////////////////////////
// JsonCallbackReader.h
//
// Copyright (c) 2009, Electronic Arts Inc. All rights reserved.
// Written by Paul Pedriana
//
// SAX-style callback parser for Json.
// See http://www.json.org/ and http://www.ietf.org/rfc/rfc4627.txt
/////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_JSONCALLBACKREADER_H
#define EAJSON_JSONCALLBACKREADER_H


#include <EAJson/internal/Config.h>
#include <EAJson/JsonReader.h>


namespace EA
{
    namespace Json
    {
        //////////////////////////////////////////////////////////////////////////
        /// IJsonReaderCallback
        ///
        /// Callback interface for JSON events.
        /// A return value of false from any of the functions causes the iteration
        /// to be stopped and the JsonCallbackReader::Parse function to return 
        /// with the last Result before stopping.
        /// The BeginDocument is always the first called function, and EndDocument
        /// is always the last called function.
        //////////////////////////////////////////////////////////////////////////

        class EAJSON_API IJsonReaderCallback
        {
        public:
            virtual ~IJsonReaderCallback() {}

            virtual bool BeginDocument() = 0;
            virtual bool EndDocument() = 0;

            virtual bool BeginObject() = 0;
            virtual bool BeginObjectValue(const char* pName, size_t nNameLength) = 0;   // If the Json content is valid then this will be followed by BeginObject, BeginArray, Integer, Double, Bool, String, or Null.
            virtual bool EndObject() = 0;

            virtual bool BeginArray() = 0;
            virtual bool EndArray() = 0;

            virtual bool Integer(int64_t value, const char* pText, size_t nLength) = 0;
            virtual bool Double(double value, const char* pText, size_t nLength) = 0;
            virtual bool Bool(bool value, const char* pText, size_t nLength) = 0;
            virtual bool String(const char* pValue, size_t nValueLength, const char* pText, size_t nLength) = 0;    // pValue will differ from pText if pText needed to have escape sequences decoded. pValue will remain valid for use after this function completes and until the user explicitly clears the Json buffer (see JsonReader).
            virtual bool Null() = 0;
        };
     


        //////////////////////////////////////////////////////////////////////////
        /// A SAX-style JSON Reader, using JsonReader as a base class.
        //////////////////////////////////////////////////////////////////////////

        class EAJSON_API JsonCallbackReader : public JsonReader
        {
        public:
            JsonCallbackReader(Allocator::ICoreAllocator* pAllocator = NULL, size_t bufferBlockSize = 0);

            void                 SetContentHandler(IJsonReaderCallback* pContentHandler);
            IJsonReaderCallback* GetContentHandler() const;

            /// The JSON input data must be set via SetStream or SetString.
            Result Parse(IJsonReaderCallback* pContentHandler = NULL);

        protected:
            IJsonReaderCallback* mpContentHandler;   // The user-supplied callback.
            bool                 mbContinue;         // True if we should continue reading.
        };

    } // namespace Json

} // namespace EA







namespace EA {
    namespace Json {

        /*
        inline JsonCallbackReader::JsonCallbackReader( Allocator::ICoreAllocator * pAllocator, size_t bufferBlockSize ) 
          : JsonReader( pAllocator, bufferBlockSize )
        {
        }

        inline void JsonCallbackReader::SetContentHandler( JsonReaderCallback * pContentHandler )
        {
            mpContentHandler = pContentHandler;
        }

        inline JsonReaderCallback * JsonCallbackReader::GetContentHandler() const
        {
            return mpContentHandler;
        }
        */

    }
}


#endif // Header include guard





