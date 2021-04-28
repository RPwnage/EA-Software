/*H********************************************************************************/
/*!
    \File blazestring.c

    \Description
        This file provides platform independent versions of some standard-C
        functions that are not "standard" across platforms and/or fixes
        implementation problems with the standard versions (such as consistent
        termination).

    \Notes
        Derived directly from plat-str.c in DirtySock codebase.

    \Copyright
        (c) Electronic Arts. All Rights Reserved.

    \Version 01/11/2005 (gschaefer) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdarg.h>     // NOTE - must come before stdio for IOP platform, or vsprintf is not defined
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>      // tolower

#include "framework/blaze.h"
#include "framework/util/shared/blazestring.h"

/*** Defines **********************************************************************/

#ifdef EA_PLATFORM_WINDOWS
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif


/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/
char EMPTY_STRING[] = "";

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function blaze_vsnzprintf

    \Description
        Replacement for vsnprintf that always includes a terminator at the end of
        the string.  The overflow semantics are also slightly different (pBuffer is
        zero'd on overflow).

    \Input *pBuffer - output buffer.  NOTE: buffer is zero'd on error or overflow
    \Input iLength  - size of output buffer
    \Input *pFormat - format string
    \Input Args      - variable argument list
    
    \Output
        int32_t     - number of characters written to output buffer, not including
                      the null character; zero on error or overflow

    \Version 04/04/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t blaze_vsnzprintf(char8_t* pBuffer, size_t uLength, const char8_t* pFormat, va_list Args)
{
    int32_t iResult;
    
    // make sure there's room for null termination
    if (uLength == 0) return(0);
    
    // format the text    
    iResult = vsnprintf(pBuffer, uLength, pFormat, Args);
    if (iResult >= (int32_t)uLength)
    {
        iResult = 0;
    }   
    else if (iResult < 0)
    {
        iResult = 0;
    }   
    pBuffer[iResult] = '\0';
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function blaze_snzprintf

    \Description
        Replacement for snprintf that always includes a terminator at the end of
        the string.  The overflow semantics are also slightly different (pBuffer is
        zero'd on overflow).

    \Input *pBuffer - output buffer.  NOTE: buffer is zero'd on error or overflow
    \Input iLength  - size of output buffer
    \Input *pFormat - format string
    \Input ...      - variable argument list
    
    \Output
        int32_t     - number of characters written to output buffer, not including
                      the null character; zero on error or overflow

    \Version 04/04/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t blaze_snzprintf(char8_t* pBuffer, size_t uLength, const char8_t* pFormat, ...)
{
    int32_t iResult;
    va_list args;

    // make sure there's room for null termination
    if (uLength == 0) return(0);

    // format the text
    va_start(args, pFormat);
    iResult = vsnprintf(pBuffer, uLength, pFormat, args);
    va_end(args);
    if (iResult >= (int32_t)uLength)
    {
        iResult = 0;
    }
    else if (iResult < 0)
    {
        iResult = 0;
    }
    pBuffer[iResult] = '\0';
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function blaze_strstr

    \Description
        Substring search

    \Input see strstr

    \Output see strstr

    \Version 03/31/2009 (dmikulec)
*/
/********************************************************************************F*/
char8_t *blaze_strstr(const char8_t* pHaystack, const char8_t* pNeedle)
{
    int32_t iIndex;
    char8_t *pMatch = nullptr;

    // make sure strings are valid
    if ((pHaystack != nullptr) && (*pHaystack != 0) && (pNeedle != nullptr) && (*pNeedle != 0))
    {
        for (; *pHaystack != 0; ++pHaystack)
        {
            // small optimization on first character search
            if (*pHaystack == *pNeedle)
            {
                for (iIndex = 1;; ++iIndex)
                {
                    if (pNeedle[iIndex] == 0)
                    {
                        pMatch = (char8_t *)pHaystack;
                        return(pMatch);
                    }
                    if (pHaystack[iIndex] == 0)
                    {
                        break;
                    }
                    if (pHaystack[iIndex] != pNeedle[iIndex])
                    {
                        break;
                    }
                }
            }
        }
    }
    return(pMatch);
}

/*F********************************************************************************/
/*!
    \Function blaze_stristr

    \Description
        Case insensitive substring search

    \Input see strstr

    \Output see strstr

    \Version 01/25/2005 (gschaefer)
*/
/********************************************************************************F*/
char8_t *blaze_stristr(const char8_t* pHaystack, const char8_t* pNeedle)
{
    int32_t iFirst;
    int32_t iIndex;
    char8_t *pMatch = nullptr;

    // make sure strings are valid
    if ((pHaystack != nullptr) && (*pHaystack != 0) && (pNeedle != nullptr) && (*pNeedle != 0))
    {
        iFirst = tolower((unsigned char)*pNeedle);
        for (; *pHaystack != 0; ++pHaystack)
        {
            // small optimization on first character search
            if (tolower((unsigned char)*pHaystack) == iFirst)
            {
                for (iIndex = 1;; ++iIndex)
                {
                    if (pNeedle[iIndex] == 0)
                    {
                        pMatch = (char8_t *)pHaystack;
                        return(pMatch);
                    }
                    if (pHaystack[iIndex] == 0)
                    {
                        break;
                    }
                    if (tolower((unsigned char)pHaystack[iIndex]) != tolower((unsigned char)pNeedle[iIndex]))
                    {
                        break;
                    }
                }
            }
        }
    }
    return(pMatch);
}

/*F********************************************************************************/
/*!
    \Function blaze_strnstr

    \Description
        Substring search of first N characters

    \Input see strstr

    \Output see strstr

    \Version 06/27/2007 (dmikulec)
*/
/********************************************************************************F*/
char8_t *blaze_strnstr(const char8_t* pHaystack, const char8_t* pNeedle, size_t uCount)
{
    char8_t *pMatch = nullptr;

    // make sure strings are valid
    if ((pHaystack != nullptr) && (*pHaystack != 0) && (pNeedle != nullptr) && (*pNeedle != 0))
    {
        char8_t first = *pNeedle;
        size_t len = strlen(pNeedle);

        for (; (*pHaystack != 0) && (len <= uCount); ++pHaystack, --uCount)
        {
            // small optimization on first character search
            if (*pHaystack == first)
            {
                for (size_t i = 1;; ++i)
                {
                    if (pNeedle[i] == 0)
                    {
                        pMatch = (char8_t*) pHaystack;
                        return(pMatch);
                    }
                    if (pHaystack[i] == 0)
                    {
                        break;
                    }
                    if (pHaystack[i] != pNeedle[i])
                    {
                        break;
                    }
                }
            }
        }
    }
    return(pMatch);
}

/*F********************************************************************************/
/*!
    \Function blaze_strnistr

    \Description
        Case insensitive substring search of first N characters

    \Input see strstr

    \Output see strstr

    \Version 03/31/2009 (dmikulec)
*/
/********************************************************************************F*/
char8_t *blaze_strnistr(const char8_t* pHaystack, const char8_t* pNeedle, size_t uCount)
{
    char8_t *pMatch = nullptr;

    // make sure strings are valid
    if ((pHaystack != nullptr) && (*pHaystack != 0) && (pNeedle != nullptr) && (*pNeedle != 0))
    {
        int32_t first = tolower((unsigned char)*pNeedle);
        size_t len = strlen(pNeedle);

        for (; (*pHaystack != 0) && (len <= uCount); ++pHaystack, --uCount)
        {
            // small optimization on first character search
            if (tolower((unsigned char)*pHaystack) == first)
            {
                for (size_t i = 1;; ++i)
                {
                    if (pNeedle[i] == 0)
                    {
                        pMatch = (char8_t*) pHaystack;
                        return(pMatch);
                    }
                    if (pHaystack[i] == 0)
                    {
                        break;
                    }
                    if (tolower((unsigned char)pHaystack[i]) != tolower((unsigned char)pNeedle[i]))
                    {
                        break;
                    }
                }
            }
        }
    }
    return(pMatch);
}

/*F********************************************************************************/
/*!
    \Function blaze_strdup

    \Description
        strdup variant that uses BLAZE_ALLOC (instead of malloc).  Caller's responsibility
        free (BLAZE_FREE) the string.
        
    \param pString     pointer to string to be dupicated
    \param memGroupId  mem group to be used by this class to allocate memory
*/
/********************************************************************************F*/
char8_t *blaze_strdup(const char8_t *pString, Blaze::MemoryGroupId memGroupId)
{
    size_t size = strlen(pString) + 1;
    char8_t* newStr = (char8_t *)BLAZE_ALLOC_MGID(size, memGroupId, "blaze_strdup");
    memcpy(newStr, pString, size);
    return newStr;
}


/*F********************************************************************************/
/*!
    \Function blaze_strcmp

    \Description
        Case sensitive string compare

    \Input see strcmp

    \Output see strcmp

    \Version 05/23/2007 (mlam)
*/
/********************************************************************************F*/
int32_t blaze_strcmp(const char8_t* pString1, const char8_t* pString2)
{
#if defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_WINDOWS)
    return strcmp(pString1, pString2);
#else
    int32_t r;
    char8_t c1, c2;

    do {
        c1 = *pString1++;
        c2 = *pString2++;
        r = c1-c2;
    } while ((c1 != 0) && (r == 0));

    return(r);
#endif
} // blaze_strcmp

/*F********************************************************************************/
/*!
    \Function blaze_stricmp

    \Description
        Case insensitive string compare

    \Input see stricmp

    \Output see stricmp

    \Version 01/25/2005 (gschaefer)
*/
/********************************************************************************F*/
int32_t blaze_stricmp(const char8_t* pString1, const char8_t* pString2)
{
#if defined(EA_PLATFORM_LINUX)
    return strcasecmp(pString1, pString2);
#elif defined(EA_PLATFORM_WINDOWS)
    return _stricmp(pString1, pString2);
#else
    int32_t r;
    char8_t c1, c2;

    do {
        c1 = *pString1++;
        if ((c1 >= 'A') && (c1 <= 'Z'))
            c1 ^= 32;
        c2 = *pString2++;
        if ((c2 >= 'A') && (c2 <= 'Z'))
            c2 ^= 32;
        r = c1-c2;
    } while ((c1 != 0) && (r == 0));

    return(r);
#endif
}

/*F********************************************************************************/
/*!
    \Function blaze_strncmp

    \Description
        Case sensitive string compare of first N characters

    \Input see strncmp

    \Output see strncmp

    \Version 05/23/2007 (mlam)
*/
/********************************************************************************F*/
int32_t blaze_strncmp(const char8_t* pString1, const char8_t* pString2, size_t uCount)
{
#if defined(EA_PLATFORM_LINUX) || defined(EA_PLATFORM_WINDOWS)
    return strncmp(pString1, pString2, uCount);
#else
    int32_t r;
    char8_t c1, c2;
    uint32_t uPos;

    if (uCount == 0)
        return (0);

    uPos = 0;
    do {
        c1 = *pString1++;
        c2 = *pString2++;
        r = c1-c2;
        uPos++;
    } while ((c1 != 0) && (r == 0) && (uPos < uCount));

    return(r);
#endif
} // blaze_strncmp

/*F********************************************************************************/
/*!
    \Function blaze_strnicmp

    \Description
        Case insensitive string compare of first N characters

    \Input see strnicmp

    \Output see strnicmp

    \Version 01/25/2005 (gschaefer)
*/
/********************************************************************************F*/
int32_t blaze_strnicmp(const char8_t* pString1, const char8_t* pString2, size_t uCount)
{
#if defined(EA_PLATFORM_LINUX)
    return strncasecmp(pString1, pString2, uCount);
#elif defined(EA_PLATFORM_WINDOWS)
    return _strnicmp(pString1, pString2, uCount);
#else
    int32_t r;
    char8_t c1, c2;
    uint32_t uPos;

    if (uCount == 0)
        return (0);

    uPos = 0;
    do {
        c1 = *pString1++;
        if ((c1 >= 'A') && (c1 <= 'Z'))
            c1 ^= 32;
        c2 = *pString2++;
        if ((c2 >= 'A') && (c2 <= 'Z'))
            c2 ^= 32;
        r = c1-c2;
        uPos++;
    } while ((c1 != 0) && (r == 0) && (uPos < uCount));

    return(r);
#endif
}

/*F********************************************************************************/
/*!
    \Function blaze_strtok

    \Description
        Re-entrant version of strtok function

    \Input see strtok_r on Linux or strtok_s on Windows

    \Output see strtok_r on Linux or strtok_s on Windows

    \Version 08/01/2008 (wsantee)
*/
/********************************************************************************F*/
char8_t *blaze_strtok(char8_t *str, const char8_t *delim, char **saveptr)
{
#if defined(EA_PLATFORM_WINDOWS)
    return strtok_s(str, delim, saveptr);
#elif defined(EA_PLATFORM_LINUX)
    return strtok_r(str, delim, saveptr);
#else
    BlazeAssertMsg(false, "blaze_strtok not implemented for this platform!");
    return nullptr;
#endif
}

/*F********************************************************************************/
/*!
    \Function blaze_strnzcpy

    \Description
        Always terminated strncpy function

    \Input see strncpy

    \Output see strncpy

    \Version 01/25/2005 (gschaefer)
*/
/********************************************************************************F*/
char8_t *blaze_strnzcpy(char8_t* pDest, const char8_t* pSource, size_t uCount)
{
    if (uCount > 0)
    {
        if (pSource == nullptr)
            pDest[0] = 0;
        else
        {
            strncpy(pDest, pSource, uCount-1);
            pDest[uCount-1] = 0;
        }
    }
    return(pDest);
}

/*F********************************************************************************/
/*!
    \Function blaze_strsubzcpy

    \Description
        Copy a substring from pSrc into the output buffer.  The output string
        is guaranteed to be null-terminated.

    \Input *pDst    - [out] output for new string
    \Input uDstLen  - length of output buffer
    \Input *pSrc    - pointer to input string to copy from
    \Input uSrcLen  - number of characters to copy from input string

    \Output
        uint32_t         - number of characters written, excluding null character

    \Version 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
size_t blaze_strsubzcpy(char8_t* pDst, size_t uDstLen, const char8_t* pSrc, size_t uSrcLen)
{
    size_t uIndex;

    // make sure buffer has enough room
    if (uDstLen == 0)
    {
        return(0);
    }
    //Take off one for the null
    uDstLen--;

    // copy the string
    for (uIndex = 0; (uIndex < uSrcLen) && (uIndex < uDstLen) && (pSrc[uIndex] != '\0'); uIndex++)
    {
        pDst[uIndex] = pSrc[uIndex];
    }

    // write null terminator and return number of bytes written
    pDst[uIndex] = '\0';
    return(uIndex);
}

/*F********************************************************************************/
/*!
    \Function blaze_strnzcat

    \Description
        Concatenate the string pointed to by pSrc to the string pointed to by pDst.
        A maximum of uDstLen-1 characters are copied, and the resulting string is
        guaranteed to be null-terminated.

    \Input *pDst    - [out] output for new string
    \Input *pSrc    - pointer to input string to copy from
    \Input uDstLen  - size of output buffer

    \Output
        size_t         - number of characters in pDst, excluding null character

    \Version 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
size_t blaze_strnzcat(char8_t* pDst, const char8_t* pSrc, size_t uDstLen)
{
    size_t uDst, uSrc;

    // make sure buffer has enough room
    if (pSrc == nullptr || uDstLen == 0)
    {
        return(0);
    }
    //Take off one for the null
    uDstLen--;

    // find end of string
    for (uDst = 0; (uDst < uDstLen) && (pDst[uDst] != '\0'); uDst++)
        ;

    // copy the string
    for (uSrc = 0; (uDst < uDstLen) && (pSrc[uSrc] != '\0'); uSrc++, uDst++)
    {
        pDst[uDst] = pSrc[uSrc];
    }

    // write null terminator and return updated length of string
    pDst[uDst] = '\0';
    return(uDst);
}

/*F********************************************************************************/
/*!
    \Function blaze_strsubzcat

    \Description
        Concatenate the substring pointed to by pSrc and with a length of uSrcLen
        to the string pointed to by pDst.  A maximum of uDstLen-1 or uSrcLen
        characters are copied, whichever is smaller, and the resulting string is
        guaranteed to be null-terminated.

    \Input *pDst    - [out] output for new string
    \Input uDstLen  - size of output buffer
    \Input *pSrc    - pointer to input string to copy from
    \Input uSrcLen  - size of substring pointed to by pSrc.

    \Output
        size_t         - number of characters in pDst, excluding null character

    \Version 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
size_t blaze_strsubzcat(char8_t* pDst, size_t uDstLen, const char8_t* pSrc, size_t uSrcLen)
{
    size_t uDst, uSrc;

 // make sure buffer has enough room
    if (uDstLen == 0)
    {
        return(0);
    }
    //Take off one for the null
    uDstLen--;

    // find end of string
    for (uDst = 0; (uDst < uDstLen) && (pDst[uDst] != '\0'); uDst++)
        ;

    // copy the string
    for (uSrc = 0; (uDst < uDstLen) && (uSrc < uSrcLen) && (pSrc[uSrc] != '\0'); uSrc++, uDst++)
    {
        pDst[uDst] = pSrc[uSrc];
    }

    // write null terminator and return updated length of string
    pDst[uDst] = '\0';
    return(uDst);

}

/*F********************************************************************************/
/*!
    \Function blaze_strlwr

    \Description
        Convert a string to lowercase.

    \Input *pStr    - String to convert.

    \Output
        None.

    \Version 04/18/2007 (doneill)
*/
/********************************************************************************F*/
void blaze_strlwr(char8_t* pStr)
{
    if (pStr == nullptr)
        return;

    for (; *pStr != 0; pStr++)
        *pStr = static_cast<char8_t>(tolower(*pStr));
}

/*F********************************************************************************/
/*!
    \Function blaze_strupr

    \Description
        Convert a string to uppercase.

    \Input *pStr    - String to convert.

    \Output
        None.

    \Version 04/18/2007 (doneill)
*/
/********************************************************************************F*/
void blaze_strupr(char8_t* pStr)
{
    if (pStr == nullptr)
        return;

    for (; *pStr != 0; pStr++)
        *pStr = static_cast<char8_t>(toupper(*pStr));
}

uint32_t blaze_strhex2int(char8_t* strHex)
{
    size_t len = strlen(strHex);
    uint32_t result = 0;

    for(size_t i=0; i < len; i++)
    {
        int32_t val=0;

        if(strHex[i] >= '0' && strHex[i] <= '9') val = strHex[i] - '0';
        else if(strHex[i] >= 'A' && strHex[i] <= 'F') val = strHex[i] - 'A' + 10;
        else if(strHex[i] >= 'a' && strHex[i] <= 'f') val = strHex[i] - 'a' + 10;

        result = result + (val<<(4*(len-i-1)));
    }

    return result;
}

/*F********************************************************************************/
/*!
    \Function blaze_bin2hex

    \Description
        Returns a hex-encoded string representing an input binary buffer.

    \Input out      - Output buffer.
    \Input outMax   - Should at least be srclen*2+1 length
    \Input src      - Source binary buffer
    \Input srclen   - Size of source binary buffer.

    \Output
        char8_t*    - Output buffer ('out').  nullptr on failure.
*/
/********************************************************************************F*/
char8_t* blaze_bin2hex(char8_t *out, size_t outMax, const uint8_t *src, size_t srclen)
{
     if (src == nullptr || out == nullptr)
         return nullptr;

#ifdef BLAZE_CLIENT_SDK
     BlazeAssert(outMax >= (srclen*2+1));
#else
     EA_ASSERT(outMax >= (srclen*2+1));
#endif
     if (outMax < (srclen*2+1))
         return nullptr;

     char8_t *curout = out;
     for (size_t srcidx = 0; srcidx < srclen; ++srcidx)
     {
         blaze_char2hex(curout[0], curout[1], src[srcidx]);
         curout += 2;
     }

     *curout = '\0';
     return out;
}

char8_t* blaze_str2flt(const char8_t *ptr, float &value)
{
    char8_t *result = nullptr;
    value = (float) strtod(ptr, &result);
    return result;
}

char8_t* blaze_str2dbl(const char8_t *ptr, double &value)
{
    char8_t *result = nullptr;
    value = strtod(ptr, &result);
    return result;
}

void blaze_strcpyFixedLengthToNullTerminated(char8_t *dest, size_t destMax, const  char8_t *source, size_t sourceSize)
{
    // Value of 0 would result in integer underflow for copySize
#ifdef BLAZE_CLIENT_SDK
     BlazeAssert(destMax != 0);
#else
     EA_ASSERT(destMax != 0);
#endif
    if (destMax == 0)
        return;

    size_t copySize = ((sourceSize + 1) > destMax) ? (destMax-1) : sourceSize;
    memcpy(dest, source, copySize);
    dest[copySize] = '\0';
}

size_t CaseInsensitiveStringHash::operator()(const char8_t* p) const
{
    size_t c, result = 2166136261U;
    while((c = (uint8_t)(tolower(*p++))) != 0)
        result = (result * 16777619) ^ c;
    return (size_t)result;
}


/*F********************************************************************************/
/*!
    \Function blaze_stripWhitespace

    \Description
        Remove all whitespaces from the given constant string (string.c_str()) and return it. 

    \Input *src  - array containing string to be stripped of whitespaces.
    \Input len   - size of array (size of dest array).

    \Output
        *dest - array containing the result string.

    \Version 11/08/2010 (gakulkarni)
*/
/********************************************************************************F*/


char8_t* blaze_stripWhitespace(const char8_t* src, char8_t* dest, uint32_t len)
{
    uint32_t i=0, j=0;

    while (src[i] != '\0' && j < len)
    {
        if (src[i] != ' ' && src[i] != '\n' && src[i] != '\t' && src[i] != '\r')
        {
            dest[j++] = src[i];
        }

        i++;
    }
    dest[j] = '\0';

    return dest;
}
