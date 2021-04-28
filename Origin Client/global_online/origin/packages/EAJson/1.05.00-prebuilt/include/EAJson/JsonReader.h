///////////////////////////////////////////////////////////////////////////////
// JsonReader.h
// 
// Copyright (c) 2009 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
//
// Implements a StAX-like iterative (a.k.a streaming) reader for JSON.
// See http://www.json.org/ and http://www.ietf.org/rfc/rfc4627.txt
///////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_JSONREADER_H
#define EAJSON_JSONREADER_H


#include <EAJson/internal/Config.h>
#include <EAJson/internal/TokenBuffer.h>
#include <EAJson/Json.h>


#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4251) // 'x' needs to have dll-interface to be used by clients of class 'y'.
#endif


namespace EA
{
    namespace Json
    {
        //////////////////////////////////////////////////////////////////////////
        /// JsonReader
        ///
        /// Implements an iterative reader of JSON content.
        /// Content is consumed via the Read function or by the lower level
        /// AddCharacter function. Text is assumed to be UTF8-encoded.
        ///
        /// Example usage:
        ///    JsonReader reader;
        ///    reader.SetString(pJsonText, textLength, false);
        ///    
        ///    for(EventType e = kETNone; (e != kETError) && (e != kETEndDocument); )
        ///    {
        ///        e = reader.Read();
        ///        Do something based on the event value.
        ///    }
        //////////////////////////////////////////////////////////////////////////

        class EAJSON_API JsonReader
        {
        public:
            /// If pAllocator is NULL, the EAJson default allocator will be used.
            /// If bufferBlockSize is 0, the EAJson default buffer block size (TokenBuffer::kBlockSizeDefault) will be used.
            JsonReader(EA::Allocator::ICoreAllocator* pAllocator = NULL, size_t bufferBlockSize = 0);
           ~JsonReader();

            EA::Allocator::ICoreAllocator* GetAllocator() const;
            void SetAllocator(EA::Allocator::ICoreAllocator* pAllocator);

            enum FormatOption
            {
                kFormatOptionEnableHexIntegers    = 0,    /// Disabled by default, since it isn't standard. Recognize hex integers in the form of "0xhhh..." (lower or upper case). 
                kFormatOptionEnableControlChars   = 1,    /// Disabled by default, since it isn't standard. Recognize char values less than 0x20 (i.e. control chars). Note that '\0' is included in this. 
                kFormatOptionAllowComments        = 2,    /// Disabled by default, since it isn't standard. Allows use of C-like comments: "/*...*/". Comments are not nestable. Comments must not interrupt values; they must be between values.
                kFormatOptionAllowNonUTF8         = 3,    /// Disabled by default, since it isn't standard. Allows use of 8 bit strings that aren't UTF8.
              //kFormatOptionEnableAbbreviations  = 4,    /// Disabled by default, since it isn't standard. Allow use of t for true, f for false, n for null. This is non-standard but may be useful, as it can safe memory.
              //kFormatOptionAllowSimpleTypeStart = 5,    /// Disabled by default, since it isn't standard. Allows use of a simple type at document start. Usually, the document must begin with an object or array.
                kFormatOptionCount                = 4
            };

            /// Set a format option
            /// See enum FormatOption.
            void SetFormatOption(int formatOption, int value);

            /// Get a format option
            /// See enum FormatOption.
            int GetFormatOption(int formatOption) const;

            /// SetBufferBlockSize
            /// Sets the size of buffering blocks. Default is TokenBuffer::kBlockSizeDefault.
            /// Typically you would call this before using the class instance.
            bool SetBufferBlockSize(size_t size);

            /// Reset the JsonReader to a newly constructed parsing state such that new data 
            /// can be parsed. However, user options and the user-specified allocator 
            /// are not reset.
            bool Reset();

