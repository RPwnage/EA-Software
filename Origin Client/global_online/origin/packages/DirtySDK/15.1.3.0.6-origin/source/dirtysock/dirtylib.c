/*H********************************************************************************/
/*!
    \File dirtylib.c

    \Description
        Platform independent routines for support library for network code.

    \Copyright
        Copyright (c) 2005 Electronic Arts Inc.

    \Version 09/15/1999 (gschaefer) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>

#include "DirtySDK/dirtysock.h"

#if defined(DIRTYCODE_PC) || defined(DIRTYCODE_XBOXONE)
#include <windows.h>
#endif

/*** Defines **********************************************************************/

// max size of line that will be tracked for netprint rate limiting
#define NETPRINT_RATELIMIT_MAXSTRINGLEN (256)

/*** Type Definitions *************************************************************/

// rate limit info
typedef struct NetPrintRateLimitT
{
    NetCritT RateCrit;
    uint32_t uRateLimitCounter;
    uint32_t uPrintTime;
    uint8_t  bRateLimitInProgress;
    char strRateLimitBuffer[NETPRINT_RATELIMIT_MAXSTRINGLEN+32];
} NetPrintRateLimitT;

/*** Variables ********************************************************************/

#if DIRTYCODE_LOGGING
// time stamp functionality
static uint8_t _NetLib_bEnableTimeStamp = TRUE;

// rate limit variables
static NetPrintRateLimitT _NetLib_RateLimit;
static uint8_t _NetLib_bRateLimitInitialized = FALSE;

// debugging hooks
static void *_NetLib_pDebugParm = NULL;
static int32_t (*_NetLib_pDebugHook)(void *pParm, const char *pText) = NULL;
#endif

// idle critical section
static NetCritT _NetLib_IdleCrit;

// idle task list
static struct
{
    void (*pProc)(void *pRef);
    void *pRef;
} _NetLib_IdleList[32];

// number of installed idle task handlers
static int32_t _NetLib_iIdleSize = 0;


/*** Private Functions ************************************************************/


