/////////////////////////////////////////////////////////////////////////////
// BsonCallbackReader.h
//
// Copyright (c) 2011, Electronic Arts Inc. All rights reserved.
// Written by Paul Pedriana
//
// SAX-style callback parser for binary JSON (BSON).
/////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_BSONCALLBACKREADER_H
#define EAJSON_BSONCALLBACKREADER_H


#include <EAJson/internal/Config.h>
#include <EAJson/JsonCallbackReader.h>
#include <EAJson/BsonReader.h>


namespace EA
{
    namespace Json
    {
        //////////////////////////////////////////////////////////////////////////
        /// IBsonReaderCallback
        ///
        /// Callback interface for BSON events.
        //////////////////////////////////////////////////////////////////////////

        class IBsonReaderCallback : public IJsonReaderCallback // It is an extension of IJsonReaderCallback.
        {
        public:
            // IJsonReaderCallback
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

            // Extended Bson-specific functionality.
            // Since this data represents binary data, we don't replicate the Json callback interface of providing a pText/nLength pair.
            virtual bool Int32(int32_t value) = 0;
            virtual bool Int16(int16_t value) = 0;
            virtual bool Int8(int8_t value) = 0;
            virtual bool Float(float value) = 0;
            virtual bool Binary(const uint8_t* pData, size_t nLength) = 0;
            virtual bool UTCDatetime(int64_t value) = 0;
            virtual bool Regex(const char8_t* pExpression, size_t nExpressionLength, const char8_t* pOptions, size_t nOptionsLength) = 0;
            virtual bool JavaScript(const char8_t* pJavaScript, size_t nJavaScriptLength) = 0;
            virtual bool ObjectId(const uint8_t* pData12Bytes) = 0;  // 12 bytes
            virtual bool DBPointer(const char8_t* pString, size_t nStringLength, const uint8_t* pData12Bytes) = 0;
            virtual bool Symbol(const char8_t* pString, size_t nStringLength) = 0;
            virtual bool Timestamp(int64_t value) = 0;
            virtual bool BeginJavaScriptObject(const char8_t* pJavaScript, size_t nJavaScriptLength) = 0;
            virtual bool EndJavaScriptObject() = 0;
          //virtual bool MaxKey() = 0;
          //virtual bool MinKey() = 0;
        };


        //////////////////////////////////////////////////////////////////////////
        /// A SAX-style BSON (binary JSON) reader, using BsonReader as a base class.
        //////////////////////////////////////////////////////////////////////////

        class EAJSON_API BsonCallbackReader : public BsonReader
        {
        public:
            BsonCallbackReader(Allocator::ICoreAllocator* pAllocator = NULL, size_t bufferBlockSize = 0);

            void                 SetContentHandler(IBsonReaderCallback* pContentHandler);
            IBsonReaderCallback* GetContentHandler() const;

            /// The BSON input data must be set via SetStream or SetString.
            Result Parse(IBsonReaderCallback* pContentHandler = NULL);

        protected:
            IBsonReaderCallback* mpContentHandler;   // The user-supplied callback.
            bool                 mbContinue;         // True if we should continue reading.
        };

    } // namespace Json

} // namespace EA




#endif // Header include guard