            /// Allows for char-by-char (one at a time) reading. 
            /// This type of reading is a low level alternative to using SetStream/SetString + Read.
            /// This function doesn't return the initial kETBeginDocument event, and so if
            /// the user is using this function to do reading then the user will have to
            /// assume that kETBeginDocument occurs implicitly upon the first call. This design is
            /// for practical reasons related to the AddCharacter implementation.
            /// Returns an EventType, which may be kETNone in the case that no stream 
            /// event (e.g. kETBeginObject) was found. 
            /// It's possible that a singe character may result in two Events (e.g. kETInteger and 
            /// kETEndObject), and the second event (which may be kETNone if there is no second event)
            /// is returned in the secondEventType argument.
            EventType AddCharacter(char8_t c, EventType& secondEventType);

            /// Allows for stream-based reading through Read.
            void SetStream(IReadStream* pStream);

            /// Allows for string-based reading through Read.
            /// The stringLength refers to the strlen of the string, and the string doesn't necessarily
            /// need to be 0-terminated, as the length value alone defines its extent.
            bool SetString(const char8_t* pString, size_t stringLength, bool bCopy);

            /// Advance to the next event in the input source.
            /// This function reads the input stream byte by byte until an event
            /// other than kETNone occurs. Upon encountering a non-kETNone event this
            /// function returns with the given EventType. Thus thus function should
            /// never return kETNone.
            /// If the JSON data is valid but the end of the stream is reached before
            /// the end of the data is reached, then the return value is kETError and 
            /// the Result is kErrorEOF. In this case you can set another stream with 
            /// more data and call Read again.
            /// This is used with SetStream or SetString.
            /// The first event returned by Read is kETBeginDocument, and the last 
            /// event returned by Read is kETEndDocument.
            EventType Read();

            /// Get the current error status.
            Result GetResult() const;

            /// Return true if the reader is currently at the end of file
            /// bool IsEof() const;
 
            /// Current line number within the current file (i.e. stream).
            /// The first line has a value of 1, which is consistent with how most text editors work.
            /// The current line number refers to the line number that the current
            /// node (the most recent one that Read found) begins on.
            int32_t GetLineNumber() const;

            /// Current column number within the current file (i.e. stream).
            /// The first column has a value of 1, which is consistent with how most text editors work.
            /// The current column number refers to the column (horizontal text space) that the current
            /// node (the most recent one that Read found) begins on.
            int32_t GetColumnNumber() const;

            /// Current byte 0-based index within the current file (i.e. stream).
            /// The current byte index refers to the byte index that the current
            /// node (the most recent one that Read found) begins on.
            /// The first byte has a value of zero.
            ssize_t GetByteIndex() const;

            /// Return the depth of the current token
            int32_t GetDepth() const;

            /// Return the current event in the stream.
            /// Based on the event, you can 
            EventType GetEventType() const;

            /// Returns the current value when EventType is kETDouble.
            /// Has no meaning when the EventType is not kETDouble.
            double GetDouble() const;

            /// Returns the current value when EventType is kETInteger.
            /// Has no meaning when the EventType is not kETInteger.
            int64_t GetInteger() const;

            /// Returns the current value when EventType is kETBool.
            /// Has no meaning when the EventType is not kETBool.
            bool GetBool() const;

            /// Returns the current string when EventType is kETString.
            /// Has no meaning when the EventType is not kETString.
            /// This string is valid only upon return of this event and not later.
            /// The returned string is 0-terminated, but you can also use GetStringLength.
            /// If the current EventType is kETDouble, the string returned is the result of
            /// sprintf'ing the double with %f. If the currentEventType is kETInteger, the 
            /// string returned is the result of sprintf-ing the integer with %lld (64 bit printf).
            const char8_t* GetString() const;

            /// Returns the strlen of the current string (GetString()).
            /// This string is valid only upon return of this event and not later.
            size_t GetStringLength() const;

            /// Returns the current value as a string.
            /// The interpretation of it depends on the current EventType.
            /// The returned string is good only while the current event type not kETNone or kETError.
            /// The returned string is good until the next read or reset of the JsonReader.
            /// This string is valid only upon return of this event and not later.
            const char8_t* GetValue() const;

