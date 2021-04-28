///////////////////////////////////////////////////////////////////////////////
// JsonDomWriter.h
// 
// Copyright (c) 2009 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
//
// See http://www.json.org/ and http://www.ietf.org/rfc/rfc4627.txt
///////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_JSONDOMWRITER_H
#define EAJSON_JSONDOMWRITER_H


#include <EAJson/internal/Config.h>
#include <EAJson/JsonDomReader.h>
#include <EAJson/JsonWriter.h>


#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4251) // 'x' needs to have dll-interface to be used by clients of class 'y'.
#endif


namespace EA
{
    namespace Json
    {
        //////////////////////////////////////////////////////////////////////////
        /// A DOM writer
        ///
        /// Writes a JSON document from a JsonDomNode.
        //////////////////////////////////////////////////////////////////////////

        class EAJSON_API JsonDomWriter : public IJsonDomReaderCallback
        {
        public:
            JsonDomWriter();

            // Acts the same as JsonWriter::Reset.
            void Reset();

            // Allows for stream-based writing through Write. To write to a 
            // string, you can use StringWriteStream<> from JsonWriter.h if
            // the string resembles an STL string.
            void SetStream(IWriteStream* pStream);

            // Writes the given domNode to the currently set IWriteStream.
            virtual bool Write(JsonDomNode& domNode);

            // The valid options are the same as JsonWriter options.
            void SetOption(int option, int value);
            int  GetOption(int option) const;

        protected:
            bool       mbSuccess;
            JsonWriter mWriter;

            virtual bool BeginDocument(JsonDomDocument& domDocument);
            virtual bool EndDocument(JsonDomDocument& domDocument);
            virtual bool BeginObject(JsonDomObject& domObject);
            virtual bool BeginObjectValue(JsonDomObject& domObject, const char* pName, size_t nNameLength, JsonDomObjectValue&);
          //virtual bool EndObjectValue(JsonDomObject& domObject, JsonDomObjectValue&);
            virtual bool EndObject(JsonDomObject& domObject);
            virtual bool BeginArray(JsonDomArray& domArray);
            virtual bool EndArray(JsonDomArray& domArray);
            virtual bool Integer(JsonDomInteger&, int64_t value, const char* pText, size_t nLength);
            virtual bool Double(JsonDomDouble&, double value, const char* pText, size_t nLength);
            virtual bool Bool(JsonDomBool&, bool value, const char* pText, size_t nLength);
            virtual bool String(JsonDomString&, const char* pValue, size_t nValueLength, const char* pText, size_t nLength);
            virtual bool Null(JsonDomNull&);
        };

    } // namespace Json

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard
