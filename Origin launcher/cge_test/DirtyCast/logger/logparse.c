/*H********************************************************************************/
/*!
    \File logparse.c

    \Description
        Basic server-inspecific log parsing for servers using 'eafn' logger

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 10/23/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"

#include "logparse.h"

/*** Defines **********************************************************************/

#define LOGPARSE_LINEBUF_ALLOCSIZE (4096)

/*** Type Definitions *************************************************************/

//!< extraction function type
typedef int32_t (LogParseLineExtractFuncT)(LogParseRefT *pLogParse, LogParseLineRefT *pLine1, const void *pExtractData);

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/


/*F********************************************************************************/
/*!
    \Function _ds_strisubstr

    \Description
        Case insensitive substring search within a limited search buffer

    \Input *pHaystack   - see stristr
    \Input *pNeedle     - see stristr
    \Input iHaystackLen - max length to search in

    \Output see stristr

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static char *_ds_strisubstr(const char *pHaystack, const char *pNeedle, int32_t iHaystackLen)
{
    int32_t iFirst, iIndex, iSearchedLen;

    // make sure strings are valid
    if ((pHaystack != NULL) && (*pHaystack != 0) && (pNeedle != NULL) && (*pNeedle != 0))
    {
        iFirst = tolower((unsigned char)*pNeedle);
        for (iSearchedLen = 0; (*pHaystack != 0) && (iSearchedLen < iHaystackLen); pHaystack += 1, iSearchedLen += 1)
        {
            // small optimization on first character search
            if (tolower((unsigned char)*pHaystack) == iFirst)
            {
                for (iIndex = 1;; ++iIndex)
                {
                    if (pNeedle[iIndex] == 0)
                    {
                        return((char *)pHaystack);
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
    return(NULL);
}

/*F********************************************************************************/
/*!
    \Function _LogParseLineBufGetLine

    \Description
        Get line text from specified line reference

    \Input *pLineRef    - lineref to get text from
    \Input *pStrLineBuf - [out] storage for line text
    \Input iLineBufSize - size of output buffer

    \Output
        const char *    - output text

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static const char *_LogParseLineBufGetLine(LogParseLineRefT *pLineRef, char *pStrLineBuf, int32_t iLineBufSize)
{
    ds_strsubzcpy(pStrLineBuf, iLineBufSize, pLineRef->pSrcBuf+pLineRef->uLineStart, pLineRef->uLineLen);
    return(pStrLineBuf);
}

/*F********************************************************************************/
/*!
    \Function _LogParsePrint

    \Description
        Print specified logparse

    \Input *pLogParse   - logparse to print
    \Input *pOutFile    - output file to print to (may be stdout, stderr, etc)
    \Input uPrintFlags  - LOGPARSE_PRINTFLAG_*

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _LogParsePrint(LogParseRefT *pLogParse, FILE *pOutFile, uint32_t uPrintFlags)
{
    LogParseLineRefT *pLineBuf;
    const char *pLinePtr;
    char strTimeBuf[32], strLineBuf[256];
    uint32_t uLineCt;
    struct tm tm;
    uint32_t uElapsed = (uint32_t)(pLogParse->DateRange.uEndTime - pLogParse->DateRange.uStartTime);
    uint32_t uMillis = uElapsed%1000;
    uint32_t uSecs = uElapsed/1000;
    uint32_t uMins = uSecs/60;
    uint32_t uHours = uMins/60;
    uSecs %= 60;
    uMins %= 60;

    // print number of lines scanned
    if (uPrintFlags & LOGPARSE_PRINTFLAG_LOGHEADER)
    {
        fprintf(pOutFile, "logparse: parsed %u lines\n", pLogParse->uNumLineEntries);

        // print start end end times
        ds_secstotime(&tm, (uint32_t)(pLogParse->DateRange.uStartTime/1000));
        fprintf(pOutFile, "logparse: t0=%s.%03u\n", ds_timetostr(&tm, TIMETOSTRING_CONVERSION_ISO_8601, TRUE, strTimeBuf, sizeof(strTimeBuf)), (uint32_t)
            (pLogParse->DateRange.uStartTime%1000));
        ds_secstotime(&tm, (uint32_t)(pLogParse->DateRange.uEndTime/1000));
        fprintf(pOutFile, "logparse: t1=%s.%03u\n", ds_timetostr(&tm, TIMETOSTRING_CONVERSION_ISO_8601, TRUE, strTimeBuf, sizeof(strTimeBuf)), (uint32_t)
            (pLogParse->DateRange.uEndTime%1000));
        fprintf(pOutFile, "logparse: dt=%u:%02u:%02u:%03u\n", uHours, uMins, uSecs, uMillis);
    }

    // print scanned lines
    if (uPrintFlags & LOGPARSE_PRINTFLAG_LOGENTRIES)
    {
        for (uLineCt = 0, pLineBuf = pLogParse->pLineBuf; uLineCt < pLogParse->uNumLineEntries; uLineCt += 1, pLineBuf += 1)
        {
            pLinePtr = _LogParseLineBufGetLine(pLineBuf, strLineBuf, sizeof(strLineBuf));
            if (uPrintFlags & LOGPARSE_PRINTFLAG_LINENUM)
            {
                fprintf(pOutFile, "[%4u] ", uLineCt);
            }
            if (uPrintFlags & LOGPARSE_PRINTFLAG_SRCBUFPTR)
            {
                fprintf(pOutFile, "[%p] ", pLineBuf->pSrcBuf);
            }                
            fprintf(pOutFile, "%s\n", pLinePtr);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _LogParseLineBufAlloc

    \Description
        Allocate (or reallocate) a line buffer

    \Input *pCurLineBuf - pointer to current linebuffer (if realloc) or NULL (if alloc)
    \Input iNumCurLines - current number of lines in linebuffer
    \Input iNumNewLines - new size for linebuffer

    \Output
        LogParseLineRefT *  - new linebuffer, or NULL on failure

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static LogParseLineRefT *_LogParseLineBufAlloc(LogParseLineRefT *pCurLineBuf, int32_t iNumCurLines, int32_t iNumNewLines)
{
    int32_t iNewBufSize = iNumNewLines * sizeof(*pCurLineBuf);
    int32_t iOldBufSize = iNumCurLines * sizeof(*pCurLineBuf);
    LogParseLineRefT *pNewLineBuf;

    // alloc new buffer
    if ((pNewLineBuf = (LogParseLineRefT *)malloc(iNewBufSize)) == NULL)
    {
        NetPrintf(("logparse: could not alloc %d bytes for log buffer\n", iNumNewLines));
        return(NULL);
    }

    // copy old data to new buffer
    if (pCurLineBuf != NULL)
    {
        ds_memcpy(pNewLineBuf, pCurLineBuf, iOldBufSize);
        // free old buffer
        free(pCurLineBuf);
    }
    // clear newly allocated space
    ds_memclr(pNewLineBuf+iNumCurLines, iNewBufSize-iOldBufSize);

    // return new one
    return(pNewLineBuf);
}

/*F********************************************************************************/
/*!
    \Function _LogParseDatetime

    \Description
        Parse datetime string into a 64-bit datetime at millisecond precision

    \Input *pDateStr    - pointer to source string containing datetime

    \Output
        uint64_t        - output 64-bit datetime

    \Todo
        Optimize

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static uint64_t _LogParseDatetime(const char *pDateStr)
{
    int32_t iYear, iMonth, iDay, iHours, iMinutes, iSeconds;
    uint64_t uTimeInMillis;
    struct tm tm;
    // one-deep cache of most recent conversion for speed
    static const char *_pCurDateTime = "";
    static uint64_t _uCurDateTime;

    // see if datetime matches currently parsed datetime
    if (strncmp(pDateStr, _pCurDateTime, 20))
    {
        // parse date/time
        iYear = strtol(pDateStr, NULL, 10);
        iMonth = strtol(pDateStr+5, NULL, 10);
        iDay = strtol(pDateStr+8, NULL, 10);
        iHours = strtol(pDateStr+11, NULL, 10);
        iMinutes = strtol(pDateStr+14, NULL, 10);
        iSeconds = strtol(pDateStr+17, NULL, 10);

        // comvert to tm
        tm.tm_year = iYear - 1900;
        tm.tm_mon = iMonth - 1;
        tm.tm_mday = iDay;
        tm.tm_hour = iHours;
        tm.tm_min = iMinutes;
        tm.tm_sec = iSeconds;

        // convert to time in milliseconds
        uTimeInMillis = ds_timetosecs(&tm);

        // cache for future use
        _uCurDateTime = uTimeInMillis;
        _pCurDateTime = pDateStr;
    }
    else
    {
        uTimeInMillis = _uCurDateTime;
    }

    if (uTimeInMillis != 0)
    {
        uTimeInMillis *= 1000;
        uTimeInMillis += strtol(pDateStr+20, NULL, 10);
    }
    
    return(uTimeInMillis);
}

/*F********************************************************************************/
/*!
    \Function _LogParseBufScanLine

    \Description
        Scan current line and fill out a lineref for it.

    \Input *pLineRef    - [out] lineref to fill in
    \Input *pSrcBuf     - source buffer to extract from
    \Input uLineStart   - offset of line start within source buffer

    \Output
        uint32_t        - offset to start of next line

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static uint64_t _LogParseBufScanLine(LogParseLineRefT *pLineRef, const char *pSrcBuf, uint64_t uLineStart)
{
    const char *pLineStart = pSrcBuf+uLineStart, *pLinePtr;

    // parse time
    pLineRef->uLineTime = _LogParseDatetime(pLineStart);

    // find eol
    for (pLinePtr = pLineStart; (*pLinePtr != '\r') && (*pLinePtr != '\n') && (*pLinePtr != '\0'); pLinePtr += 1)
        ;

    // save line info
    pLineRef->pSrcBuf = pSrcBuf;
    pLineRef->uLineStart = uLineStart;
    pLineRef->uLineLen = pLinePtr - pLineStart;

    // skip past eol
    for (; (*pLinePtr == '\r') || (*pLinePtr == '\n'); pLinePtr += 1)
        ;
    return(pLinePtr - pLineStart);
}

/*F********************************************************************************/
/*!
    \Function _LogParseBufScan

    \Description
        Generate a linebuffer for the specified file buffer.

    \Input *pFileBuf        - file buffer to scan
    \Input uFileLen         - length of file buffer
    \Input *pLineEntries    = [out] number of line entries scanned

    \Output
        LogParseLineRefT *  - linebuffer, or NULL on failure

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static LogParseLineRefT *_LogParseBufScan(const char *pFileBuf, uint64_t uFileLen, uint32_t *pLineEntries)
{
    LogParseLineRefT *pLineBuf = NULL;
    uint32_t uNumLines, uLineBufLines, uLinesToAlloc;
    const char *pFilePtr;

    // parse buffer
    for (uLineBufLines = 0, uNumLines = 0, pFilePtr = pFileBuf; *pFilePtr != '\0'; uNumLines += 1)
    {
        // realloc line buffer?
        if (uLineBufLines == uNumLines)
        {
            // estimate size we need based on average line length, if available
            if (uLineBufLines > LOGPARSE_LINEBUF_ALLOCSIZE)
            {
                uint32_t uCharsPerLine = (pFilePtr-pFileBuf)/uLineBufLines + 1;
                uLinesToAlloc = uFileLen/uCharsPerLine;
                uLinesToAlloc = ((uLinesToAlloc/LOGPARSE_LINEBUF_ALLOCSIZE)+1) * LOGPARSE_LINEBUF_ALLOCSIZE;
            }
            else
            {
                uLinesToAlloc = LOGPARSE_LINEBUF_ALLOCSIZE;
            }
            if ((pLineBuf = _LogParseLineBufAlloc(pLineBuf, uLineBufLines, uLineBufLines+uLinesToAlloc)) == NULL) // tmp line buff alloc size
            {
                return(NULL);
            }
            uLineBufLines = uLineBufLines+uLinesToAlloc;
        }
        // parse current line
        pFilePtr += _LogParseBufScanLine(&pLineBuf[uNumLines], pFileBuf, pFilePtr-pFileBuf);
    }
    
    // return line buffer and size
    *pLineEntries = uNumLines;
    return(pLineBuf);
}

/*F********************************************************************************/
/*!
    \Function _LogParseLineExtractStristr

    \Description
        Extraction function; used to extract lines containing specified string
        (case insensitive).

    \Input *pLogParse       - logparse to extract from
    \Input *pLine           - line to check for extraction
    \Input *pExtractData    - string to match

    \Output
        int32_t             - 0 to extract, 1 to not extract

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LogParseLineExtractStristr(LogParseRefT *pLogParse, LogParseLineRefT *pLine, const void *pExtractData)
{
    const char *pCompare = (const char *)pExtractData;
    const char *pString = pLine->pSrcBuf+pLine->uLineStart;
    return((_ds_strisubstr(pString, pCompare, pLine->uLineLen) != NULL) ? 0 : 1);
}

/*F********************************************************************************/
/*!
    \Function _LogParseLineExtractStristrList

    \Description
        Extraction function; used to extract lines containing specified strings
        (case insensitive).

    \Input *pLogParse       - logparse to extract from
    \Input *pLine           - line to check for extraction
    \Input *pExtractData    - strings to match

    \Output
        int32_t             - 0 to extract, 1 to not extract

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LogParseLineExtractStristrList(LogParseRefT *pLogParse, LogParseLineRefT *pLine, const void *pExtractData)
{
    LogParseStrListT *pStrList = (LogParseStrListT *)pExtractData;
    const char *pString = pLine->pSrcBuf+pLine->uLineStart;
    uint32_t uString, uResult;
    for (uString = 0, uResult = 1; (uString < pStrList->uNumStrings) && (uResult != 0); uString += 1)
    {
        uResult = (_ds_strisubstr(pString, pStrList->pStrings[uString], pLine->uLineLen) != NULL) ? 0 : 1;
    }
    return(uResult);
}

/*F********************************************************************************/
/*!
    \Function _LogParseExtractDaterange

    \Description
        Extraction function; used to extract lines between specified datetimes.

    \Input *pLogParse       - logparse to extract from
    \Input *pLine           - line to check for extraction
    \Input *pExtractData    - start/end datetime

    \Output
        int32_t             - 0 to extract, 1 to not extract

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LogParseLineExtractDaterange(LogParseRefT *pLogParse, LogParseLineRefT *pLine, const void *pExtractData)
{
    const LogParseDateRangeT *pDateRange = (const LogParseDateRangeT *)pExtractData;
    return(((pLine->uLineTime >= pDateRange->uStartTime) && (pLine->uLineTime <= pDateRange->uEndTime)) ? 0 : 1);
}

/*F********************************************************************************/
/*!
    \Function _LogParseExtract

    \Description
        Extract lines from logparse as indicated by the specified extraction function.

    \Input *pLogParse       - logparse to extract from
    \Input *pExtractFunc    - extraction function, used to determine lines to extract
    \Input *pExtractData    - function-specific extraction data

    \Output
        LogParseRefT *      - pointer to result logparse, or NULL on failure

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static LogParseRefT *_LogParseExtract(LogParseRefT *pLogParse, LogParseLineExtractFuncT *pExtractFunc, const void *pExtractData)
{
    LogParseRefT *pNewLogParse;
    uint32_t uLine;

    // alloc new logparse buffer
    if ((pNewLogParse = (LogParseRefT *)malloc(sizeof(*pLogParse))) == NULL)
    {
        NetPrintf(("logparse: could not allocate buffer for extract\n"));
        return(NULL);
    }
    ds_memclr(pNewLogParse, sizeof(*pNewLogParse));

    // copy logparse
    for (uLine = 0; uLine < pLogParse->uNumLineEntries; uLine += 1)
    {
        // realloc line buffer?
        if (pNewLogParse->uNumLineEntries == pNewLogParse->uMaxLineEntries)
        {
            if ((pNewLogParse->pLineBuf = _LogParseLineBufAlloc(pNewLogParse->pLineBuf, pNewLogParse->uMaxLineEntries, pNewLogParse->uMaxLineEntries + LOGPARSE_LINEBUF_ALLOCSIZE)) == NULL) // tmp line buff alloc size
            {
                return(NULL);
            }
            pNewLogParse->uMaxLineEntries += LOGPARSE_LINEBUF_ALLOCSIZE;
        }
        // check current line
        if (!pExtractFunc(pLogParse, &pLogParse->pLineBuf[uLine], pExtractData))
        {
            // add to new buffer
            ds_memcpy(&pNewLogParse->pLineBuf[pNewLogParse->uNumLineEntries], &pLogParse->pLineBuf[uLine], sizeof(pNewLogParse->pLineBuf[0]));
            pNewLogParse->uNumLineEntries += 1;
        }
    }

    // get start/end times
    if (pNewLogParse->uNumLineEntries > 0)
    {
        pNewLogParse->DateRange.uStartTime = pNewLogParse->pLineBuf[0].uLineTime;
        pNewLogParse->DateRange.uEndTime = pNewLogParse->pLineBuf[pNewLogParse->uNumLineEntries-1].uLineTime;
    }

    // return new logparse
    return(pNewLogParse);
}

/*F********************************************************************************/
/*!
    \Function _LogParseCopy

    \Description
        Copy a logparse

    \Input *pCopy       - logparse to copy to
    \Input *pSource     - logparse to copy from

    \Output
        LogParseRefT *  - pointer to result logparse, or NULL on failure

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static LogParseRefT *_LogParseCopy(LogParseRefT *pLogParse)
{
    LogParseRefT *pCopy;

    // create logparse
    if ((pCopy = LogParseCreate(NULL, 0)) == NULL)
    {
        return(NULL);
    }
    // copy parse ref
    ds_memcpy(pCopy, pLogParse, sizeof(*pCopy));
    // allocate line buffer
    if ((pCopy->pLineBuf = _LogParseLineBufAlloc(NULL, 0, pLogParse->uNumLineEntries)) == NULL)
    {
        NetPrintf(("logparse: could not alloc line buffer in copy operation\n"));
        return(NULL);
    }
    // copy line buffer
    ds_memcpy(pCopy->pLineBuf, pLogParse->pLineBuf, pLogParse->uNumLineEntries*sizeof(*pCopy->pLineBuf));
    return(pCopy);
}

/*F********************************************************************************/
/*!
    \Function _LogParseAppend

    \Description
        Append non-overlapping logparse buffers (A+B).

    \Input *pLogAppendA     - logparse to append to
    \Input *pLogAppendB     - logparse to append from

    \Output
        LogParseRefT *      - pointer to result logparse, or NULL on failure

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static LogParseRefT *_LogParseAppend(LogParseRefT *pLogAppendA, LogParseRefT *pLogAppendB)
{
    LogParseRefT *pLogAppendRslt;
    uint32_t uNumEntries;

    // create append buffer
    if ((pLogAppendRslt = LogParseCreate(NULL, 0)) == NULL)
    {
        return(NULL);
    }
    // create append line buffer
    uNumEntries = pLogAppendA->uNumLineEntries+pLogAppendB->uNumLineEntries;
    if ((pLogAppendRslt->pLineBuf = _LogParseLineBufAlloc(pLogAppendRslt->pLineBuf, pLogAppendRslt->uMaxLineEntries, uNumEntries)) == NULL)
    {
        NetPrintf(("logparse: could not alloc line buffer in append operation\n"));
        return(NULL);
    }
    // copy info
    if (uNumEntries > 0)
    {
        // copy appenda buffer
        ds_memcpy(pLogAppendRslt->pLineBuf, pLogAppendA->pLineBuf, pLogAppendA->uNumLineEntries*sizeof(*pLogAppendRslt->pLineBuf));
        // append appendb buffer
        ds_memcpy(pLogAppendRslt->pLineBuf+pLogAppendA->uNumLineEntries, pLogAppendB->pLineBuf, pLogAppendB->uNumLineEntries*sizeof(*pLogAppendRslt->pLineBuf));
        // recalc logparse info
        pLogAppendRslt->uNumLineEntries = pLogAppendRslt->uMaxLineEntries = uNumEntries;
        pLogAppendRslt->DateRange.uStartTime = pLogAppendRslt->pLineBuf[0].uLineTime;
        pLogAppendRslt->DateRange.uEndTime = pLogAppendRslt->pLineBuf[pLogAppendRslt->uNumLineEntries-1].uLineTime;
    }
    return(pLogAppendRslt);
}

/*F********************************************************************************/
/*!
    \Function _LogParseMergeInterleaved

    \Description
        Merge two overlapping logparse buffers into one, using an insertion sort.

    \Input *pLogMergeA      - first logparse to merge
    \Input *pLogMergeB      - second logparse to merge

    \Output
        LogParseRefT *      - pointer to result logparse, or NULL on failure

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static LogParseRefT *_LogParseMergeInterleaved(LogParseRefT *pLogMergeA, LogParseRefT *pLogMergeB)
{
    uint32_t uNumEntries, uMergeLine, uLine1, uLine2;
    uint64_t uLineTime1, uLineTime2;
    LogParseRefT *pLogMergeRslt;

    // create append buffer
    if ((pLogMergeRslt = LogParseCreate(NULL, 0)) == NULL)
    {
        return(NULL);
    }
    // create append line buffer
    uNumEntries = pLogMergeA->uNumLineEntries+pLogMergeB->uNumLineEntries;
    if ((pLogMergeRslt->pLineBuf = _LogParseLineBufAlloc(pLogMergeRslt->pLineBuf, pLogMergeRslt->uMaxLineEntries, uNumEntries)) == NULL)
    {
        NetPrintf(("logparse: could not alloc line buffer in merge operation\n"));
        return(NULL);
    }
    // perform insertion sort
    if (uNumEntries > 0)
    {
        for (uMergeLine = 0, uLine1 = 0, uLine2 = 0; uMergeLine < uNumEntries; uMergeLine += 1)
        {
            uLineTime1 = (uLine1 < pLogMergeA->uNumLineEntries) ? pLogMergeA->pLineBuf[uLine1].uLineTime : (uint64_t)-1;
            uLineTime2 = (uLine2 < pLogMergeB->uNumLineEntries) ? pLogMergeB->pLineBuf[uLine2].uLineTime : (uint64_t)-1;

            if (uLineTime1 <= uLineTime2)
            {
                ds_memcpy(&pLogMergeRslt->pLineBuf[uMergeLine], &pLogMergeA->pLineBuf[uLine1++], sizeof(pLogMergeRslt->pLineBuf[0]));
            }
            else
            {
                ds_memcpy(&pLogMergeRslt->pLineBuf[uMergeLine], &pLogMergeB->pLineBuf[uLine2++], sizeof(pLogMergeRslt->pLineBuf[0]));
            }
        }
        // set number of line entries
        pLogMergeRslt->uNumLineEntries = pLogMergeRslt->uMaxLineEntries = uNumEntries;
        // get start/end times
        pLogMergeRslt->DateRange.uStartTime = pLogMergeRslt->pLineBuf[0].uLineTime;
        pLogMergeRslt->DateRange.uEndTime = pLogMergeRslt->pLineBuf[pLogMergeRslt->uNumLineEntries-1].uLineTime;
    }
    return(pLogMergeRslt);
}

/*F********************************************************************************/
/*!
    \Function _LogParseMerge

    \Description
        Merge two logparse buffers into one.

    \Input *pLogMerge1      - first logparse to merge
    \Input *pLogMerge2      - second logparse to merge

    \Output
        LogParseRefT *      - pointer to result logparse, or NULL on failure

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static LogParseRefT *_LogParseMerge(LogParseRefT *pLogMerge1, LogParseRefT *pLogMerge2)
{
    if (pLogMerge1->DateRange.uEndTime <= pLogMerge2->DateRange.uStartTime)
    {
        return(_LogParseAppend(pLogMerge1, pLogMerge2));
    }
    else if (pLogMerge2->DateRange.uEndTime <= pLogMerge1->DateRange.uStartTime)
    {
        return(_LogParseAppend(pLogMerge2, pLogMerge1));
    }
    else // do the merge
    {
        return(_LogParseMergeInterleaved(pLogMerge1, pLogMerge2));
    }
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function LogParseCreate

    \Description
        Create log parser, and scan file buffer if not NULL.

    \Input *pFileBuf    - file buffer to parser, or NULL
    \Input uFileLen     - size of file buffer in memory

    \Output
        LogParseRefT *  - log parser, or NULL on failure

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
LogParseRefT *LogParseCreate(const char *pFileBuf, uint64_t uFileLen)
{
    LogParseRefT *pLogParse;
#if DIRTYCODE_LOGGING
    uint32_t uScanTimer;
#endif

    // allocate log parse ref
    if ((pLogParse = (LogParseRefT *)malloc(sizeof(*pLogParse))) == NULL)
    {
        NetPrintf(("logparse: could not allocate memory for logparser state\n"));
        return(NULL);
    }
    ds_memclr(pLogParse, sizeof(*pLogParse));
    
    // check for no data
    if ((pFileBuf != NULL) && (*pFileBuf != '\0'))
    {
#if DIRTYCODE_LOGGING
        // start scan timer
        uScanTimer = NetTick();
#endif
    
        // scan the buffer
        if ((pLogParse->pLineBuf = _LogParseBufScan(pFileBuf, uFileLen, &pLogParse->uNumLineEntries)) != NULL)
        {
            NetPrintf(("logparse: scanned %d entries in %dms\n", pLogParse->uNumLineEntries, NetTickDiff(NetTick(), uScanTimer)));

            // get start/end times
            pLogParse->DateRange.uStartTime = pLogParse->pLineBuf[0].uLineTime;
            pLogParse->DateRange.uEndTime = pLogParse->pLineBuf[pLogParse->uNumLineEntries-1].uLineTime;
        }
    }

    // return to caller
    return(pLogParse);
}

/*F********************************************************************************/
/*!
    \Function LogParseDestroy

    \Description
        Destroy specified log parser, free allocated memory

    \Input *pFileBuf    - log parser to destroy

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
void LogParseDestroy(LogParseRefT *pLogParse)
{
    if (pLogParse->pLineBuf != NULL)
    {
        free(pLogParse->pLineBuf);
    }
    free(pLogParse);
}

/*F********************************************************************************/
/*!
    \Function LogParsePrint

    \Description
        Print logparse entries to stdout

    \Input *pFileBuf    - log parser to print
    \Input *pOutFile    - output file (may be stdin, stderr, etc)
    \Input uPrintFlags  - LOGPARSE_PRINTFLAG_*

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
void LogParsePrint(LogParseRefT *pLogParse, FILE *pOutFile, uint32_t uPrintFlags)
{
    _LogParsePrint(pLogParse, pOutFile, uPrintFlags);
}

/*F********************************************************************************/
/*!
    \Function LogParseCopy

    \Description
        Copy a logparse

    \Input *pLogParse       - logparse to copy

    \Output
        LogParseRefT *      - pointer to result logparse, or NULL on failure

    \Version 10/25/2012 (jbrookes)
*/
/********************************************************************************F*/
LogParseRefT *LogParseCopy(LogParseRefT *pLogParse)
{
    return(_LogParseCopy(pLogParse));
}