            /// Returns the strlen of the current value (GetValue()).
            /// The returned length is good only while the current event type not kETNone or kETError.
            /// The returned length is good until the next read or reset of the JsonReader.
            size_t GetValueLength() const;

            /// Returns the name of the Object identified by the kBETBeginObjectValue event.
            /// This string is valid only upon return of this event and not later.
            const char8_t* GetName() const;

            /// Returns the debug text indicating where the last error occurred.
            /// The last char in the returned string is the char where the error was found.
            /// Valid only for the case of EAJSON_ENABLE_DEBUG_HELP = 1.
            const char8_t* GetDebugText() const;

            /// Sets whether EA_ASSERT is called when a syntax error is encountered.
            /// This may be useful for disabling assertion failures when errors are 'expected'.
            void SetAssertOnSyntaxError(bool bEnable);

        protected:
            friend class BsonReader;

            bool WriteEscapedChar(char8_t c);
            void FinalizeSimpleType();
            void AppendDebugChar(char8_t c);
            void ParseInteger() const;
            void ParseDouble() const;


            class StringReadStream : public IReadStream
            {
            public:
                StringReadStream(EA::Allocator::ICoreAllocator* pAllocator);
                virtual ~StringReadStream();

                void      Reset();
                bool      SetString(const char8_t* pString, size_t stringLength, bool bCopy);
                size_type Read(void* pData, size_type nSize);

            public:
                typedef EA::Allocator::ICoreAllocator Allocator;

                const char8_t* mpData;          // Pointer to char data. Doesn't require 0-termination.
                size_t         mnLength;        // Length of the data pointed to by mpData. Does not include a terminating 0 char.
                size_t         mnPosition;      // Current position within mpData.
                Allocator*     mpAllocator;     // Allocator used to free mpData, if non-NULL.
                bool           mbCopied;        // True if mpData's contents copied from user-specified data as opposed to mpData being the pointer the user supplied.
            };

        protected:
            Result                          mResult;                            /// 
            EA::Allocator::ICoreAllocator*  mpAllocator;                        /// Allocation pool we're drawing from.
            TokenBuffer                     mTokenBuffer;                       /// Items that stay around until all parsing complete.
            TokenBuffer                     mParseStateStack;                   /// 
            IReadStream*                    mpStream;                           /// Used if the user calls SetStream().
            StringReadStream                mStringStream;                      /// Used if the user calls SetString().
            int32_t                         mLineIndex;                         /// Line number in the source buffer. 0-based.
            int32_t                         mColumnIndex;                       /// Column index in the source buffer. 0-based.
            ssize_t                         mByteIndex;                         /// Byte index in the source buffer. 0-based.
            int32_t                         mElementDepth;                      /// Current nesting depth of the parser.
            EventType                       mNextEventType;                     /// The EventType that will be next when the end of the current element is reached.
            EventType                       mEventType;                         /// The current EventType. Usually this is kETNone except when the char is encountered which finalizes the currently being read element.
            EventType                       mReadSavedSecondEventType;          /// Used by the Read function.
            int8_t                          mState;                             /// See enum CharState and enum ActionState.
            int8_t                          mStateSaved;                        /// Saved state from before there was a comment. mState is set back to mStateSaved after a comment ends.
            bool                            mbBeginDocumentOccurred;            /// Initially set to false. Set to true once we have returned kETBeginDocument.
            bool                            mbClearTokenBuffer;                 /// If true then we should clear mTokenBuffer on the next byte read. This variable is a 'message' left for us at the end of reading the previous byte. We delay the clear because the user needs to read mTokenBuffer between the last byte and the next byte. 
            bool                            mbCharEscapeActive;                 /// If true then we are in the middle of reading an escape char (one of \\ \/ \b \f \n \r \t \uXXXX).
            bool                            mbCommentActive;                    /// If true then we are in the middle of a comment. See kOptionAllowComments.
            bool                            mFormatOptions[kFormatOptionCount]; /// 
            bool                            mbAssertOnSyntaxError;              /// Defaults to enabled, though EA_ASSERT still needs to be enabled for it to have any effect.
            mutable bool                    mbNeedToParseValue;                 /// We delay the parsing of types like mValueDouble until the user calls GetDouble.
            mutable double                  mValueDouble;                       /// 
            mutable int64_t                 mValueInteger;                      /// 
            const char8_t*                  mValueString8;                      /// Pointer to current event text. Points into mTokenBuffer. 0-terminated.
            size_t                          mValueString8Length;                /// The strlen of mValueString8.
            bool                            mValueBool;                         ///
            char8_t                         mDebugString[16];                   /// Stores last N chars when EAJSON_ENABLE_DEBUG_HELP is enabled.
            int                             mDebugStringLength;                 /// Strlen of mDebugString.
        };


