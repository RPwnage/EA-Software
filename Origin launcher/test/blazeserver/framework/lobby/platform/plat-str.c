/*H********************************************************************************/
/*!
    \File plat-str.c

    \Description
        This file provides platform independent versions of some standard-C
        functions that are not "standard" across platforms and/or fixes 
        implementation problems with the standard versions (such as consistent
        termination).

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 01/11/2005 (gschaefer) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdarg.h>     // NOTE - must come before stdio for IOP platform, or vsprintf is not defined
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>      // tolower

#undef DS_PLATFORM
#include "platform.h"

/*** Defines **********************************************************************/

#if (DIRTYCODE_PLATFORM == DIRTYCODE_PC)
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#endif

/*** Type Definitions *************************************************************/

/*** Variables ********************************************************************/

/*** Public functions *************************************************************/

/*F********************************************************************************/
/*!
    \Function ds_vsnzprintf

    \Description
        Replacement for vsnprintf that always includes a terminator at the end of
        the string.

    \Input see vsnprintf
    
    \Output see vsnprintf

    \Version 12/20/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_vsnzprintf(char *pBuffer, int32_t iLength, const char *pFormat, va_list Args)
{
    int32_t iResult;

    // always terminate
    pBuffer[0] = '\0';

    // format the text    
    #if DIRTYCODE_PLATFORM != DIRTYCODE_IOP
    iResult = vsnprintf(pBuffer, iLength, pFormat, Args);
    pBuffer[iLength-1] = '\0';
    #else
    iResult = vsprintf(pBuffer, pFormat, Args);
    #if DIRTYCODE_DEBUG
    if (iResult >= iLength)
    {
        printf("plat-str: warning - buffer overflow in vsprintf call, trashed %d bytes\n", iResult-iLength);
    }
    #endif
    #endif
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ds_snzprintf

    \Description
        Replacement for snprintf that always includes a terminator at the end of
        the string.

    \Input see snprintf
    
    \Output see snprintf

    \Version 01/25/2005 (gschaefer)
*/
/********************************************************************************F*/
int32_t ds_snzprintf(char *pBuffer, int32_t iLength, const char *pFormat, ...)
{
    int32_t iResult;
    va_list args;

    // format the text
    va_start(args, pFormat);
    pBuffer[0] = 0;
    #if DIRTYCODE_PLATFORM != DIRTYCODE_IOP
    iResult = vsnprintf(pBuffer, iLength, pFormat, args);
    pBuffer[iLength-1] = 0;
    #else
    iResult = vsprintf(pBuffer, pFormat, args);
    #if DIRTYCODE_DEBUG
    if (iResult >= iLength)
    {
        printf("plat-str: warning - buffer overflow in vsprintf call, trashed %d bytes\n", iResult-iLength);
    }
    #endif
    #endif
    va_end(args);
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function ds_stristr

    \Description
        Case insensitive substring search

    \Input see strstr
    
    \Output see strstr

    \Version 01/25/2005 (gschaefer)
*/
/********************************************************************************F*/
char *ds_stristr(const char *pHaystack, const char *pNeedle)
{
    int32_t iFirst;
    int32_t iIndex;
    char *pMatch = NULL;

    // make sure strings are valid
    if ((pHaystack != NULL) && (*pHaystack != 0) && (pNeedle != NULL) && (*pNeedle != 0))
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
                        pMatch = (char *)pHaystack;
                        pHaystack = "";
                        break;
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
    \Function ds_stricmp

    \Description
        Case insensitive string compare

    \Input see stricmp
    
    \Output see stricmp

    \Version 01/25/2005 (gschaefer)
*/
/********************************************************************************F*/
int32_t ds_stricmp(const char *pString1, const char *pString2)
{
    int32_t r;
    char c1, c2;

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
}

/*F********************************************************************************/
/*!
    \Function ds_strnicmp

    \Description
        Case insensitive string compare of first N characters

    \Input see strnicmp
    
    \Output see strnicmp

    \Version 01/25/2005 (gschaefer)
*/
/********************************************************************************F*/
int32_t ds_strnicmp(const char *pString1, const char *pString2, uint32_t uCount)
{
    int32_t r;
    char c1, c2;
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
}

/*F********************************************************************************/
/*!
    \Function ds_strnzcpy

    \Description
        Always terminated strncpy function

    \Input see strncpy
    
    \Output see strncpy

    \Version 01/25/2005 (gschaefer)
*/
/********************************************************************************F*/
char *ds_strnzcpy(char *pDest, const char *pSource, int32_t iCount)
{
    strncpy(pDest, pSource, iCount);
    pDest[iCount-1] = 0;
    return(pDest);
}

/*F********************************************************************************/
/*!
    \Function ds_strsubzcpy

    \Description
        Copy a substring from pSrc into the output buffer.  The output string
        is guaranteed to be null-terminated.

    \Input *pDst    - [out] output for new string
    \Input iDstLen  - length of output buffer
    \Input *pSrc    - pointer to input string to copy from
    \Input iSrcLen  - number of characters to copy from input string
    
    \Output
        int32_t         - number of characters written, excluding null character

    \Version 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_strsubzcpy(char *pDst, int32_t iDstLen, const char *pSrc, int32_t iSrcLen)
{
    int32_t iIndex;
    
    // make sure buffer has enough room
    if (--iDstLen < 0)
    {
        return(0);
    }
    
    // copy the string
    for (iIndex = 0; (iIndex < iSrcLen) && (iIndex < iDstLen) && (pSrc[iIndex] != '\0'); iIndex++)
    {
        pDst[iIndex] = pSrc[iIndex];
    }
    
    // write null terminator and return number of bytes written
    pDst[iIndex] = '\0';
    return(iIndex);
}

/*F********************************************************************************/
/*!
    \Function ds_strnzcat

    \Description
        Concatenate the string pointed to by pSrc to the string pointed to by pDst.
        A maximum of iDstLen-1 characters are copied, and the resulting string is
        guaranteed to be null-terminated.

    \Input *pDst    - [out] output for new string
    \Input *pSrc    - pointer to input string to copy from
    \Input iDstLen  - size of output buffer
    
    \Output
        int32_t         - number of characters in pDst, excluding null character

    \Version 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_strnzcat(char *pDst, const char *pSrc, int32_t iDstLen)
{
    int32_t iDst, iSrc;

    // make sure buffer has enough room
    if (--iDstLen < 0)
    {
        return(0);
    }
    
    // find end of string
    for (iDst = 0; (iDst < iDstLen) && (pDst[iDst] != '\0'); iDst++)
        ;
        
    // copy the string
    for (iSrc = 0; (iDst < iDstLen) && (pSrc[iSrc] != '\0'); iSrc++, iDst++)
    {
        pDst[iDst] = pSrc[iSrc];
    }
    
    // write null terminator and return updated length of string
    pDst[iDst] = '\0';
    return(iDst);
}

/*F********************************************************************************/
/*!
    \Function ds_strsubzcat

    \Description
        Concatenate the substring pointed to by pSrc and with a length of iSrcLen
        to the string pointed to by pDst.  A maximum of iDstLen-1 or iSrcLen
        characters are copied, whichever is smaller, and the resulting string is
        guaranteed to be null-terminated.

    \Input *pDst    - [out] output for new string
    \Input iDstLen  - size of output buffer
    \Input *pSrc    - pointer to input string to copy from
    \Input iSrcLen  - size of substring pointed to by pSrc.
    
    \Output
        int32_t         - number of characters in pDst, excluding null character

    \Version 09/27/2005 (jbrookes)
*/
/********************************************************************************F*/
int32_t ds_strsubzcat(char *pDst, int32_t iDstLen, const char *pSrc, int32_t iSrcLen)
{
    int32_t iDst, iSrc;

    // make sure buffer has enough room
    if (--iDstLen < 0)
    {
        return(0);
    }
    
    // find end of string
    for (iDst = 0; (iDst < iDstLen) && (pDst[iDst] != '\0'); iDst++)
        ;
        
    // copy the string
    for (iSrc = 0; (iDst < iDstLen) && (iSrc < iSrcLen) && (pSrc[iSrc] != '\0'); iSrc++, iDst++)
    {
        pDst[iDst] = pSrc[iSrc];
    }
    
    // write null terminator and return updated length of string
    pDst[iDst] = '\0';
    return(iDst);

}