#if DIRTYCODE_LOGGING
/*F*************************************************************************************************/
/*!
    \Function    _NetPrintRateLimit

    \Description
        Rate limit NetPrintf output; identical lines of fewer than 256 characters that are
        line-terminated (end in a \n) will be suppressed, until a new line of text is output or
        a 100ms timer elapses.  At that point the line will be output, with additional text
        indicating the number of lines that would have been output in total.  A critical section
        guarantees integrity of the buffer and tracking information, but it is only tried to prevent
        any possibility of deadlock.  In the case of a critical section try failure, rate limit
        processing is skipped until the next print statement.

    \Input *pText   - output text to (potentially) rate limit

    \Output
        int32_t     - zero=not rate limited (line should be printed), one=rate limited (do not print)

    \Version 04/05/2016 (jbrookes)
*/
/*************************************************************************************************F*/
static int32_t _NetPrintRateLimit(const char *pText)
{
    NetPrintRateLimitT *pRateLimit = &_NetLib_RateLimit;
    int32_t iTextLen = (int32_t)strlen(pText);
    uint32_t uCurTick = NetTick(), uDiffTick = 0;
    int32_t iStrCmp, iResult = 0;
    /* rate limit IFF:
       - output is relatively small & line-terminated
       - we have initialized rate limiting
       - we are not printing rate-limited text ourselves
       - we can secure access to the critical section (MUST COME LAST!) */
    if ((iTextLen >= NETPRINT_RATELIMIT_MAXSTRINGLEN) || (pText[iTextLen-1] != '\n') || !_NetLib_bRateLimitInitialized || pRateLimit->bRateLimitInProgress || !NetCritTry(&pRateLimit->RateCrit))
    {
        return(iResult);
    }
    // does the string match our current string?
    if ((iStrCmp = strcmp(pText, pRateLimit->strRateLimitBuffer)) == 0)
    {
        // if yes, calculate tick difference between now and when the line was buffered
        uDiffTick = NetTickDiff(uCurTick, pRateLimit->uPrintTime);
    }
    // if we have a line we are rate-limiting, and either the timeout has elapsed or there is a new line
    if ((pRateLimit->uRateLimitCounter > 1) && ((uDiffTick > 100) || (iStrCmp != 0)))
    {
        // print the line, tagging on how many times it was printed
        iTextLen = (int32_t)strlen(pRateLimit->strRateLimitBuffer);
        pRateLimit->strRateLimitBuffer[iTextLen-1] = '\0';
        pRateLimit->bRateLimitInProgress = TRUE;
        NetPrintfCode("%s (%d times)\n", pRateLimit->strRateLimitBuffer, pRateLimit->uRateLimitCounter-1);
        pRateLimit->bRateLimitInProgress = FALSE;
        // set compare to non-matching to force restart below
        iStrCmp = 1;
    }
    // if this line doesn't match our current buffered line, buffer it
    if (iStrCmp != 0)
    {
        ds_strnzcpy(pRateLimit->strRateLimitBuffer, pText, sizeof(pRateLimit->strRateLimitBuffer));
        pRateLimit->uRateLimitCounter = 1;
        pRateLimit->uPrintTime = NetTick();
    }
    else
    {
        // match, so increment rate counter and suppress the line
        pRateLimit->uRateLimitCounter += 1;
        iResult = 1;
    }
    NetCritLeave(&pRateLimit->RateCrit);
    return(iResult);
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetTimeStamp

    \Description
        A printf function that returns a date time string for time stamp.
        Only if time stamps is enabled

    \Input *pBuffer - pointer to format time stamp string
    \Input iLen     - length of the supplied buffer

    \Output
        int32_t     - return the length of the Time Stamp String

    \Version 09/10/2013 (tcho)
*/
/********************************************************************************F*/
static int32_t _NetTimeStamp(char *pBuffer, int32_t iLen)
{
    int32_t iTimeStampLen = 0;

    if (_NetLib_bEnableTimeStamp == TRUE)
    {
        struct tm tm;
        int32_t imsec;

        ds_plattimetotimems(&tm, &imsec);
        iTimeStampLen = ds_snzprintf(pBuffer, iLen,"%d/%02d/%02d-%02d:%02d:%02d.%03.3d ", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, imsec);
    }

    return(iTimeStampLen);
}
#endif

/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function NetLibCommonInit

    \Description
        NetLib platform-common initialization

    \Version 04/05/2016 (jbrookes)
*/
/********************************************************************************F*/
void NetLibCommonInit(void)
{
    // reset the idle list
    NetIdleReset();

    // initialize critical sections
    NetCritInit(NULL, "lib-global");
    NetCritInit(&_NetLib_IdleCrit, "lib-idle");

    // set up rate limiting
    #if DIRTYCODE_LOGGING
    ds_memclr(&_NetLib_RateLimit, sizeof(_NetLib_RateLimit));
    NetCritInit(&_NetLib_RateLimit.RateCrit, "lib-rate");
    _NetLib_bRateLimitInitialized = TRUE;
    #endif
}

/*F********************************************************************************/
/*!
    \Function NetLibCommonShutdown

    \Description
        NetLib platform-common shutdown

    \Version 04/05/2016 (jbrookes)
*/
/********************************************************************************F*/
void NetLibCommonShutdown(void)
{
    // kill critical sections
    #if DIRTYCODE_LOGGING
    _NetLib_bRateLimitInitialized = FALSE;
    NetCritKill(&_NetLib_RateLimit.RateCrit);
    #endif
    NetCritKill(&_NetLib_IdleCrit);
    NetCritKill(NULL);
}

/*F********************************************************************************/
/*!
    \Function NetIdleReset

    \Description
        Reset idle function count.

    \Version 06/21/2006 (jbrookes)
*/
/********************************************************************************F*/
void NetIdleReset(void)
{
    _NetLib_iIdleSize = 0;
}

/*F********************************************************************************/
/*!
    \Function NetIdleAdd

    \Description
        Add a function to the idle callback list. The functions are called whenever
        NetIdleCall() is called.

    \Input *pProc   - callback function pointer
    \Input *pRef    - function specific parameter

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void NetIdleAdd(void (*pProc)(void *pRef), void *pRef)
{
    // make sure proc is valid
    if (pProc == NULL)
    {
        NetPrintf(("dirtylib: attempt to add an invalid idle function\n"));
        return;
    }

    // add item to list
    _NetLib_IdleList[_NetLib_iIdleSize].pProc = pProc;
    _NetLib_IdleList[_NetLib_iIdleSize].pRef = pRef;
    _NetLib_iIdleSize += 1;
}

/*F********************************************************************************/
/*!
    \Function NetIdleDel

    \Description
        Remove a function from the idle callback list.

    \Input *pProc   - callback function pointer
    \Input *pRef    - function specific parameter

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void NetIdleDel(void (*pProc)(void *pRef), void *pRef)
{
    int32_t iProc;

    // make sure proc is valid
    if (pProc == NULL)
    {
        NetPrintf(("dirtylib: attempt to delete an invalid idle function\n"));
        return;
    }

    // mark item as deleted
    for (iProc = 0; iProc < _NetLib_iIdleSize; ++iProc)
    {
        if ((_NetLib_IdleList[iProc].pProc == pProc) && (_NetLib_IdleList[iProc].pRef == pRef))
        {
            _NetLib_IdleList[iProc].pProc = NULL;
            _NetLib_IdleList[iProc].pRef = NULL;
            break;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function NetIdleDone

    \Description
        Make sure all idle calls have completed

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void NetIdleDone(void)
{
    NetCritEnter(&_NetLib_IdleCrit);
    NetCritLeave(&_NetLib_IdleCrit);
}

/*F********************************************************************************/
/*!
    \Function NetIdleCall

    \Description
        Call all of the functions in the idle list.

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
void NetIdleCall(void)
{
    int32_t iProc;

    // only do idle call if we have control
    if (NetCritTry(&_NetLib_IdleCrit))
    {
        // walk the table calling routines
        for (iProc = 0; iProc < _NetLib_iIdleSize; ++iProc)
        {
            // get pProc pointer
            void (*pProc)(void *pRef) = _NetLib_IdleList[iProc].pProc;
            void *pRef = _NetLib_IdleList[iProc].pRef;

            /* if pProc is deleted, handle removal here (this
               helps prevent corrupting table in race condition) */
            if (pProc == NULL)
            {
                // swap with final element
                _NetLib_IdleList[iProc].pProc = _NetLib_IdleList[_NetLib_iIdleSize-1].pProc;
                _NetLib_IdleList[iProc].pRef = _NetLib_IdleList[_NetLib_iIdleSize-1].pRef;
                _NetLib_IdleList[_NetLib_iIdleSize-1].pProc = NULL;
                _NetLib_IdleList[_NetLib_iIdleSize-1].pRef = NULL;

                // drop the item count
                _NetLib_iIdleSize -= 1;

                // restart the loop at new current element
                --iProc;
                continue;
            }
            // perform the idle call
            (*pProc)(pRef);
        }
        // done with critical section
        NetCritLeave(&_NetLib_IdleCrit);
    }
}