/*F********************************************************************************/
/*!
    \Function LogParseMerge

    \Description
        Merge two logparse buffers into one.

    \Input *pLogMerge1      - first logparse to merge
    \Input *pLogMerge2      - second logparse to merge

    \Output
        LogParseRefT *      - pointer to result logparse, or NULL on failure

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
LogParseRefT *LogParseMerge(LogParseRefT *pLogMerge1, LogParseRefT *pLogMerge2)
{
    return(_LogParseMerge(pLogMerge1, pLogMerge2));
}

/*F********************************************************************************/
/*!
    \Function LogParseExtractStr

    \Description
        Extract logparse entries that contain the specified string (like grep).
        Extraction is case-insensitve.

    \Input *pLogParse   - logparse to extract from
    \Input *pExtractStr - string lines must contain to be extracted

    \Output
        LogParseRefT *  - pointer to result logparse, or NULL on failure

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
LogParseRefT *LogParseExtractStr(LogParseRefT *pLogParse, const char *pExtractStr)
{
    return(_LogParseExtract(pLogParse, _LogParseLineExtractStristr, (const void *)pExtractStr));
}

/*F********************************************************************************/
/*!
    \Function LogParseExtractStrList

    \Description
        Extract logparse entries that contain the specified string list (like grep).
        Extraction is case-insensitve.

    \Input *pLogParse   - logparse to extract from
    \Input *pStrList    - list of strings that must be present to be extracted

    \Output
        LogParseRefT *  - pointer to result logparse, or NULL on failure

    \Version 07/10/2020 (jbrookes)
*/
/********************************************************************************F*/
LogParseRefT *LogParseExtractStrList(LogParseRefT *pLogParse, const LogParseStrListT *pStrList)
{
    // if only one string, use faster version
    if (pStrList->uNumStrings < 2)
    {
        return(_LogParseExtract(pLogParse, _LogParseLineExtractStristr, (const void *)pStrList->pStrings[0]));
    }
    else
    {
        return(_LogParseExtract(pLogParse, _LogParseLineExtractStristrList, (const void *)pStrList));
    }
}

