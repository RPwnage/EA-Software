///////////////////////////////////////////////////////////////////////////////
// BsonReader.h
// 
// Copyright (c) 2011 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
//
// Implements a StAX-like iterative (a.k.a streaming) reader for binary JSON (BSON).
///////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_BSONREADER_H
#define EAJSON_BSONREADER_H


#include <EAJson/internal/Config.h>
#include <EAJson/internal/BsonShared.h>
#include <EAJson/Bson.h>
#include <EAJson/JsonReader.h>
#include <EAJson/internal/TokenBuffer.h>
#include <EASTL/fixed_vector.h>


#ifdef _MSC_VER
    #pragma warning(push)
    #pragma warning(disable: 4251) // 'x' needs to have dll-interface to be used by clients of class 'y'.
#endif


namespace EA
{
    namespace Json
    {
        //////////////////////////////////////////////////////////////////////////
        /// BsonReader
        ///
        /// Implements an iterative reader of BSON content.
        /// Content is consumed via the Read function or by the lower level
        /// AddCharacter function. Text is assumed to be UTF8-encoded.
        ///
        /// Example usage:
        ///    BsonReader reader;
        ///    reader.SetString(pJsonText, textLength, false);
        ///    
        ///    for(BsonEventType e = kBETNone; (e != kBETError) && (e != kBETEndDocument); )
        ///    {
        ///        e = reader.Read();
        ///        Do something based on the event value.
        ///    }
        //////////////////////////////////////////////////////////////////////////

        class EAJSON_API BsonReader
        {
        public:
            /// If pAllocator is NULL, the EAJson default allocator will be used.
            /// If bufferBlockSize is 0, the EAJson default buffer block size (TokenBuffer::kBlockSizeDefault) will be used.
            BsonReader(EA::Allocator::ICoreAllocator* pAllocator = NULL, size_t bufferBlockSize = 0);
           ~BsonReader();

            EA::Allocator::ICoreAllocator* GetAllocator() const;
            void SetAllocator(EA::Allocator::ICoreAllocator* pAllocator);

            enum FormatOption
            {
                kFormatOptionEndian = 0,    /// One of enum EA::Json::Endian. Specifies the endian-ness of read values. Default is kEndianLittle, as that's what the BSON spec calls for.
                kFormatOptionCount  = 1
            };

            /// Set a format option
            /// See enum FormatOption
            void SetFormatOption(int formatOption, int value);

            /// Get a format option
            /// See enum FormatOption
            int GetFormatOption(int formatOption) const;

            /// Reset the Reader to a newly constructed parsing state such that new data 
            /// can be parsed. However, user options and the user-specified allocator 
            /// are not reset.
            bool Reset();

            /// Allows for byte-by-byte(one at a time) reading. 
            /// This type of reading is a low level alternative to using SetStream/SetString + Read.
            /// This function doesn't return the initial kBETBeginDocument event, and so if
            /// the user is using this function to do reading then the user will have to
            /// assume that kBETBeginDocument occurs implicitly upon the first call. This design is
            /// for practical reasons related to the AddCharacter implementation.
            /// Returns an EventType, which may be kBETNone in the case that no stream 
            /// event (e.g. kBETBeginObject) was found. 
            /// It's possible that a singe character may result in two Events (e.g. kBETInteger and 
            /// kBETEndObject), and the second event (which may be kBETNone if there is no second event)
            /// is returned in the secondEventType argument.
            BsonEventType AddCharacter(char8_t c, BsonEventType& secondEventType);

            /// Allows for stream-based reading through Read.
            void SetStream(IReadStream* pStream);

            /// Allows for string-based reading through Read.
            bool SetString(const char8_t* pString, size_t stringLength, bool bCopy);

            /// Advance to the next event in the input source.
            /// This function reads the input stream byte by byte until an event
            /// other than kBETNone occurs. Upon encountering a non-kBETNone event this
            /// function returns with the given EventType. Thus thus function should
            /// never return kBETNone.
            /// If the JSON data is valid but the end of the stream is reached before
            /// the end of the data is reached, then the return value is kBETError and 
            /// the Result is kErrorEOF. In this case you can set another stream with 
            /// more data and call Read again.
            /// This is used with SetStream or SetString.
            /// The first event returned by Read is kBETBeginDocument, and the last 
            /// event returned by Read is kBETEndDocument.
            BsonEventType Read();

            /// Get the current error status.
            Result GetResult() const;

            /// Current byte index within the current file (i.e. stream).
            /// The current byte index refers to the byte index that the current
            /// node (the most recent one that Read found) begins on.
            /// The first byte has a value of zero.
            ssize_t GetByteIndex() const;

            /// Return the depth of the current token
            int32_t GetDepth() const;

            /// Return the current event in the stream.
            /// Based on the event, you can 
            BsonEventType GetEventType() const;

            /// Returns the current value when EventType is kBETDouble.
            /// Has no meaning when the EventType is not kBETDouble.
            double GetDouble() const;
            float  GetFloat() const;

