/*************************************************************************************************/
/*!
    \file


    \attention
        (c) Electronic Arts. All Rights Reserved.
*/
/*************************************************************************************************/

#ifndef BLAZE_BLAZESTRING_H
#define BLAZE_BLAZESTRING_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

/*** Include files ****************************************************************/

// we need va_list to be a universal type
#include <stdarg.h>
#include "EASTL/string.h"
#include "EASTL/numeric_limits.h"

#ifdef BLAZE_CLIENT_SDK
#include "BlazeSDK/alloc.h" //for DEFAULT_BLAZE_MEMGROUP
#include "BlazeSDK/blaze_eastl/string.h"
#endif

#include "framework/eastl_containers.h"

/*** Defines **********************************************************************/

/*** Macros ***********************************************************************/
#ifndef ATTRIBUTE_PRINTF_CHECK
    #ifdef EA_COMPILER_GNUC
    #define ATTRIBUTE_PRINTF_CHECK(x,y) __attribute__((format(printf,x,y)))
    #else
    #define ATTRIBUTE_PRINTF_CHECK(x,y)
    #endif
#endif

#ifdef max
#undef max
#endif
/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/
extern char EMPTY_STRING[];

/*** Functions ********************************************************************/

// replacement string functions
BLAZESDK_API int32_t blaze_vsnzprintf(char8_t *pBuffer, size_t iLength, const char8_t *pFormat, va_list Args);
BLAZESDK_API int32_t blaze_snzprintf(char8_t *pBuffer, size_t iLength, const char8_t *pFormat, ...) ATTRIBUTE_PRINTF_CHECK(3,4);
BLAZESDK_API char8_t *blaze_strnzcpy(char8_t *pDest, const char8_t *pSource, size_t iCount);
BLAZESDK_API size_t blaze_strnzcat(char8_t *pDst, const char8_t *pSrc, size_t iDstLen);
BLAZESDK_API int32_t blaze_strcmp(const char8_t *pString1, const char8_t *pString2);
BLAZESDK_API int32_t blaze_stricmp(const char8_t *pString1, const char8_t *pString2);
BLAZESDK_API int32_t blaze_strncmp(const char8_t *pString1, const char8_t *pString2, size_t uCount);
BLAZESDK_API int32_t blaze_strnicmp(const char8_t *pString1, const char8_t *pString2, size_t uCount);
BLAZESDK_API char8_t *blaze_strstr(const char8_t *pHaystack, const char8_t *pNeedle);
BLAZESDK_API char8_t *blaze_stristr(const char8_t *pHaystack, const char8_t *pNeedle);
BLAZESDK_API char8_t *blaze_strnstr(const char8_t *pHaystack, const char8_t *pNeedle, size_t uCount);
BLAZESDK_API char8_t *blaze_strnistr(const char8_t *pHaystack, const char8_t *pNeedle, size_t uCount);
BLAZESDK_API char8_t *blaze_strdup(const char8_t *pString, Blaze::MemoryGroupId memGroupId = DEFAULT_BLAZE_MEMGROUP);
BLAZESDK_API char8_t *blaze_strtok(char8_t *str, const char8_t *delim, char **saveptr);

/*************************************************************************************************/
/*!
    \Function blaze_stltokenize

    Split string into tokens using ANY of the characters specified in the delimiters.
    Can choose if empty string tokens are allowed in the result.

    \param[in]              str - Input string.
    \param[in]      pDelimiters - Delimiter characters.
    \param[out]          result - Vector containing the splited tokens.
    \param[in]  allowEmptyToken - Allow empty string tokens in the result.

    \return Number of tokens in the result vector.
*/
/*************************************************************************************************/
template <typename String>
/*BLAZESDK_API not needed because the function is inline and in fact adding that causes an error when building Arson*/ 
size_t blaze_stltokenize(const String& str, const typename String::value_type* pDelimiters, eastl::vector<String>& result, const bool allowEmptyToken = false)
{
    typename String::size_type begin = 0;
    typename String::size_type end = str.find_first_of(pDelimiters, begin);

    while ((begin != String::npos) || (end != String::npos))
    {
        if (allowEmptyToken || (begin != str.length() && begin != end))
        {
            result.push_back().assign(str, begin, end - begin);
        }
        begin = (end == String::npos ? String::npos : end + 1);
        end = str.find_first_of(pDelimiters, begin);
    }

    return result.size();
}

// 'original' string functions
BLAZESDK_API size_t blaze_strsubzcpy(char8_t *pDst, size_t iDstLen, const char8_t *pSrc, size_t iSrcLen);
BLAZESDK_API size_t blaze_strsubzcat(char8_t *pDst, size_t iDstLen, const char8_t *pSrc, size_t iSrcLen);

BLAZESDK_API void blaze_strupr(char8_t* pStr);
BLAZESDK_API void blaze_strlwr(char8_t* pStr);
BLAZESDK_API char8_t* blaze_stripWhitespace(const char8_t* src, char8_t* dest, uint32_t);

/*************************************************************************************************/
/*!
    \Function blaze_strcpyFixedLengthToNullTerminated

    Copies a fixed length string do the destination buffer adding a null
    terminator at the end.  Useful with the HTTP component as all of the 
    string references are not null terminated.

    \param[out]      dest - Destination buffer to store the null terminated string.
    \param[in]    destMax - Max size of the destination buffer.
    \param[in]     source - Source string to copy.
    \param[in] sourceSize - Size of the string to copy.

    \return 
*/
/*************************************************************************************************/
BLAZESDK_API void blaze_strcpyFixedLengthToNullTerminated(char8_t *dest, size_t destMax, const  char8_t *source, size_t sourceSize);