/*F********************************************************************************/
/*!
    \Function NetHash

    \Description
        Calculate a unique 32-bit hash based on the given input string.

    \Input  *pString    - pointer to string to calc hash of

    \Output
        int32_t         - resultant 32bit hash

    \Version 2.0 07/26/2011 (jrainy) rewrite to lower collision rate
*/
/********************************************************************************F*/
int32_t NetHash(const char *pString)
{
    return(NetHashBin(pString, (uint32_t)strlen(pString)));
}

/*F********************************************************************************/
/*!
    \Function NetHashBin

    \Description
        Calculate a unique 32-bit hash based on the given buffer.

    \Input  *pBuffer    - pointer to buffer to calc hash of
    \Input  *uLength    - length of buffer to calc hash of

    \Output
        int32_t         - resultant 32bit hash

    \Version 1.0 02/14/2011 (jrainy) First Version
*/
/********************************************************************************F*/
int32_t NetHashBin(const void *pBuffer, uint32_t uLength)
{
    // prime factor to multiply by at each 16-char block boundary
    static uint32_t uShift = 436481627;

    // prime factors to multiply individual characters by
    static uint32_t uFactors[16] = {
        682050377,  933939593,  169587707,  131017121,
        926940523,  102453581,  543947221,  775968049,
        129461173,  793216343,  870352919,  455044847,
        747808279,  727551509,  431178773,  519827743};

    // running hash
    uint32_t uSum = 0;
    uint32_t uChar;

    for (uChar = 0; uChar != uLength; uChar++)
    {
        // at each 16-byte boundary, multiply the running hash by fixed factor
        if ((uChar & 0xf) == 0)
        {
            uSum *= uShift;
        }
        // sum up the value of the char at position iChar by prime factor iChar%16
        uSum += ((uint8_t)((char*)pBuffer)[uChar]) * uFactors[uChar & 0xf];
    }

    return((int32_t)uSum);
}