            /// Returns the current value when EventType is kBETInteger.
            /// Has no meaning when the EventType is not kBETInteger.
            int64_t GetInteger() const;
            int32_t GetInt32() const;
            int16_t GetInt16() const;
            int8_t  GetInt8() const;

            /// Returns the current value when EventType is kBETBool.
            /// Has no meaning when the EventType is not kBETBool.
            bool GetBool() const;

            /// Returns the current string when EventType is kBETString.
            /// Has no meaning when the EventType is not kBETString.
            /// This string is valid only upon return of this event and not later.
            const char8_t* GetString() const;

            /// Returns the strlen of the current string (GetString()).
            /// This string is valid only upon return of this event and not later.
            size_t GetStringLength() const;

            /// Returns the current value as a string.
            /// The interpretation of it depends on the current EventType.
            /// The returned string is good only while the current event type not kBETNone or kBETError.
            /// The returned string is good until the next read or reset of the JsonReader.
            /// This string is valid only upon return of this event and not later.
            const char8_t* GetValue() const;

            /// Returns the strlen of the current value (GetValue()).
            /// The returned length is good only while the current event type not kBETNone or kBETError.
            /// The returned length is good until the next read or reset of the JsonReader.
            size_t GetValueLength() const;

            /// Returns the name of the Object identified by the kBETBeginObjectValue event.
            /// This string is valid only upon return of this event and not later.
            const char8_t* GetName() const;

            /// Returns the current binary data.
            void GetBinaryData(const uint8_t*& pData, size_t& nDataLength, uint8_t& subType) const;

            /// Returns the debug text indicating where the last error occurred.
            /// The last char in the returned string is the char where the error was found.
            /// Valid only for the case of EAJSON_ENABLE_DEBUG_HELP = 1.
            const char8_t* GetDebugText() const;

            /// Sets whether EA_ASSERT is called when a syntax error is encountered.
            /// This may be useful for disabling assertion failures when errors are 'expected'.
            void SetAssertOnSyntaxError(bool bEnable);

        protected:
            friend class BsonWriter;

            enum State
            {
                kStateObjectSize,                       // Reading object size. BSON spec calls this "document" instead of "object," but that's inconsistent with the JSON spec.
                kStateElementType,                      // Reading the single byte element type id. e.g. kBETArray.
                kStateObjectName,                       // Reading the 0-terminated object name.
                kStateObjectTerminator,                 // BSON objects and arrays are 0-terminated. The terminating 0 doesn't serve much purpose aside from knowing the end of an object or array when you somehow don't have its size.
                kStateElementValue,                     // Reading the element value. What we expect for this state depends on the state's element type.
                kStateObjectNameJavascriptScopeSize,    // Used for reading kBLTJavaScriptS, which is a type that breaks the elegance of BSON/JSON.
                kStateObjectNameJavascriptScopeCode     // "
            };

            struct StateEntry
            {
                Internal::BsonElementType mElementType;       /// One of enum BsonElementType.
                State                     mState;             /// One of enum State.
                uint32_t                  mExpectedSize;      /// The current size at any time is equal to mByteBuffer.size(). All sizes are 32 bits in BSON.
                size_type                 mBeginPosition;     /// The byte position in the stream where this element began.
                size_type                 mEndPosition;       /// The byte position in the stream where the state ended. This is one-past the last byte of the element.
            };

            typedef eastl::fixed_vector<StateEntry, 8, true, EASTLCoreAllocator> StateEntryStack;

            typedef eastl::fixed_vector<uint8_t, 32, true, EASTLCoreAllocator> ByteBuffer;

        protected:
            static const size_type kPositionCurrent = (size_type)(ssize_type)-1;
            static const size_type kPositionUnknown = (size_type)(ssize_type)-1;

            void     PushStateEntry(Internal::BsonElementType elementType, State state, size_type beginPosition = kPositionCurrent);
            void     FinalizeValue();
            void     AppendDebugChar(char8_t c);
            bool     BsonElementTypeIsValid(uint8_t c);
            bool     BsonSizeIsValid(size_t size);
            bool     BsonNameLengthIsValid(size_t nameLength);
            bool     BsonNameIsValid(const ByteBuffer& sName);
            bool     BsonStringIsValid(const ByteBuffer& sString);
            uint16_t ReadUint16(uint8_t* pBuffer);
            uint32_t ReadUint32(uint8_t* pBuffer);
            uint64_t ReadUint64(uint8_t* pBuffer);
            float    ReadFloat(uint8_t* pBuffer);
            double   ReadDouble(uint8_t* pBuffer);

        protected:
            // To do: order these members for best packing size.
            Result                          mResult;                            /// 
            EA::Allocator::ICoreAllocator*  mpAllocator;                        /// Allocation pool we're drawing from.
            ByteBuffer                      mByteBuffer;                        /// A small buffer for storing an a multi-byte component while it's being read. e.g. string element, object size, object name string.
            ByteBuffer                      mByteBuffer2;                       /// Used in a few cases where we need to strings simultaneously.
            StateEntryStack                 mStateEntryStack;                   /// 
            Endian                          mEndian;                            /// Endian-ness of stream we read.
            IReadStream*                    mpStream;                           /// Used if the user calls SetStream().
            JsonReader::StringReadStream    mStringStream;                      /// Used if the user calls SetString().
            size_type                       mByteIndex;                         /// Byte index in the source buffer. 0-based.
            BsonEventType                   mEventType;                         /// The current BsonEventType. Usually this is kBETNone except when the char is encountered which finalizes the currently being read element.
            bool                            mbBeginDocumentOccurred;            /// Initially set to false. Set to true once we have returned kBETBeginDocument.
            bool                            mbAssertOnSyntaxError;              /// Defaults to enabled, though EA_ASSERT still needs to be enabled for it to have any effect.
            double                          mValueDouble;                       /// 
            float                           mValueFloat;                        /// 
            int64_t                         mValueInt64;                        /// 
            int32_t                         mValueInt32;                        /// 
            int16_t                         mValueInt16;                        /// 
            int8_t                          mValueInt8;                         /// 
            const char8_t*                  mValueString8;                      /// Pointer to current event text. Points into mTokenBuffer.
            size_t                          mValueString8Length;                /// The strlen of mValueString8.
            bool                            mValueBool;                         ///
            const uint8_t*                  mValueBinary;                       /// 
            size_t                          mValueBinaryLength;                 /// The length of the data pointed to by mValueBinary.
            uint8_t                         mValueBinarySubtype;                /// Declared as uint8_t instead of BsonBinarySubtype because there are possible user-defined values.
            const char8_t*                  mJavaScriptString8;                 /// JavaScript text.
            size_t                          mJavaScriptString8Length;           /// Strlen of mJavaScriptString8
            size_t                          mJavaScriptSSize;                   /// Size of the entire code_w_s (see the BSON spec) element: [int32 string document]
            const char8_t*                  mValueRegexPatternString8;         /// Pointer to current event text. Points into mTokenBuffer.
            size_t                          mValueRegexPatternString8Length;   /// The strlen of mValueString8.
            const char8_t*                  mValueRegexOptionsString8;         /// Pointer to current event text. Points into mTokenBuffer.
            size_t                          mValueRegexOptionsString8Length;   /// The strlen of mValueString8.
            char8_t                         mDebugString[16];                   /// Stores last N chars when EAJSON_ENABLE_DEBUG_HELP is enabled.
            int                             mDebugStringLength;                 /// Strlen of mDebugString.
        };


        /// Standalone function to convert the Result value into a string.
        /// Returns a read-only C string describing the error associated with the
        /// given ResultCode. The string will be lower case and have no terminating
        /// punctuation. This string is not localized; it is in US English.
        /// Example output:
        ///     GetBsonReaderResultString(kResultSyntax) -> "Syntax error"
        ///
        EAJSON_API const char8_t* GetBsonReaderResultString(Result result);


        /// This is a high level function for creating a printable error string 
        /// resulting from a BsonReader error. It provides more information than the
        /// GetBsonReaderResultString function, which simply translates Result to a string.
        /// errorStringCapacity must be greater than zero.
        /// Returns pErrorString.
        /// Example output (due to invalid floating format):
        ///      Result: Syntax error
        ///      Byte: 65\n", pBsonReader->GetByteIndex());
        ///      Location:   { "price" : 1.99.
        ///
        EAJSON_API const char8_t* FormatBsonResultString(const BsonReader* pBsonReader, char8_t* pErrorString, size_t errorStringCapacity);


        /// Validates a BSON block.
        /// If pOptionsArray is non-NULL then it must contain all (kOptionCount) option values.
        ///
        EAJSON_API Result ValidateBson(const char8_t* pBsonText, size_t textLength = (size_t)-1, 
                                        int* pOptionsArray = NULL, ssize_t* pErrorPosition = NULL,
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
        inline EA::Allocator::ICoreAllocator* BsonReader::GetAllocator() const
        {
            return mpAllocator;
        }

        inline Result BsonReader::GetResult() const
        {
            return mResult;
        }

        inline ssize_t BsonReader::GetByteIndex() const
        { 
            return (ssize_t)mByteIndex;
        }

        inline int32_t BsonReader::GetDepth() const
        { 
            return (int32_t)mStateEntryStack.size();
        }

        inline BsonEventType BsonReader::GetEventType() const
        { 
            return mEventType;
        }

        inline const char8_t* BsonReader::GetValue() const
        {
            return mValueString8;
        }

        inline size_t BsonReader::GetValueLength() const
        {
            return mValueString8Length;
        }

        inline void BsonReader::GetBinaryData(const uint8_t*& pData, size_t& nDataLength, uint8_t& subType) const
        {
            pData       = mValueBinary;
            nDataLength = mValueBinaryLength;
            subType     = mValueBinarySubtype;
        }

        inline const char8_t* BsonReader::GetDebugText() const
        {
            return mDebugString;
        }

        inline void BsonReader::SetAssertOnSyntaxError(bool bEnable)
        {
            mbAssertOnSyntaxError = bEnable;
        }

    } // namespace Json

} // namespace EA



#ifdef _MSC_VER
    #pragma warning(pop)
#endif


#endif // Header include guard