/*F********************************************************************************/
/*!
    \Function LogParseExtractDateRange

    \Description
        Extract logparse entries that fall between the specified start and end time.

    \Input *pLogParse       - logparse to extract from
    \Input *pStartdatetime  - starting datetime
    \Input *pEndDatetime    - ending datetime

    \Output
        LogParseRefT *  - pointer to result logparse, or NULL on failure

    \Notes
        Datetime format YYYY/MM/DD HH:MM:SS:mmm

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
LogParseRefT *LogParseExtractDateRange(LogParseRefT *pLogParse, const char *pStartDatetime, const char *pEndDatetime)
{
    LogParseDateRangeT DateRange;
    DateRange.uStartTime = _LogParseDatetime(pStartDatetime);
    DateRange.uEndTime = _LogParseDatetime(pEndDatetime);
    return(_LogParseExtract(pLogParse, _LogParseLineExtractDaterange, (const void *)&DateRange));
}

/*F********************************************************************************/
/*!
    \Function LogParseGetLine

    \Description
        Get line text from specified line reference

    \Input *pLineRef    - lineref to get text from
    \Input *pStrLineBuf - [out] storage for line text
    \Input iLineBufSize - size of output buffer

    \Output
        const char *    - output text

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
const char *LogParseGetLine(LogParseLineRefT *pLineRef, char *pStrLineBuf, int32_t iLineBufSize)
{
    return(_LogParseLineBufGetLine(pLineRef, pStrLineBuf, iLineBufSize));
}

/*F********************************************************************************/
/*!
    \Function LogParseDatetime

    \Description
        Parse datetime string into a 64-bit datetime at millisecond precision

    \Input *pDateStr    - pointer to source string containing datetime

    \Output
        uint64_t        - output 64-bit datetime

    \Version 07/22/2020 (jbrookes)
*/
/********************************************************************************F*/
uint64_t LogParseDatetime(const char *pDateStr)
{
    return(_LogParseDatetime(pDateStr));
}