/*F*************************************************************************************************/
/*!
    \Function NetRand

    \Description
        A simple pseudo-random sequence generator. The sequence is implicitly seeded in the first
        call with the millisecond tick count at the time of the call

    \Input uLimit   - upper bound of pseudo-random number output

    \Output
        uint32_t     - pseudo-random number from [0...(uLimit - 1)]

    \Version 06/25/2009 (jbrookes)
*/
/*************************************************************************************************F*/
uint32_t NetRand(uint32_t uLimit)
{
    static uint32_t _aRand = 0;
    if (_aRand == 0)
    {
        _aRand = NetTick();
    }
    if (uLimit == 0)
    {
        return(0);
    }
    _aRand = (_aRand * 125) % 2796203;
    return(_aRand % uLimit);
}

/*F********************************************************************************/
/*!
    \Function NetTime

    \Description
        This function replaces the standard library time() function. Main
        differences are the missing pointer parameter (not needed) and the uint64_t
        return value. The function returns 0 on unsupported platforms vs time which
        returns -1.

    \Output
        uint64_t    - number of elapsed seconds since Jan 1, 1970.

    \Version 01/12/2005 (gschaefer)
*/
/********************************************************************************F*/
uint64_t NetTime(void)
{
    return((uint64_t)time(NULL));
}


#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetTimeStampEnableCode

    \Description
        Enables Time Stamp in Logging

    \Input bEnableTimeStamp  - TRUE to enable Time Stamp in Logging

    \Version 9/11/2014 (tcho)
