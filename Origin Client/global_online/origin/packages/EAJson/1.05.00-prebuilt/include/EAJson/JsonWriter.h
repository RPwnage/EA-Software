///////////////////////////////////////////////////////////////////////////////
// JsonWriter.h
// 
// Copyright (c) 2009 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
//
// See http://www.json.org/ and http://www.ietf.org/rfc/rfc4627.txt
//
///////////////////////////////////////////////////////////////////////
// Example of formatted output of the EAJsonWriter class:
//
// [
//     "Some string",
//     "key":"value string",
//     "key":123,
//     null,
//     {
//         "array":
//         [
//             {
//             }
//         ],
//         "object":
//         {
//         }
//     }
// ]
///////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_JSONWRITER_H
#define EAJSON_JSONWRITER_H


#include <EAJson/internal/Config.h>
#include <EAJson/Json.h>


#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4251) // 'x' needs to have dll-interface to be used by clients of class 'y'.
#endif


namespace EA
{
    namespace Json
    {
        ///////////////////////////////////////////////////////////////////////
        // JsonWriter
        //
        // Implements a JSON-encoded text writer.
        // This class allocates no memory, though the user-provided IWriteStream
        // may possibly do so.
        //
        // Example usage:
        // 
        //
        ///////////////////////////////////////////////////////////////////////

        class EAJSON_API JsonWriter
        {
        public:
            JsonWriter();
            virtual ~JsonWriter();

            void Reset();

            enum FormatOption
            {
                kFormatOptionIndentSpacing = 0,   /// Specifies the number of automatic indent spacing chars to use per indent. Default is 0. A value of -1 means to use tab.
                kFormatOptionLineEnd       = 1,   /// Specifies if \n or \r\n is used for newlines (use '\n' or '\r' for the value in SetOption). A value of zero means no newlines are generated. Default is '\n'.
                kFormatOptionCount         = 2
            };

            void SetFormatOption(int formatOption, int value);
            int  GetFormatOption(int formatOption) const;

            void SetStream(IWriteStream* pStream);

            bool BeginDocument();
            bool EndDocument();

            bool BeginObject();
            bool BeginObjectValue(const char8_t* pName, size_t nNameLength = (size_t)-1); // You are not expected to provide the outer quotes for pName; they will be written for you. This function should be followed by Integer, String, Bool, Null, BeginObject, or BeginArray.
            bool EndObject();

            bool BeginArray();
            bool EndArray();

            bool Integer(int64_t value);
            bool Double(double value, const char* pFormat = NULL); // Writes a double as "%g" printf format (required by the JSON standard), but with redundant trailing zeroes removed. If pFormat is non-NULL, then it is a printf format specifier e, f, or g for the value (e.g. "2.3f" or "4.0E" or "8.2g").
            bool Bool(bool value);
            bool String(const char8_t* pValue, size_t nLength = (size_t)-1); // You are not expected to provide the enclosing " chars. You are not required to encode the string, but you can.
            bool Null();

            // Used to write a comment, in the form of /* */ or //. 
            // The pComment argument should include the comment delimiters.
            // This function simply writes the given pComment text to the 
            // output as-is. If you want space before the comment then you 
            // should include that in your text (e.g. "    // blah")
            // Comments may include newlines.
            bool Comment(const char8_t* pComment, size_t nLength = (size_t)-1);

        public: 
            // Deprecated, as this has been renamed for consistency with other code.
            enum Option { kOptionIndentSpacing = 0, kOptionLineEnd = 1 };
            void SetOption(int option, int value) { SetFormatOption(option, value); }
            int  GetOption(int option) const      { return GetFormatOption(option); }

        protected:
            bool WriteCommaNewlineIndent();
            bool WriteEncodedString(const char8_t* pText, size_t nLength);
            bool Write(const char8_t* pText, size_t nLength);

            struct StackEntry               // To consider: make this use just four bytes of space instead of 8.
            {
                EventType mEventType;       // The type of the current level (kETBeginObject or kETBeginArray).
                int       mElementCount;    // Number of elements written.
            };

            static const size_t kStackCapacity = 64;    // To consider: Make this configurable or dynamic.

        protected:
            size_t          mIndentLevel;           // 0, 1, 2, 3. A new document starts with an indent level of 0, a valid mStack[0] entry, and an implied array container (mStack[0].mEventType == kETBeginArray).
            StackEntry      mStack[kStackCapacity]; // 
            bool            mbFirstNewline;         // True if WriteNewlineIndent is being called the first time. Used for telling that \n shouldn't be written for the first ever written line.
            bool            mbSuppressComma;        // Used for telling if "," should be written in the next call to WriteNewlineIndent. For example, set to true if the last function called was EndObject or EndArray. 
            bool            mbSuppressNewline;      // Used for telling if \n" should be written in the next call to WriteNewlineIndent. For example, true if the last function called was BeginObjectValue. 
            size_t          mSpacingPerIndent;      // Defaults to 0.
            char8_t         mLineEnd[3];            // Defaults to '\n'. May also be '\r\n' or '\0'.
            IWriteStream*   mpStream;               // 
        };

    } // namespace Json

} // namespace EA





//////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////

namespace EA
{
    namespace Json
    {

        inline JsonWriter::~JsonWriter()
        {
        }

        inline void JsonWriter::SetStream(IWriteStream* pStream)
        {
            mpStream = pStream;
        }

        inline bool JsonWriter::Write(const char8_t* pText, size_t nLength)
        {
            if(mpStream)
                return mpStream->Write(pText, (size_type)nLength);

            return false;
        }

    } // namespace Json

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard
