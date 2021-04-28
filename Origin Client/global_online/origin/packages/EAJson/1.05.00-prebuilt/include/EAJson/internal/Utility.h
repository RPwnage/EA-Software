/////////////////////////////////////////////////////////////////////////////
// Utility.h
// 
// Copyright (c) 2009 Electronic Arts Inc.
// Written and maintained by Paul Pedriana.
/////////////////////////////////////////////////////////////////////////////


#ifndef EAJSON_INTERNAL_UTILITY_H
#define EAJSON_INTERNAL_UTILITY_H


#include <EAJson/internal/Config.h>


namespace EA
{
    namespace Json
    {
        // Returns number of chars that would be in a UTF8-encoded equivalent of UCS2 char c16.
        // The return value will be 1, 2, or 3. 
        EAJSON_API size_t UTF8Length(char16_t c16);


        // Returns the byte length of a UTF8 multibyte string that begins with c
        // Returns 0 if the char is an invalid start to a UTF8 string.
        EAJSON_API size_t UTF8CharSize(char8_t c);


        // This function assumes that there is enough space at p to write the char.
        // At most three bytes are needed to write a char16_t value.
        // returns pDest after appending the bytes.
        EAJSON_API char8_t* UTF8WriteChar(char8_t* pDest, const char16_t c);


        // Returns true if the given UTF8 character sequence is valid up to the
        // length that is present.
        EAJSON_API bool UTF8ValidatePartial(const char8_t* pText, size_t nLength);


        // This is a copy of the EAStdC package's StrtoU64Common.
        // If we become dependent on EAStdC via other means, then we should
        // switch our usage here to use the EAStdC version.
        // We need such a function because there is no portable 64 bit
        // implementation of atoi or sscanf(%lld).
        EAJSON_API uint64_t StrtoU64Common(const char8_t* pValue, char8_t** ppEnd, int nBase, bool bUnsigned);


        // This is a copy of the EAStdC package's StrtodEnglish.
        // If we become dependent on EAStdC via other means, then we should
        // switch our usage here to use the EAStdC version.
        // We need such a function because otherwise localization issues
        // mess up the ability to use the runtime-library strtod or sscanf.
        EAJSON_API double StrtodEnglish(const char8_t* pValue, char8_t** ppEnd);


        // Returns the first non-whitespace (" \t\r\n") char.
        // This is an internal function and thus not tagged with EAJSON_API.
        EAJSON_API const char8_t* SkipWhitespace(const char8_t* p);

    } // namespace Json

} // namespace EA





//////////////////////////////////////////////////////////////////////////
// Inlines
//////////////////////////////////////////////////////////////////////////

namespace EA
{
    namespace Json
    {
        inline size_t UTF8Length(char16_t c16)
        {
            if(c16 < 0x00000080)
                return 1;
            else if(c16 < 0x00000800)
                return 2;
            else // if(c16 < 0x00010000) 
                return 3;
        }

    } // namespace Json

} // namespace EA


#endif // Header include guard