*/
/********************************************************************************F*/
void NetTimeStampEnableCode(uint8_t bEnableTimeStamp)
{
    _NetLib_bEnableTimeStamp = bEnableTimeStamp;
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetPrintfHook

    \Description
        Hook into debug output.

    \Input *pPrintfDebugHook    - pointer to function to call with debug output
    \Input *pParm               - user parameter

    \Version 03/29/2005 (jbrookes)
*/
/********************************************************************************F*/
void NetPrintfHook(int32_t (*pPrintfDebugHook)(void *pParm, const char *pText), void *pParm)
{
    _NetLib_pDebugHook = pPrintfDebugHook;
    _NetLib_pDebugParm = pParm;
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetPrintfCode

    \Description
        Debug formatted output

    \Input *pFormat - pointer to format string
    \Input ...      - variable argument list

    \Output
        int32_t     - zero

    \Version 09/15/1999 (gschaefer)
*/
/********************************************************************************F*/
int32_t NetPrintfCode(const char *pFormat, ...)
{
    va_list pFmtArgs;
    char strText[4096];
    const char *pText = strText;
    int32_t iOutput = 1;
    int32_t iTimeStampLen = 0;

    // init the string buffer (not done at instantiation so as to avoid having to clear the entire array, for perf)
    strText[0] = '\0';

    // only returns time stamp if time stamp is enabled
    iTimeStampLen = _NetTimeStamp(strText, sizeof(strText));

    // format the text
    va_start(pFmtArgs, pFormat);
    if ((pFormat[0] == '%') && (pFormat[1] == 's') && (pFormat[2] == '\0'))
    {
        ds_strnzcat(strText + iTimeStampLen, va_arg(pFmtArgs, const char *), sizeof(strText) - iTimeStampLen);
    }
    else
    {
        ds_vsnprintf(strText + iTimeStampLen, sizeof(strText) - iTimeStampLen, pFormat, pFmtArgs);
    }
    va_end(pFmtArgs);

    // check for rate limit (omit timestamp from consideration)
    if (_NetPrintRateLimit(strText+iTimeStampLen))
    {
        return(0);
    }

    // forward to debug hook, if defined
    if (_NetLib_pDebugHook != NULL)
    {
       iOutput = _NetLib_pDebugHook(_NetLib_pDebugParm, pText);
    }

    // output to debug output, unless suppressed by debug hook
    if (iOutput != 0)
    {
        #if defined(DIRTYCODE_PC) || defined(DIRTYCODE_XBOXONE)
        OutputDebugStringA(pText);
        #else
        printf("%s", pText);
        #endif
    }

    return(0);
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetPrintfVerboseCode

    \Description
        Display input data if iVerbosityLevel is > iCheckLevel

    \Input iVerbosityLevel  - current verbosity level
    \Input iCheckLevel      - level to check against
    \Input *pFormat         - format string

    \Version 12/20/2005 (jbrookes)
*/
/********************************************************************************F*/
void NetPrintfVerboseCode(int32_t iVerbosityLevel, int32_t iCheckLevel, const char *pFormat, ...)
{
    va_list Args;
    char strText[1024];

    va_start(Args, pFormat);
    ds_vsnprintf(strText, sizeof(strText), pFormat, Args);
    va_end(Args);

    if (iVerbosityLevel > iCheckLevel)
    {
        NetPrintf(("%s", strText));
    }
}
#endif

#if DIRTYCODE_LOGGING
/*F********************************************************************************/
/*!
    \Function NetPrintWrapCode

    \Description
        Display input data with wrapping.

    \Input *pString - pointer to packet data to display
    \Input iWrapCol - number of columns to wrap at

    \Version 09/15/2005 (jbrookes)
*/
/********************************************************************************F*/
void NetPrintWrapCode(const char *pString, int32_t iWrapCol)
{
    const char *pTemp, *pEnd, *pEqual, *pSpace;
    char strTemp[256] = "   ";
    uint32_t bDone;
    int32_t iLen;

    // loop through whole packet
    for (bDone = FALSE; bDone == FALSE; )
    {
        // scan forward, tracking whitespace, linefeeds, and equal signs
        for (pTemp=pString, pEnd=pTemp+iWrapCol, pSpace=NULL, pEqual=NULL; (pTemp < pEnd); pTemp++)
        {
            // remember most recent whitespace
            if ((*pTemp == ' ') || (*pTemp == '\t'))
            {
                pSpace = pTemp;
            }

            // remember most recent equal sign
            if (*pTemp == '=')
            {
                pEqual = pTemp;
            }

            // if eol or eos, break here
            if ((*pTemp == '\n') || (*pTemp == '\0'))
            {
                break;
            }
        }

        // scanned an entire line?
        if (pTemp == pEnd)
        {
            // see if we have whitespace to break on
            if (pSpace != NULL)
            {
                pTemp = pSpace;
            }
            // see if we have an equals to break on
            else if (pEqual != NULL)
            {
                pTemp = pEqual;
            }
        }

        // format string for output
        iLen = (int32_t)(pTemp - pString + 1);
        strncpy(strTemp + 3, pString, iLen);
        if (*pTemp == '\0')
        {
            strTemp[iLen+2] = '\n';
            strTemp[iLen+3] = '\0';
            bDone = TRUE;
        }
        else if ((*pTemp != '\n') && (*pTemp != '\r'))
        {
            strTemp[iLen+3] = '\n';
            strTemp[iLen+4] = '\0';
        }
        else
        {
            strTemp[iLen+3] = '\0';
        }

        // print it out
        NetPrintf(("%s", strTemp));

        // increment to next line
        pString += iLen;
    }
}
#endif // #if DIRTYCODE_LOGGING

#if DIRTYCODE_LOGGING
/*F*************************************************************************************************/
/*!
    \Function    NetPrintMemCode

    \Description
        Dump memory to debug output

    \Input *pMem    - pointer to memory to dump
    \Input iSize    - size of memory block to dump
    \Input *pTitle  - pointer to title of memory block

    \Version    1.0        05/17/05 (jbrookes) First Version
*/
/*************************************************************************************************F*/
void NetPrintMemCode(const void *pMem, int32_t iSize, const char *pTitle)
{
    static const char _hex[] = "0123456789ABCDEF";
    char strOutput[128];
    int32_t iBytes, iOutput = 2;

    ds_memset(strOutput, ' ', sizeof(strOutput)-1);
    strOutput[sizeof(strOutput)-1] = '\0';

    NetPrintf(("dirtylib: dumping memory for object %s (%d bytes)\n", pTitle, iSize));

    for (iBytes = 0; iBytes < iSize; iBytes++, iOutput += 2)
    {
        unsigned char cByte = ((unsigned char *)pMem)[iBytes];
        strOutput[iOutput]   = _hex[cByte>>4];
        strOutput[iOutput+1] = _hex[cByte&0xf];
        strOutput[(iOutput/2)+40] = isprint(cByte) ? cByte : '.';
        if (iBytes > 0)
        {
            if (((iBytes+1) % 16) == 0)
            {
                strOutput[(iOutput/2)+40+1] = '\0';
                NetPrintf(("%s\n", strOutput));
                ds_memset(strOutput, ' ', sizeof(strOutput)-1);
                strOutput[sizeof(strOutput)-1] = '\0';
                iOutput = 0;
            }
            else if (((iBytes+1) % 4) == 0)
            {
                iOutput++;
            }
        }
    }

    if ((iBytes % 16) != 0)
    {
        strOutput[(iOutput/2)+40+1] = '\0';
        NetPrintf(("%s\n", strOutput));
    }
}
#endif

#if DIRTYCODE_LOGGING
/*F*************************************************************************************************/
/*!
    \Function    NetPrintArrayCode

    \Description
        Dump memory to debug output in the form of a c-style array declaration

    \Input *pMem    - pointer to memory to dump
    \Input iSize    - size of memory block to dump
    \Input *pTitle  - pointer to title of memory block

    \Version 06/05/2014 (jbrookes)
*/
/*************************************************************************************************F*/
void NetPrintArrayCode(const void *pMem, int32_t iSize, const char *pTitle)
{
    static const char _hex[] = "0123456789ABCDEF";
    char strOutput[128];
    int32_t iBytes, iOutput = 4;

    ds_memset(strOutput, ' ', sizeof(strOutput)-1);
    strOutput[sizeof(strOutput)-1] = '\0';

    NetPrintf(("dirtylib: dumping declaration for object %s (%d bytes)\n", pTitle, iSize));
    NetPrintf(("static const uint8_t %s[] =\n{\n", pTitle));

    for (iBytes = 0; iBytes < iSize; iBytes++)
    {
        uint8_t cByte = ((uint8_t *)pMem)[iBytes];
        strOutput[iOutput+0] = '0';
        strOutput[iOutput+1] = 'x';
        strOutput[iOutput+2] = _hex[cByte>>4];
        strOutput[iOutput+3] = _hex[cByte&0xf];
        strOutput[iOutput+4] = ',';
        iOutput += 5;
        if (iBytes > 0)
        {
            if (((iBytes+1) % 16) == 0)
            {
                strOutput[iOutput] = '\0';
                NetPrintf(("%s\n", strOutput));
                ds_memset(strOutput, ' ', sizeof(strOutput)-1);
                strOutput[sizeof(strOutput)-1] = '\0';
                iOutput = 4;
            }
            else if (((iBytes+1) % 4) == 0)
            {
                iOutput++;
            }
        }
    }

    if ((iBytes % 16) != 0)
    {
        strOutput[iOutput] = '\0';
        NetPrintf(("%s\n", strOutput));
    }

    NetPrintf(("};\n", pTitle));
}
#endif
