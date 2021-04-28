/*H********************************************************************************/
/*!
    \File logparse.h

    \Description
        Basic server-inspecific log parsing for servers using 'eafn' logger

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 10/23/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

#ifndef _logparse_h
#define _logparse_h

/*** Include files ****************************************************************/

/*** Defines **********************************************************************/

// printing option flags for LogParsePrint()
#define LOGPARSE_PRINTFLAG_LOGHEADER    (1)     //!< print log header
#define LOGPARSE_PRINTFLAG_LOGENTRIES   (2)     //!< print log entries
#define LOGPARSE_PRINTFLAG_LINENUM      (4)     //!< print log entries with line numbers (implies LOGENTRIES)
#define LOGPARSE_PRINTFLAG_SRCBUFPTR    (8)     //!< print log lines referencing source buffer pointers (implies LOGENTRIES)
#define LOGPARSE_PRINTFLAG_ALL          (15)    //!< all print options

/*** Macros ***********************************************************************/

/*** Type Definitions *************************************************************/

typedef struct LogParseDateRangeT
{
    uint64_t uStartTime;
    uint64_t uEndTime;
} LogParseDateRangeT;

typedef struct LogParseStrListT
{
    uint32_t uNumStrings;
    const char *pStrings[16];       //! string list
} LogParseStrListT;

typedef struct LogParseLineRefT
{
    uint64_t uLineTime;             //!< timestamp of line in milliseconds
    const char *pSrcBuf;            //!< pointer to source buffer containing line
    uint64_t uLineStart;            //!< offset of start of line within buffer
    uint16_t uLineLen;              //!< length of line
    uint8_t  pad[2];
} LogParseLineRefT;

typedef struct LogParseRefT
{
    LogParseLineRefT *pLineBuf;     //!< pointer to line buffer array
    uint32_t uNumLineEntries;       //!< number of line entries
    uint32_t uMaxLineEntries;       //!< max number of line buffer entries in line buffer array
    LogParseDateRangeT DateRange;   //!< beginning and end timestamps for parsed file buffer
}LogParseRefT;

/*** Variables ********************************************************************/

/*** Functions ********************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// create log parser module for specified file buffer
LogParseRefT *LogParseCreate(const char *pFileBuf, uint64_t uFileLen);

// destroy log parser module
void LogParseDestroy(LogParseRefT *pLogParse);

// print a parsed log
void LogParsePrint(LogParseRefT *pLogParse, FILE *pOutFile, uint32_t uPrintFlags);

// copy logparse
LogParseRefT *LogParseCopy(LogParseRefT *pLogParse);

// merge two logs into one
LogParseRefT *LogParseMerge(LogParseRefT *pLogMerge1, LogParseRefT *pLogMerge2);

// extract lines including string from log
LogParseRefT *LogParseExtractStr(LogParseRefT *pLogParse, const char *pExtractStr);

// extract lines including one or more of a list of strings from log
LogParseRefT *LogParseExtractStrList(LogParseRefT *pLogParse, const LogParseStrListT *pStrList);

// extract lines between start and end date/time references in form "YYYY/MM/DD HH:MM:SS:000"
LogParseRefT *LogParseExtractDateRange(LogParseRefT *pLogParse, const char *pStartDatetime, const char *pEndDatetime);

// get a line from line buffer
const char *LogParseGetLine(LogParseLineRefT *pLineRef, char *pStrLineBuf, int32_t iLineBufSize);

// get datetime from string
uint64_t LogParseDatetime(const char *pDateStr);

#ifdef __cplusplus
}
#endif

#endif // _logparse_h