/*************************************************************************************************/
/*!
    \Function blaze_str2int

    Converts the numeric part of a string into a number and returns a ptr to the first non-numeric
    part of the string found.  For example, if the string is "1234sometext", it will convert "1234"
    into 1234 and return a ptr to "sometext".

    Hexidecimal conversion will take place if the string begins with "0x".  But be careful, passing
    in a string like "0x1234andsometext" will get you the number 0x1234a and a return value pointing
    to the string "ndsometext".

    Since a nullptr terminator is considered non-numeric strings that consist of just digits are fine.

    \param[in]  nptr - nullptr-terminated string to convert into a number
    \param[out] value - Where to store the number (template parameter)

    \return Pointer to the first non-numeric part of the string after conversion.  If the return
            value is the same as the passed in string, no conversion took place (usually indicates
            an error).

*/
/*************************************************************************************************/
template<typename T>
char8_t* blaze_str2int(const char8_t* nptr, T* value)
{
    T val = 0;
    T max = eastl::numeric_limits<T>::max() / 10;
    T rem = eastl::numeric_limits<T>::max() % 10;
    const char8_t* ptr = nptr;

    if (nptr == nullptr || *nptr == '\0' || value == nullptr)
    {
        // Don't advance the nptr or set the value.  This should indicate an error.
        return const_cast<char8_t*>(nptr);
    }
            
    if ((nptr[0] == '0') && ((nptr[1] | 32) == 'x'))
    {       
        // Hex number
        nptr += 2;
            
        for(; *nptr != '\0'; nptr++)
        {
            uint8_t ch = (unsigned char)*nptr;
            if ((ch >= '0') && (ch <= '9'))
                ch -= '0';
            else
            {
                ch |= 32;
                if ((ch >= 'a') && (ch <= 'f'))
                {
                    ch -= 'a';
                    ch += 10;
                }
                else
                {
                    // End of the number
                    break;
                }
            }
            // Return original pointer to indicate overflow detected
            if ((val > max) || ((val == max) && (ch > rem)))
            {
                nptr = ptr;
                break;
            }
            val = (val << 4) | ch;
        }
    }
    else
    {
        // Decimal number
        bool neg = false;
        if (nptr[0] == '-')
        {
            neg = true;
            // To be consistent with both strtol and strtoul, base overflow on the 
            // minimum value of the given integer type for negative signed integers 
            // and on the maximum value for negative unsigned integers.
            if (eastl::numeric_limits<T>::lowest() != 0)
            {
                max = ((T)-1)*(eastl::numeric_limits<T>::lowest() / 10);
                rem = ((T)-1)*(eastl::numeric_limits<T>::lowest() % 10);
            }
            nptr++;
        }
        for(; *nptr != '\0'; nptr++)
        {
            uint8_t ch = (unsigned char)*nptr;
            if ((ch >= '0') && (ch <= '9'))
            {
                ch -= '0';
                // Return original pointer to indicate overflow detected
                if ((val > max) || ((val == max) && (ch > rem)))
                {
                    nptr = ptr;
                    break;
                }
                val = (val * 10) + ch;
            }
            else
            {
                // End of the number
                break;
            }
        }
        if (neg)
            val *= (T)-1;
    }
    *value = val;
    return const_cast<char8_t*>(nptr);
}

BLAZESDK_API uint32_t blaze_strhex2int(char8_t* strHex);

BLAZESDK_API char8_t* blaze_str2flt(const char8_t *ptr, float &value);

BLAZESDK_API char8_t* blaze_str2dbl(const char8_t *ptr, double &value);

//  Returns a hex-encoded string representing an input binary buffer.  outMax should at lest be srclen*2+1 length
BLAZESDK_API char8_t* blaze_bin2hex(char8_t *out, size_t outMax, const uint8_t *src, size_t srclen);

// EASTL compatible c-string comparison structs
struct BLAZESDK_API CaseInsensitiveStringLessThan {
    bool operator()(const char8_t* a, const char8_t* b) const { return blaze_stricmp(a,b) < 0; }
    bool operator()(const BLAZE_EASTL_STRING &a, const BLAZE_EASTL_STRING &b) const { return (a.comparei(b) < 0); }
};

struct BLAZESDK_API CaseSensitiveStringLessThan {
    bool operator()(const char8_t* a, const char8_t* b) const { return blaze_strcmp(a,b) < 0; }
};

struct BLAZESDK_API CaseInsensitiveStringEqualTo
{
    bool operator()(const char8_t* a, const char8_t* b) const { return (blaze_stricmp(a,b) == 0); }
    bool operator()(const BLAZE_EASTL_STRING &a, const BLAZE_EASTL_STRING &b) const { return (a.comparei(b) == 0); }
};

struct BLAZESDK_API CaseInsensitiveStringHash
{
    size_t operator()(const char8_t* p) const;
    size_t operator()(const BLAZE_EASTL_STRING &p) const { return (*this)(p.c_str()); }
};


#endif // BLAZE_BLAZESTRING_H