        /// Standalone function to convert the Result value into a string.
        /// Returns a read-only C string describing the error associated with the
        /// given ResultCode. The string will be lower case and have no terminating
        /// punctuation. This string is not localized; it is in US English.
        /// Example output:
        ///     GetJsonReaderResultString(kResultSyntax) -> "Syntax error"
        EAJSON_API const char8_t* GetJsonReaderResultString(Result result);


        /// This is a high level function for creating a printable error string 
        /// resulting from a JsonReader error. It provides more information than the
        /// GetJsonReaderResultString function, which simply translates Result to a string.
        /// errorStringCapacity must be greater than zero.
        /// Returns pErrorString.
        /// Example output (due to invalid floating format):
        ///      Result: Syntax error
        ///      Line: 15, column: 37, byte: 99
        ///      Location:   { "price" : 1.99.
        EAJSON_API const char8_t* FormatJsonResultString(const JsonReader* pJsonReader, char8_t* pErrorString, size_t errorStringCapacity);


        /// Validates a Json block.
        /// If pOptionsArray is non-NULL then it must contain all (kOptionCount) option values.
        EAJSON_API Result ValidateJson(const char8_t* pJsonText, size_t textLength = (size_t)-1, 
                                        const int* pOptionsArray = NULL, ssize_t* pErrorPosition = NULL,
                                        char8_t* pErrorString = NULL, size_t errorStringCapacity = 0,
                                        EA::Allocator::ICoreAllocator* pAllocator = NULL);

    } // namespace Json

} // namespace EA





//////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////

namespace EA
{
    namespace Json
    {
        inline EA::Allocator::ICoreAllocator* JsonReader::GetAllocator() const
        {
            return mpAllocator;
        }

        inline void JsonReader::SetAllocator(EA::Allocator::ICoreAllocator* pAllocator)
        {
            mpAllocator = pAllocator;
            mTokenBuffer.SetAllocator(pAllocator);
            mParseStateStack.SetAllocator(pAllocator);
            mStringStream.mpAllocator = pAllocator;
        }

        inline Result JsonReader::GetResult() const
        {
            return mResult;
        }

        inline int32_t JsonReader::GetLineNumber() const
        { 
            return mLineIndex + 1;
        }

        inline int32_t JsonReader::GetColumnNumber() const
        { 
            return mColumnIndex + 1;
        }

        inline ssize_t JsonReader::GetByteIndex() const
        { 
            return mByteIndex;
        }

        inline int32_t JsonReader::GetDepth() const
        { 
            return mElementDepth;
        }

        inline EventType JsonReader::GetEventType() const
        { 
            return mEventType;
        }

        inline const char8_t* JsonReader::GetValue() const
        {
            return mValueString8;
        }

        inline size_t JsonReader::GetValueLength() const
        {
            return mValueString8Length;
        }

        inline const char8_t* JsonReader::GetDebugText() const
        {
            return mDebugString;
        }

        inline void JsonReader::SetAssertOnSyntaxError(bool bEnable)
        {
            mbAssertOnSyntaxError = bEnable;
        }


    } // namespace Json

} // namespace EA


#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard








