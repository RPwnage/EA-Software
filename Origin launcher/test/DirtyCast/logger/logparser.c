/*H********************************************************************************/
/*!
    \File logparser.c

    \Description
        Log parser application for servers using 'eafn' logger

    \Copyright
        Copyright (c) 2012 Electronic Arts Inc.

    \Version 1.0 10/23/2012 (jbrookes) First Version
*/
/********************************************************************************H*/

/*
    hypothetical LogParser command syntax

    load [logparse] [filename1] <filename2> ... <filenameN> ; load one or more files into [logparse].  if more than one file, files are merged.  if [logparse] exists, files are merged into it, otherwise it is created
    save [logparse] [filename] ; save [logparse] to [filename]
    list ; list all logparses
    merge [logparse] [logparseM1] [logparseM2] ; merge [logparseM1] and [logparseM2] into [logparse]
    xstrn [logparse] [logparseSrc] [string] ; extract lines containing [string] from [logpraseSrc], into [logparse]
    xdate [logparse] [logparseSrc] [datefrom] [dateto] ; extract lines from [datefrom] to [dateto]
    xevnt [logparse] [logparseSrc] [delta] <minevents>; extract lines that fall within [delta] milliseconds of one another; if <minevents> is specified, events smaller are excluded

    example:

    load LogParse *.log*
    xstrn LogParseStalls LogParse stall
    xevnt LogParseEvents LogParseStalls 200 4
    save LogParseEvents vsevents.log
*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <search.h> // qsort

#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/dirtynet.h"
#include "DirtySDK/dirtysock/dirtymem.h"

#if defined(LOGPARSE_GEOIP)
#include <GeoIP.h>
#include <GeoIPCity.h>
#endif

#include "libsample/zfile.h"
#include "libsample/zlib.h"
#include "libsample/zmem.h"

#include "logparse.h"

/*** Defines **********************************************************************/

enum
{
    LOGPARSE_CNTRLAT = 0,
    LOGPARSE_CNTRLON,
    LOGPARSE_MAXDIST,
    LOGPARSE_AVGDIST,
    LOGPARSE_STNDDEV,
    LOGPARSE_NUMSTAT
};

#define MAX_ADDRS   (4096)
#define MAX_ASNS    (4096)
#define MAX_ISPS    (4096)
#define MAX_STALLS  (4096)
#define MAX_CLIENTS (16*1024)

/*** Type Definitions *************************************************************/

//! thompson tau test table entry
typedef struct ThompsonTauTableEntryT
{
    uint32_t uNumSamples;
    float    fValue;
} ThompsonTauTableEntryT;

//! asn record
// $$todo - wanted to use this for sorting but doesn't work for ordered list insertion of generic uint32_t array
typedef struct AsnRecordT
{
    uint32_t uAsn;
    uint32_t uAsnAddr;
    uint32_t uAsnCount;
    uint32_t uAsnDiscs;
} AsnRecordT;

typedef struct IspRecordT
{
    uint16_t uIspCount;
    uint16_t uIspDiscs;
    char strOrg[64];
    char strCountry[64];
} IspRecordT;

//! client info
typedef struct LogParserClientInfoT
{
    uint64_t uAddTime;
    uint32_t uIdent;
    uint32_t uAddr;
    uint8_t  bDisc;
    uint8_t _pad[3];
} LogParserClientInfoT;

//! stall record
typedef struct LogParserStallRecordT
{
    uint64_t uTime;             //!< log line timestamp
    uint32_t uClientId;         //!< clientid
    uint32_t uDuration;         //!< event duration
    uint32_t uAddr;
    uint32_t uAsn;              //!< autonomous system number
    float    fLatitude;
    float    fLongitude;
    float    fDist;
    char     strCountry[64];
    char     strRegion[32];
    char     strCity[64];
    char     strOrg[64];
    char     strAsnOrg[64];        
    uint8_t  bValid;
    uint8_t _pad[3];
} LogParserStallRecordT;

//! geo info (asn/isp)
typedef struct LogParserGeoInfoT
{
    uint32_t uNumAsns;
    uint32_t aAsnList[MAX_ASNS];
    uint32_t aAsnAddrs[MAX_ASNS];
    uint16_t aAsnCounts[MAX_ASNS];
    uint16_t aAsnDiscs[MAX_ASNS];
    uint32_t uNumIsps;
    IspRecordT aIspRecords[MAX_ISPS];
} LogParserGeoInfoT;


//! main application state
typedef struct LogParserAppT
{
    #if defined(LOGPARSE_GEOIP)
    GeoIP *pGeoIP;              //!< geoip main db
    GeoIP *pGeoIPOrg;           //!< org-specific db if available
    const char *pGeoIPAsn;      //!< asn-specific db if available
    LogParseRefT *pGeoIPAsnParsed;
    #endif

    uint32_t uStallThreshold;   //!< minimum threshold of events to be considered a stall
    uint32_t uStallWindow;      //!< window of time to aggregate into stall event in ms
    int32_t iClientListFull;    //!< number of clients overflowing client list array

    LogParseStrListT ExtractList; //!< list of strings to extract from logfile

    LogParserStallRecordT StallRecords[MAX_STALLS];
    LogParserClientInfoT ClientList[MAX_CLIENTS];
    LogParserGeoInfoT GeoInfo;

    char strInpFile[256];
    char strOutFile[256];
    char strAsnFile[256];
    char strGeoFile[256];
    char strOrgFile[256];

    char strStartTime[32];      //!< start time, if constraining to a window of time
    char strEndTime[32];        //!< end time, if constraining to a window of time

    char strIspFilter[32];      //!< isp filter string (only select entries that match this isp)
    char strAsnFilter[32];      //!< asn filter string (only select entries that match this isp)
    char strOutFilter[32];      //!< output filter (see output function for details)

    int16_t iEventPopulationIdx; //!< index of event to enable population report for
    uint8_t bDisconnects;       //!< process disconnect events (exclusive with stalls)
    uint8_t bStalls;            //!< process stall events (exclusive with disconnections)
    uint8_t bValidate;          //!< enable log line validation (ensures log entries start with a valid date)
    uint8_t bOutliers;          //!< calculate and display outliers if enabled
    uint8_t bAnonymize;         //!< anonymize user IP addresses by replacing the last digit
    uint8_t bAddrSort;          //!< sort addresses when printing events
    uint8_t bCsvOutput;         //!< output stall event entries as csv
    uint8_t bAsnExclude;        //!< true if excluding by ASN, false if selecting by ASN
    uint8_t bIspExclude;        //!< true if excluding by ISP, false if selecting by ISP
    uint8_t bAsnReport;         //!< true if ASN report is enabled, else false
    uint8_t uMinAsnReport;      //!< min number of ASNs to be visible in report
    uint8_t bIspReport;         //!< true if ISP report is enabled, else false
    uint8_t uMinIspReport;      //!< min number of ISPs to be visible in report
    uint8_t bHeadersOnly;       //!< output event headers only
    uint8_t bDynamicOutput;     //<! true if dynamic output else false
    uint8_t _pad;
} LogParserAppT;

/*** Variables ********************************************************************/

/*** Private Functions ************************************************************/

//$$todo - should be in plat-str
static char *ds_secstostrms(uint64_t elap, TimeToStringConversionTypeE eConvType, uint8_t bLocalTime, uint8_t bPrintMillis, char *pOutBuf, int32_t iOutSize)
{
    struct tm tm;
    int32_t iOffset;
    ds_secstotime(&tm, elap/1000);
    iOffset = ds_snzprintf(pOutBuf, iOutSize, "%s", ds_timetostr(&tm, eConvType, bLocalTime, pOutBuf, iOutSize));
    if (bPrintMillis)
    {
        ds_snzprintf(pOutBuf+iOffset, iOutSize-iOffset, ".%03d", elap%1000);
    }
    return(pOutBuf);
}

#if defined(DIRTYCODE_PC) 

#include <windows.h>
#include <tchar.h>
#include <stdio.h>

#define INVALID_HANDLE_VALUE ((HANDLE)(LONG_PTR)-1)
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010

//$$todo - should be in zfile
ZFileT ZFileGetDirFile(char *pFileName, int32_t iFileNameLen, ZFileT iFindId, uint8_t *bDirectory, const TCHAR *pDirectory)
{
    WIN32_FIND_DATA FindFileData;
    HANDLE hFind;

    if (iFindId == ZFILE_INVALID)
    {
        if ((hFind = FindFirstFileA(pDirectory, &FindFileData)) != INVALID_HANDLE_VALUE)
        {
            ds_strnzcpy(pFileName, FindFileData.cFileName, iFileNameLen);
            *bDirectory = FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? TRUE : FALSE;
            iFindId = (ZFileT)hFind;
        }
    }
    else
    {
        if (FindNextFileA((HANDLE)iFindId, &FindFileData))
        {
            ds_strnzcpy(pFileName, FindFileData.cFileName, iFileNameLen);
            *bDirectory = FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ? TRUE : FALSE;
        }
        else
        {
            //DWORD dwError = GetLastError();
            FindClose((HANDLE)iFindId);
            iFindId = ZFILE_INVALID;
        }
    }
    return(iFindId);
}
#endif

#if defined(DIRTYCODE_PC)
/*F********************************************************************************/
/*!
    \Function _LogParserPrintfHook

    \Description
        Debug printf hook for Windows

    \Input *pParm       - unused
    \Input *pText       - text to print

    \Output
        int32_t         - 1

    \Version ??/??/20?? (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LogParserPrintfHook(void *pParm, const char *pText)
{
    printf("%s", pText);
    return(1);
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserGetDuration

    \Description
        Get duration string, formatting time in terms of seconds and/or minutes

    \Input *pBuffer     - [out] storage for formatted string
    \Input iBufSize     - string buffer size
    \Input uTicks       - millisecond tick counter
    
    \Output
        const char *    - formatted string

    \Version 12/04/2019 (jbrookes)
*/
/********************************************************************************F*/
static const char *_LogParserGetDuration(char *pBuffer, int32_t iBufSize, uint32_t uTicks)
{
    // convert to seconds
    uTicks /= 1000;

    // format based on duration
    if (uTicks < 60)
    {
        ds_snzprintf(pBuffer, iBufSize, "%d seconds", uTicks);
    }
    else if (uTicks < 120)
    {
        ds_snzprintf(pBuffer, iBufSize, "1 minute and %d seconds", uTicks%60);
    }
    else
    {
        ds_snzprintf(pBuffer, iBufSize, "%d minutes and %d seconds", uTicks/60, uTicks%60);
    }
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function _LogParseValidate

    \Description
        Perform (very rudimentary) validation of input lines to make sure they
        are valid log lines.  Only validates when enabled for perf reasons.

    \Input *pApp        - parser app state
    \Input *pLogParse   - logparse to validate lines of

    \TODO
        Better validation that isn't just validating the log lines start with a
        valid date

    \Version 08/03/2020 (jbrookes)
*/
/********************************************************************************F*/
static void _LogParseValidate(LogParserAppT *pApp, LogParseRefT *pLogParse)
{
    uint32_t uLine;
    if (!pApp->bValidate)
    {
        return;
    }
    for (uLine = 0; uLine < pLogParse->uNumLineEntries; uLine += 1)
    {
        if (pLogParse->pLineBuf[uLine].uLineTime == 0)
        {
            char strLine[128];
            LogParseGetLine(pLogParse->pLineBuf+uLine, strLine, sizeof(strLine));
            ZPrintf("--- bad line: \'%s\' --\n", strLine);
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _LogParserExtractDateRange

    \Description
        Extract log entries within the specified date range

    \Input *pApp        - parser app state
    \Input *pLogParse   - logparse to extract lines from

    \Output
        LogParseRefT *  - new logparse with extracted entries

    \Notes
        Date range has an hour subtracted on the front end; this is for cases
        where we are parsing client adds/removals, to give us some leeway to
        make sure we catch adds before the beginning of our date range where
        possible.

    \Version 07/30/2020 (jbrookes)
*/
/********************************************************************************F*/
static LogParseRefT *_LogParserExtractDateRange(LogParserAppT *pApp, LogParseRefT *pLogParse)
{
    LogParseRefT *pLogParse2;
    char strStartTime[32];

    ds_strnzcpy(strStartTime, pApp->strStartTime, sizeof(strStartTime));

    // if we're looking for disconnect events, move the start time back by an hour for extraction so we pick up clients that are added to the server before the start of the time range
    if (pApp->bDisconnects)
    {
        uint64_t uDateTime = LogParseDatetime(pApp->strStartTime);
        uDateTime -= 60*60*1000;
        ds_secstostrms(uDateTime, TIMETOSTRING_CONVERSION_ISO_8601, TRUE, FALSE, strStartTime, sizeof(strStartTime));
    }

    ZPrintf("logparser: extracting lines between %s and %s \n", strStartTime, pApp->strEndTime);
    pLogParse2 = LogParseExtractDateRange(pLogParse, strStartTime, pApp->strEndTime);
    if (pLogParse2 != NULL)
    {
        LogParseDestroy(pLogParse);
        pLogParse = pLogParse2;
    }

    return(pLogParse);
}

/*F********************************************************************************/
/*!
    \Function _LogParserLoadFile

    \Description
        Load specified file into memory and create a logparse for it.

    \Input *pApp        - parser app state
    \Input *pFilename   - filename to load
    \Input *pStrList    - list of strings to extract from file

    \Output
        LogParseRefT *  - logparse for file, or NULL on error

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static LogParseRefT *_LogParserLoadFile(LogParserAppT *pApp, const char *pFilename, LogParseStrListT *pStrList)
{
    LogParseRefT *pLogParse, *pLogParse2;
    const char *pFileBuf;
    int32_t iFileSize;

    // load logfile into memory
    if ((pFileBuf = ZFileLoad(pFilename, &iFileSize, FALSE)) == NULL)
    {
        ZPrintf("logparser: could not load file '%s'\n", pFilename);
        return(NULL);
    }

    // create logger
    ZPrintf(pApp->bDynamicOutput ? "\rlogparser: parsing file %s" : "logparser: parsing file %s\n", pFilename);
    if ((pLogParse = LogParseCreate(pFileBuf, iFileSize)) == NULL)
    {
        ZPrintf("logparser: could not create logparser\n");
        ZMemFree((void *)pFileBuf);
        return(NULL);
    }
    _LogParseValidate(pApp, pLogParse);

    // extract lines in date range, if specified
    if ((pApp->strStartTime[0] != '\0') && (pApp->strEndTime[0] != '\0'))
    {
        pLogParse = _LogParserExtractDateRange(pApp, pLogParse);
    }

    // extract lines in string list, if specified
    if ((pStrList->uNumStrings != 0) && ((pLogParse2 = LogParseExtractStrList(pLogParse, pStrList)) != NULL))
    {
        LogParseDestroy(pLogParse);
        pLogParse = pLogParse2;
    }
    _LogParseValidate(pApp, pLogParse);

    return(pLogParse);
}

/*F********************************************************************************/
/*!
    \Function _LogParserLoadWildcardFile

    \Description
        Check for a wildcard, and if there is one, expand to list of files and
        load them all.  If there is no wildcard, do nothing.

    \Input *pApp        - parser app state
    \Input *pFilename   - filename to load
    \Input *pStrList    - list of strings to extract from file

    \Output
        LogParseRefT *  - logparse for file, or NULL on error

    \Version 08/26/2016 (jbrookes)
*/
/********************************************************************************F*/
static LogParseRefT *_LogParserLoadWildcardFile(LogParserAppT *pApp, const char *pFilename, LogParseStrListT *pStrList)
{
    #if defined(DIRTYCODE_PC) 
    LogParseRefT *pLogParse, *pLogParse2, *pLogParseM;
    char strFileName[256];
    uint8_t bDirectory;
    ZFileT iFindId;
    int32_t iFile;

    // create empty log parser
    pLogParse = LogParseCreate(NULL, 0);

    // load and parse files
    for (iFile = 0, iFindId = ZFILE_INVALID; ; iFile += 1)
    {
        iFindId = ZFileGetDirFile(strFileName, sizeof(strFileName), iFindId, &bDirectory, pFilename);
        if (iFindId == ZFILE_INVALID)
        {
            break;
        }
        if (!bDirectory)
        {
            if ((pLogParse2 = _LogParserLoadFile(pApp, strFileName, pStrList)) != NULL)
            {
                if ((pLogParseM = LogParseMerge(pLogParse, pLogParse2)) != NULL)
                {
                    _LogParseValidate(pApp, pLogParseM);
                    LogParseDestroy(pLogParse);
                    pLogParse = pLogParseM;
                }
                LogParseDestroy(pLogParse2);
            }
        }
    }
    ZPrintf("\nlogparser: processed %d files\n", iFile);

    return(pLogParse);
    #else
    return(NULL);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _LogParserSaveFile

    \Description
        Save specified logpare to a file

    \Input *pLogParse   - logparse to save
    \Input *pFileName   - filename to save to

    \Version 10/25/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _LogParserSaveFile(LogParseRefT *pLogParse, const char *pFileName)
{
    #if defined(DIRTYCODE_PC) || defined(DIRTYCODE_LINUX)
    ZFileT iOutFile;

    // open output file
    if ((iOutFile = ZFileOpen(pFileName, ZFILE_OPENFLAG_WRONLY|ZFILE_OPENFLAG_CREATE)) == ZFILE_INVALID)
    {
        ZPrintf("logparser: could not open file '%s' for writing\n", pFileName);
        return;
    }
 
    // $$tmp - workaround inability to get file pointer
    LogParsePrint(pLogParse, (FILE *)iOutFile, LOGPARSE_PRINTFLAG_LOGENTRIES);
    ZFileClose(iOutFile);
    ZPrintf("logparser: saved logparse to file '%s'\n", pFileName);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _LogParserLoadFilesFromCmdline

    \Description
        Load and merge logfiles specified on commandline

    \Input *pApp        - parser app state
    \Input iStartIndex  - start of logfiles on commandline
    \Input iArgc        - number of commandline arguments
    \Input *pArgv[]     - commandline arguments
    \Input *pStrList    - list of strings to extract from file

    \Output
        LogParseRefT *  - log parser containing merged log entries from input files

    \Version 10/25/2012 (jbrookes)
*/
/********************************************************************************F*/
static LogParseRefT *_LogParserLoadFilesFromCmdline(LogParserAppT *pApp, int32_t iStartIndex, int32_t iArgc, const char *pArgv[], LogParseStrListT *pStrList)
{
    LogParseRefT *pLogParse, *pLogParse2, *pLogParseM;
    int32_t iArg; 

    // create empty log parser
    pLogParse = LogParseCreate(NULL, 0);

    for (iArg = iStartIndex; iArg < iArgc; iArg += 1)
    {
        // load file (or load multiple)
        pLogParse2 = strchr(pArgv[iArg], '*') ? _LogParserLoadWildcardFile(pApp, pArgv[iArg], pStrList) : _LogParserLoadFile(pApp, pArgv[iArg], pStrList);

        // if we got a parse buffer...
        if (pLogParse2 != NULL)
        {
            if ((pLogParseM = LogParseMerge(pLogParse, pLogParse2)) != NULL)
            {
                LogParseDestroy(pLogParse);
                pLogParse = pLogParseM;
            }
            LogParseDestroy(pLogParse2);
        }
    }
    return(pLogParse);
}

/*F********************************************************************************/
/*!
    \Function _LogParserLoadFilesFromInputfile

    \Description
        Load and merge logfiles listed in specified input file

    \Input *pApp        - parser app state
    \Input *pStrInpFile - input file containing list of logfiles to parse
    \Input *pArgv[]     - commandline arguments
    \Input *pStrList    - list of strings to extract from file

    \Output
        LogParseRefT *  - log parser containing merged log entries from input files

    \Version 09/27/2013 (jbrookes)
*/
/********************************************************************************F*/
static LogParseRefT *_LogParserLoadFilesFromInputfile(LogParserAppT *pApp, const char *pStrInpFile, const char *pArgv[], LogParseStrListT *pStrList)
{
    LogParseRefT *pLogParse, *pLogParse2, *pLogParseM;
    char *pInput, *pInpFile, *pInpNext;
    char strFileName[256];
    uint32_t uStartTick;
    int32_t iFileSize;

    if ((pInpFile = ZFileLoad(pStrInpFile, &iFileSize, FALSE)) == NULL)
    {
        ZPrintf("logparser: could not load input file '%s'\n", pStrInpFile);
        return(NULL);
    }

    // create empty log parser
    pLogParse = LogParseCreate(NULL, 0);

    for (pInput = pInpFile; (pInput-pInpFile) < iFileSize; pInput = pInpNext+1)
    {
        if ((pInpNext = strchr(pInput, '\n')) != NULL)
        {
            *pInpNext = '\0';
        }
        ds_strnzcpy(strFileName, pInput, sizeof(strFileName));
        if ((pLogParse2 = _LogParserLoadFile(pApp, strFileName, pStrList)) != NULL)
        {
            uStartTick = NetTick();
            if ((pLogParseM = LogParseMerge(pLogParse, pLogParse2)) != NULL)
            {
                ZPrintf("%s: merged in %dms\n", pArgv[0], NetTickDiff(NetTick(), uStartTick));
                LogParseDestroy(pLogParse);
                pLogParse = pLogParseM;
            }
            else
            {
                ZPrintf("%s: failed to merge\n", pArgv[0]);
            }
            LogParseDestroy(pLogParse2);
        }
    }

    ZMemFree(pInpFile);
    return(pLogParse);
}

/*F********************************************************************************/
/*!
    \Function _LogParserNumberListInsert

    \Description
        Unsigned insert of numbers into number list, with or without sorting

    \Input *pNumberList - number list to insert into
    \Input uListCount   - number of entries in list
    \Input uListSize    - max size of list
    \Input uNumber      - number to insert in list
    \Input bSort        - TRUE to sort, else just append
    \Input *pIndex      - [out] storage for index of inserted number (may be NULL)

    \Output
        uint32_t        - TRUE if added, else FALSE

    \Version 11/13/2012 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _LogParserNumberListInsert(uint32_t *pNumberList, uint32_t uListCount, uint32_t uListSize, uint32_t uNumber, uint8_t bSort, uint32_t *pIndex)
{
    uint32_t uNumberIdx, uResult;

    // bail if list is full
    if ((uListCount + 1) >= uListSize)
    {
        return(0);
    }

    // find insertion point
    for (uNumberIdx = 0, uResult = 0; (uNumberIdx < uListCount) && (pNumberList[uNumberIdx] != 0); uNumberIdx += 1)
    {
        // if we're already in the list, do not add
        if (pNumberList[uNumberIdx] == uNumber)
        {
            break;
        }
        else if (bSort && (pNumberList[uNumberIdx] > uNumber))
        {
            break;
        }
    }

    if (pNumberList[uNumberIdx] != uNumber)
    {
        // make room for new entry
        memmove(pNumberList+uNumberIdx+1, pNumberList+uNumberIdx, (uListCount-uNumberIdx)*sizeof(*pNumberList));
        // write new entry
        pNumberList[uNumberIdx] = uNumber;
        uResult = 1;
    }
    // write index, if requested
    if (pIndex != NULL)
    {
        *pIndex = uNumberIdx;
    }
    return(uResult);
}

/*F********************************************************************************/
/*!
    \Function _LogParserIspListInsert

    \Description
        Insert of ISP into ISP list, with or without sorting

    \Input *pIspRecords - [in/out] list of IPS records to insert into
    \Input uListCount   - number of entries in list
    \Input uListSize    - max size of list
    \Input *pIsp        - ISP to insert
    \Input *pCountry    - country of ISP to insert
    \Input bSort        - TRUE to sort, else just append
    \Input *pIndex      - [out] storage for index of inserted entry (may be NULL)

    \Output
        uint32_t        - TRUE if added, else FALSE

    \Version 06/23/2020 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _LogParserIspListInsert(IspRecordT *pIspRecords, uint32_t uListCount, uint32_t uListSize, const char *pIsp, const char *pCountry, uint8_t bSort, uint32_t *pIndex)
{
    uint32_t uIspIdx, uResult;

    // bail if list is full
    if ((uListCount + 1) >= uListSize)
    {
        return(0);
    }

    // find insertion point
    for (uIspIdx = 0, uResult = 0; (uIspIdx < uListCount) && (pIspRecords[uIspIdx].strOrg[0] != '\0'); uIspIdx += 1)
    {
        // if we're already in the list, do not add
        if (!strncmp(pIspRecords[uIspIdx].strOrg, pIsp, sizeof(pIspRecords[0].strOrg)-1))
        {
            break;
        }
        else if (bSort && (strncmp(pIspRecords[uIspIdx].strOrg, pIsp, sizeof(pIspRecords[uIspIdx].strOrg)-1) > 0))
        {
            break;
        }
    }

    if (strcmp(pIspRecords[uIspIdx].strOrg, pIsp))
    {
        // make room for new entry
        memmove(pIspRecords+uIspIdx+1, pIspRecords+uIspIdx, (uListCount-uIspIdx)*sizeof(pIspRecords[0]));
        // write new entry
        ds_strnzcpy(pIspRecords[uIspIdx].strOrg, pIsp, sizeof(pIspRecords[0].strOrg));
        // copy country
        ds_strnzcpy(pIspRecords[uIspIdx].strCountry, pCountry, sizeof(pIspRecords[0].strCountry));
        // clear count
        pIspRecords[uIspIdx].uIspCount = 0;
        uResult = 1;
    }
    // write index, if requested
    if (pIndex != NULL)
    {
        *pIndex = uIspIdx;
    }
    return(uResult);
}

/*F********************************************************************************/
/*!
    \Function _LogParserAsnSort

    \Description
        qsort callback sort function to sort ASN list

    \Input *pA          - asn element a
    \Input *pB          - asn element b

    \Output
        int             - comparison of a to b

    \Version 06/18/2020 (jbrookes)
*/
/********************************************************************************F*/
static int _LogParserAsnSort(void const *pA, void const *pB)
{
    const AsnRecordT *pAsnA = (const AsnRecordT *)pA;
    const AsnRecordT *pAsnB = (const AsnRecordT *)pB;
    return(pAsnB->uAsnCount-pAsnA->uAsnCount);
}

/*F********************************************************************************/
/*!
    \Function _LogParserIspSort

    \Description
        qsort callback sort function to sort ISP list

    \Input *pA          - asn element a
    \Input *pB          - asn element b

    \Output
        int             - comparison of a to b

    \Version 06/18/2020 (jbrookes)
*/
/********************************************************************************F*/
static int _LogParserIspSort(void const *pA, void const *pB)
{
    const IspRecordT *pStrA = (const IspRecordT *)pA;
    const IspRecordT *pStrB = (const IspRecordT *)pB;
    return(pStrB->uIspCount-pStrA->uIspCount);
}

/*F********************************************************************************/
/*!
    \Function _LogParserStallEventSort

    \Description
        qsort callback sort function to sort stall records

    \Input *pA          - stall record a
    \Input *pB          - stall record b

    \Output
        int             - comparison of a to b

    \Version 06/18/2020 (jbrookes)
*/
/********************************************************************************F*/
static int _LogParserStallEventSort(void const *pA, void const *pB)
{
    const LogParserStallRecordT *pStallA = (const LogParserStallRecordT *)pA;
    const LogParserStallRecordT *pStallB = (const LogParserStallRecordT *)pB;
    int iResult = 0;
    if (pStallA->uAddr < pStallB->uAddr)
    {
        iResult = -1;
    }
    else if (pStallA->uAddr > pStallB->uAddr)
    {
        iResult = 1;
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function _LogParserClientListGet

    \Description
        Get client from client list

    \Input *pApp            - parser app state
    \Input *pClientList     - client list to find client in
    \Input uNumClients      - number of clients in client list
    \Input uClientIdent     - ident of client to get
    \Input *pIndex          - [out] storage for client index if present, else index where client would be

    \Output
        LogParserClientInfoT * - client, or NULL if not found

    \Version 08/03/2020 (jbrookes)
*/
/********************************************************************************F*/
static const LogParserClientInfoT *_LogParserClientListGet(LogParserAppT *pApp, const LogParserClientInfoT *pClientList, uint32_t uNumClients, uint32_t uClientIdent, uint32_t *pIndex)
{
    const LogParserClientInfoT *pClientInfo;
    int32_t iLo, iHi, iEntry;
    uint32_t uCheckIdent;
    uint8_t bDone;

    // find address in range
    for (bDone = FALSE, iEntry = 0, iLo = 0, iHi = (signed)uNumClients, pClientInfo = NULL; !bDone; )
    {
        // get entry to look at
        if ((iEntry = ((iHi-iLo)>>1) + iLo) == iLo)
        {
            bDone = TRUE;
        }

        // process based on entry key
        if ((uCheckIdent = pClientList[iEntry].uIdent) < uClientIdent)
        {
            iLo = iEntry;
        }
        else if (uCheckIdent > uClientIdent)
        {
            iHi = iEntry;
        }
        else
        {
            pClientInfo = pClientList+iEntry;
            bDone = TRUE;
        }
    }
    // write final index (either the client index, or where the client index would be inserted after if it isn't present)
    if (pIndex != NULL)
    {
        if ((uCheckIdent = pClientList[iEntry].uIdent) < uClientIdent)
        {
            iEntry = iHi;
        }
        else if (uCheckIdent > uClientIdent)
        {
            iEntry = iLo;
        }
        *pIndex = iEntry;
    }
    return(pClientInfo);
}

#if 0
/*F********************************************************************************/
/*!
    \Function _LogParserClientListGetAddr

    \Description
        Get client address from client list

    \Input *pApp            - parser app state
    \Input *pClientList     - client list to find client in
    \Input uNumClients      - number of clients in client list
    \Input uClientIdent     - ident of client to get

    \Output
        uint32_t            - address, or zero if not found

    \Version 08/03/2020 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _LogParserClientListGetAddr(LogParserAppT *pApp, const LogParserClientInfoT *pClientList, uint32_t uNumClients, uint32_t uClientIdent)
{
    const LogParserClientInfoT *pClientInfo;
    uint32_t uClientAddr = 0;
    if ((pClientInfo = _LogParserClientListGet(pApp, pClientList, uNumClients, uClientIdent, NULL)) != NULL)
    {
        uClientAddr = pClientInfo->uAddr;
    }
    return(uClientAddr);
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserClientListAdd

    \Description
        Add a client to client list (sorted)

    \Input *pApp            - parser app state
    \Input *pClientList     - client list to add client to
    \Input uNumClients      - number of clients in client list
    \Input uMaxClients      - max number of clients list will hold
    \Input uClientIdent     - ident of client to add
    \Input uClientAddr      - address of client to add
    \Input uAddTime         - time client was added (in actuality matched, not added, but close enough)
    \Input *pIndex          - [out] storage for client index if present, else index where client would be

    \Output
        uint32_t            - one if added, else zero

    \Version 08/03/2020 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _LogParserClientListAdd(LogParserAppT *pApp, LogParserClientInfoT *pClientList, uint32_t uNumClients, uint32_t uMaxClients, uint32_t uClientIdent, uint32_t uClientAddr, uint64_t uAddTime, uint32_t *pIndex)
{
    uint32_t uClient, uResult = 0;

    // bail if list is full
    if ((uNumClients+1) >= uMaxClients)
    {
        pApp->iClientListFull += 1;
        return(0);
    }

    // get client index (if present) or insertion point (if not)
    _LogParserClientListGet(pApp, pClientList, uNumClients, uClientIdent, &uClient);

    // if it's not in the list, add it
    if (pClientList[uClient].uIdent != uClientIdent)
    {
        // make room for new entry
        memmove(pClientList+uClient+1, pClientList+uClient, (uNumClients-uClient)*sizeof(*pClientList));
        // write new entry
        pClientList[uClient].uAddTime = uAddTime;
        pClientList[uClient].uAddr = uClientAddr;
        pClientList[uClient].uIdent = uClientIdent;
        pClientList[uClient].bDisc = FALSE;
        uResult = 1;
    }
    // write index, if requested
    if (pIndex != NULL)
    {
        *pIndex = uClient;
    }
    return(uResult);
}

/*F********************************************************************************/
/*!
    \Function _LogParserClientListDel

    \Description
        Del a client from client list

    \Input *pApp            - parser app state
    \Input *pClientList     - client list to add client to
    \Input uNumClients      - number of clients in client list
    \Input uClient          - index of client to del, or -1 to use index
    \Input uClientIdent     - ident of client to del if index is -1

    \Output
        uint32_t            - one if deleted, else zero

    \Version 08/03/2020 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _LogParserClientListDel(LogParserAppT *pApp, LogParserClientInfoT *pClientList, uint32_t uNumClients, uint32_t uClient, uint32_t uClientIdent)
{
    // find client in list
    if ((uClient == (unsigned)-1) && !_LogParserClientListGet(pApp, pClientList, uNumClients, uClientIdent, &uClient))
    {
        return(0);
    }
    // remove entry
    memmove(pClientList+uClient, pClientList+uClient+1, (uNumClients-uClient)*sizeof(*pClientList));
    return(1);
}

#if 0
/*F********************************************************************************/
/*!
    \Function _LogParserClientListGetOldest

    \Description
        Returns oldest client in client list

    \Input *pApp            - parser app state
    \Input *pClientList     - client list
    \Input uNumClients      - number of clients in client list

    \Output
        LogParserClientInfoT * - pointer to oldest client in client list

    \Version 08/13/2020 (jbrookes)
*/
/********************************************************************************F*/
static LogParserClientInfoT *_LogParserClientListGetOldest(LogParserAppT *pApp, LogParserClientInfoT *pClientList, uint32_t uNumClients)
{
    uint64_t uOldestTime;
    uint32_t uClient, uOldestClient;

    for (uClient = 0, uOldestClient = 0, uOldestTime = 0; uClient < uNumClients; uClient += 1)
    {
        if (pClientList[uClient].uAddTime < uOldestTime)
        {
            uOldestClient = uClient;
            uOldestTime = pClientList[uClient].uAddTime;
        }
    }

    return(pClientList+uOldestClient);
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserClientListUpdate

    \Description
        Update client list

    \Input *pApp            - parser app state
    \Input *pClientList     - client list
    \Input uNumClients     -  number of clients in client list
    \Input uCurTime         - latest log entry time

    \Input
        uint32_t            - updated number of clients in client list

    \Version 08/13/2020 (jbrookes)
*/
/********************************************************************************F*/
static uint32_t _LogParserClientListUpdate(LogParserAppT *pApp, LogParserClientInfoT *pClientList, uint32_t uNumClients, uint64_t uCurTime)
{
    uint64_t uOldestTime;
    uint32_t uClient, uOldestClient;

    for (uClient = 0, uOldestClient = 0, uOldestTime = 0; uClient < uNumClients; uClient += 1)
    {
        /* process client list and clean up disconnected clients, or old clients that are dangling; the latter can
           happen e.g. because a game is deleted without removing all of the users first (this seems to happen when
           a gameserver goes offline.  we could fix this by adding parsing of nusr and dgam messages at the cost of
           bigger parsed logs and more processing time */
        if (pClientList[uClient].bDisc || (NetTickDiff(uCurTime, pClientList[uClient].uAddTime) > 45*60*1000))
        {
            _LogParserClientListDel(pApp, pClientList, uNumClients, uClient, 0);
            uNumClients -= 1;
        }
    }

    return(uNumClients);
}

/*F********************************************************************************/
/*!
    \Function _LogParserAddrGeoIPRecordLookup

    \Description
        Look up address in GeoIP main database, populate stall record

    \Input *pApp        - parser app state
    \Input uAddr        - address to look up
    \Input *pStallRecord- [out] storage for geoip information

    \Version 09/27/2013 (jbrookes)
*/
/********************************************************************************F*/
#if defined(LOGPARSE_GEOIP)
static void _LogParserAddrGeoIPRecordLookup(LogParserAppT *pApp, uint32_t uAddr, LogParserStallRecordT *pStallRecord)
{
    GeoIPRecord *pGeoRecord = NULL;
    if ((pGeoRecord = GeoIP_record_by_ipnum(pApp->pGeoIP, uAddr)) != NULL)
    {
        /* available fields: area_code, charset, city, continent_code, pGeoRecord->country_code3,
           country_name, dma_code, latitude, longitude, metro_code, postal_code, region */

        pStallRecord->uAddr = uAddr;
        pStallRecord->fLatitude = pGeoRecord->latitude;
        pStallRecord->fLongitude = pGeoRecord->longitude;
        ds_strnzcpy(pStallRecord->strCity, pGeoRecord->city ? pGeoRecord->city : "", sizeof(pStallRecord->strCity));
        ds_strnzcpy(pStallRecord->strRegion, pGeoRecord->region ? pGeoRecord->region : "", sizeof(pStallRecord->strRegion));
        ds_strnzcpy(pStallRecord->strCountry, pGeoRecord->country_name ? pGeoRecord->country_name : "", sizeof(pStallRecord->strCountry));
        pStallRecord->bValid = TRUE;

        // clean up record
        GeoIPRecord_delete(pGeoRecord);
    }
    else
    {
        pStallRecord->bValid = FALSE;
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserAddrGeoIPOrgLookup

    \Description
        Look up address in GeoIP org database, add org to stall record

    \Input *pApp        - parser app state
    \Input uAddr        - address to look up
    \Input *pStallRecord- [out] storage for geoip information

    \Version 09/27/2013 (jbrookes)
*/
/********************************************************************************F*/
#if defined(LOGPARSE_GEOIP)
static void _LogParserAddrGeoIPOrgLookup(LogParserAppT *pApp, uint32_t uAddr, LogParserStallRecordT *pStallRecord)
{
    const char *pOrg;
    if ((pApp->pGeoIPOrg != NULL) && ((pOrg = GeoIP_name_by_ipnum(pApp->pGeoIPOrg, uAddr)) != NULL))
    {
        ds_strnzcpy(pStallRecord->strOrg, pOrg, sizeof(pStallRecord->strOrg));
    }
    else
    {
        ds_memclr(pStallRecord->strOrg, sizeof(pStallRecord->strOrg));
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserAsnEntryParse

    \Description
        Parse an ASN entry from csv-formatted database

    \Input *pEntry      - entry to parse e.g. "1.0.0.0/24,13335,CLOUDFLARENET"
    \Input *pAddr       - [out] storage for address portion of record
    \Input *pMask       - [out] storage for mask portion of record
    \Input *pAsn        - [out] storage for ASN
    \Input *pOrg        - [out] storage for organization string
    \Input iOrgSize     - length of org buffer

    \Output
        const char *    - pointer past end of this entry

    \Version 06/12/2020 (jbrookes)
*/
/********************************************************************************F*/
#if defined(LOGPARSE_GEOIP)
static const char *_LogParserAsnEntryParse(const char *pEntry, uint32_t *pAddr, uint32_t *pMask, uint32_t *pAsn, char *pOrg, int32_t iOrgSize)
{
    char strLine[256], *pEnd, *pLineEnd, *pStart;
    int32_t iLineEnd;

    // get end of line and copy the line to temp buffer
    if ((pLineEnd = pEnd = strchr(pEntry, '\n')) == NULL)
    {
        return(NULL);
    }
    ds_strsubzcpy(strLine, sizeof(strLine), pEntry, (int32_t)(pEnd-pEntry));
    iLineEnd = pEnd-pEntry-1;

    // parse address
    if ((pEnd = strchr(strLine, '/')) == NULL)
    {
        return(NULL);
    }
    *pEnd++ = '\0';
    *pAddr = SocketInTextGetAddr(pEntry);

    // parse mask
    pEntry = pEnd;
    if ((pEnd = strchr(pEntry, ',')) == NULL)
    {
        return(NULL);
    }
    *pEnd++ = '\0';
    *pMask = strtol(pEntry, NULL, 10);

    // parse asn
    pEntry = pEnd;
    if ((pEnd = strchr(pEntry, ',')) == NULL)
    {
        return(NULL);
    }
    *pEnd++ = '\0';
    *pAsn = strtol(pEntry, NULL, 10);

    // parse name (the rest is just the name)
    pStart = pEnd;
    pLineEnd = strLine+iLineEnd;
    // if quoted, strip the quotes
    if ((*pStart == '"') && (*pLineEnd == '"'))
    {
        pStart += 1;
        *pLineEnd = '\0';
    }
    ds_strnzcpy(pOrg, pStart, iOrgSize);

    // return end of line
    return(pLineEnd+1);
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserAddrGeoIPAsnLookup

    \Description
        Look up address in GeoIP asn database, add asn to stall record

    \Input *pApp        - parser app state
    \Input uAddr        - address to look up
    \Input *pStallRecord- [out] storage for geoip information

    \Version 06/12/2020 (jbrookes)
*/
/********************************************************************************F*/
#if defined(LOGPARSE_GEOIP)
static void _LogParserAddrGeoIPAsnLookup(LogParserAppT *pApp, uint32_t uAddr, LogParserStallRecordT *pStallRecord)
{
    uint32_t uAsnAddr, uAsnMask, uAsn;
    char strOrg[64];
    int32_t iLo, iHi, iEntry;

    // clear result
    pStallRecord->uAsn = 0;
    pStallRecord->strAsnOrg[0] = '\0';

    // skip if not available
    if ((pApp->pGeoIPAsnParsed == NULL) || (pApp->pGeoIPAsnParsed->uNumLineEntries == 0))
    {
        return;
    }   

    // find address in range
    for (iLo = 0, iHi = (signed)pApp->pGeoIPAsnParsed->uNumLineEntries; (iHi-iLo) > 1; )
    {
        // get entry to look at
        iEntry = ((iHi-iLo)>>1) + iLo;
        // parse entry
        _LogParserAsnEntryParse(pApp->pGeoIPAsnParsed->pLineBuf[iEntry].pSrcBuf + pApp->pGeoIPAsnParsed->pLineBuf[iEntry].uLineStart, &uAsnAddr, &uAsnMask, &uAsn, strOrg, sizeof(strOrg));

        // calculate binary mask
        uAsnMask = ~((1 << (32-uAsnMask)) - 1);
        // see if address is in range
        if ((uAddr & uAsnMask) == (uAsnAddr & uAsnMask))
        {
            // found it; copy data and return
            pStallRecord->uAsn = uAsn;
            ds_strnzcpy(pStallRecord->strAsnOrg, strOrg, sizeof(pStallRecord->strAsnOrg));
            break;
        }

        // update to new range
        if (uAsnAddr < uAddr)
        {
            iLo = iEntry;
        }
        else if (uAsnAddr > uAddr)
        {
            iHi = iEntry;
        }
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserAddrCalcDist

    \Description
        Calculate spherical distance between two lat/long coordinates using
        spherical low of cosines.  Inputs are assumed to be in radians.

    \Input fLatA    - latitude of point a
    \Input fLonA    - longitude of point a
    \Input fLatB    - latitude of point b
    \Input fLonB    - longitude of point b

    \Output
        float       - distance in km

    \Version 09/27/2013 (jbrookes)
*/
/********************************************************************************F*/
#if defined(LOGPARSE_GEOIP)
static float _LogParserAddrCalcDist(float fLatA, float fLonA, float fLatB, float fLonB)
{
    double dSinLat, dCosLat, dCosLon;
    const float fKm = 6371.0f;
    float fDist, fVal;

    // calculate distance using spherical law of cosines:
    // acos(sin(lat1)*sin(lat2)+cos(lat1)*cos(lat2)*cos(lon2-lon1))*6371
    dSinLat = sin(fLatA) * sin(fLatB);
    dCosLat = cos(fLatA) * cos(fLatB);
    dCosLon = cos(fLonB - fLonA);

    fVal = DS_CLAMP(dSinLat + dCosLat * dCosLon, -1.0f, 1.0f);
    fDist = acos(fVal) * fKm;
    return(fDist);
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserGetThompsonTauValue

    \Description
        Calculate a Thompson Tau value a la
        https://www.statisticshowto.datasciencecentral.com/modified-thompson-tau-test/

    \Input *pApp            - parser app state
    \Input *pResult         - storage for lat, long, and radius of event
    \Input *pStallRecords   - stall records
    \Input uNumRecords      - number of stall records

    \Output
        float               - thompson tau value

    \Version 10/??/2017 (jbrookes)
*/
/********************************************************************************F*/
#if defined(LOGPARSE_GEOIP)
static float _LogParserGetThompsonTauValue(uint32_t uNumSamples)
{
    static ThompsonTauTableEntryT _ThompsonTauTable[] =
    {
        {    0,    0.0f }, {    1,    0.0f }, {    2,    0.0f }, {    3, 1.1511f }, {    4, 1.4250f }, {    5, 1.5712f }, {    6, 1.6563f }, {    7, 1.7110f }, {    8, 1.7491f }, {    9, 1.7770f },
        {   10, 1.7984f }, {   11, 1.8153f }, {   12, 1.8290f }, {   13, 1.8403f }, {   14, 1.8498f }, {   15, 1.8579f }, {   16, 1.8649f }, {   17, 1.8710f }, {   18, 1.8764f }, {   19, 1.8811f },
        {   20, 1.8853f }, {   21, 1.8891f }, {   22, 1.8926f }, {   23, 1.8957f }, {   24, 1.8985f }, {   25, 1.9011f }, {   26, 1.9035f }, {   27, 1.9057f }, {   28, 1.9078f }, {   29, 1.9096f },
        {   30, 1.9114f }, {   31, 1.9130f }, {   32, 1.9146f }, {   33, 1.9160f }, {   34, 1.9174f }, {   35, 1.9186f }, {   36, 1.9198f }, {   37, 1.9209f }, {   38, 1.9220f },
        {   40, 1.9240f }, {   42, 1.9257f }, {   44, 1.9273f }, {   46, 1.9288f }, {   48, 1.9301f },
        {   50, 1.9314f }, {   55, 1.9340f }, {   60, 1.9362f }, {   65, 1.9381f },
        {   70, 1.9397f }, {   80, 1.9423f }, {   90, 1.9443f },
        {  100, 1.9459f }, {  200, 1.9530f },
        {  500, 1.9572f }, { 1000, 1.9586f },
        { 5000, 1.9597f },
        { 0xffffffff, 1.9600f }
    };
    uint32_t uEntry;

    for (uEntry = 3; _ThompsonTauTable[uEntry].uNumSamples != 0xffffffff; uEntry += 1)
    {
        if ((uNumSamples == _ThompsonTauTable[uEntry].uNumSamples) || (uNumSamples < _ThompsonTauTable[uEntry+1].uNumSamples))
        {
            break;
        }
    }

    return(_ThompsonTauTable[uEntry].fValue);
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserCalcCenter

    \Description
        Calculate center coordinate of records

    \Input *pApp            - parser app state
    \Input *pResult         - storage for lat, long, and radius of event
    \Input *pStallRecords   - stall records
    \Input uNumRecords      - number of stall records

    \Version 04/04/2019 (jbrookes) split from _LogParserAddrCalcImpact
*/
/********************************************************************************F*/
#if defined(LOGPARSE_GEOIP)
static void _LogParserCalcCenter(LogParserAppT *pApp, float *pResult, LogParserStallRecordT *pStallRecords, uint32_t uNumRecords)
{
    float fCtrLat, fCtrLon;
    uint32_t uRecord, uValidRecords;

    for (uRecord = 0, uValidRecords = 0, fCtrLat = 0.0f, fCtrLon = 0.0f; uRecord < uNumRecords; uRecord += 1)
    {
        // skip addresses we failed to look up
        if (pStallRecords[uRecord].bValid == FALSE)
        {
            continue;
        }
        uValidRecords += 1;
        
        // keep track of sum of lat/lon for avg
        fCtrLat += pStallRecords[uRecord].fLatitude;
        fCtrLon += pStallRecords[uRecord].fLongitude;
    }

    // calculate center
    if (uValidRecords > 0)
    {
        pResult[LOGPARSE_CNTRLAT] = fCtrLat/(float)uValidRecords;
        pResult[LOGPARSE_CNTRLON] = fCtrLon/(float)uValidRecords;
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserCalcCenterDist

    \Description
        Calculate bounding circle of specified stall records

    \Input *pApp            - parser app state
    \Input *pResult         - storage for lat, long, and radius of event
    \Input *pStallRecords   - stall records
    \Input uNumRecords      - number of stall records

    \Version 04/04/2019 (jbrookes) split from _LogParserAddrCalcImpact
*/
/********************************************************************************F*/
#if defined(LOGPARSE_GEOIP)
static void _LogParserCalcCenterDist(LogParserAppT *pApp, float *pResult, LogParserStallRecordT *pStallRecords, uint32_t uNumRecords)
{
    float fMaxDist, fSumDist, fLatA, fLonA, fDist;
    uint32_t uRecord, uValidRecords;

    for (uRecord = 0, uValidRecords = 0, fMaxDist = 0.0f, fSumDist = 0.0f; uRecord < uNumRecords; uRecord += 1)
    {
        // skip addresses we failed to look up
        if (pStallRecords[uRecord].bValid == FALSE)
        {
            continue;
        }
        uValidRecords += 1;

        // reference lat/lon in radians
        fLatA = pStallRecords[uRecord].fLatitude/180.0f;
        fLonA = pStallRecords[uRecord].fLongitude/180.0f;

        // calculate distance between point and center
        fDist = _LogParserAddrCalcDist(fLatA, fLonA, pResult[LOGPARSE_CNTRLAT]/180.0f, pResult[LOGPARSE_CNTRLON]/180.0f);
        if (fMaxDist < fDist)
        {
            fMaxDist = fDist;
        }
        fSumDist += fDist;

        // save distance from center of event
        pStallRecords[uRecord].fDist = fDist;
    }

    // save max distance
    pResult[LOGPARSE_MAXDIST] = fMaxDist;
    // save avg distance
    pResult[LOGPARSE_AVGDIST] = fSumDist/(float)uValidRecords;
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserCalcStdDev

    \Description
        Calculate bounding circle of specified stall records

    \Input *pApp            - parser app state
    \Input *pResult         - storage for lat, long, and radius of event
    \Input *pStallRecords   - stall records
    \Input uNumRecords      - number of stall records

    \Output
        uint32_t            - stddev

    \Version 04/04/2019 (jbrookes) split from _LogParserAddrCalcImpact
*/
/********************************************************************************F*/
#if defined(LOGPARSE_GEOIP)
static void _LogParserCalcStdDev(LogParserAppT *pApp, float *pResult, LogParserStallRecordT *pStallRecords, uint32_t uNumRecords)
{
    uint32_t uRecord, uValidRecords;
    float fValue, fSqrMean;

    // calculate sum of squared differences
    for (uRecord = 0, uValidRecords = 0, fSqrMean = 0.0f; uRecord < uNumRecords; uRecord += 1)
    {
        // skip addresses we failed to look up
        if (!pStallRecords[uRecord].bValid)
        {
            continue;
        }
        uValidRecords += 1;
        // for each record, subtract the mean
        fValue = pStallRecords[uRecord].fDist - pResult[LOGPARSE_AVGDIST];
        // square the result
        fValue *= fValue;
        // add to sum
        fSqrMean += fValue;
    }

    // convert sum to mean
    fSqrMean /= (float) uValidRecords;

    // square root to get standard deviation
    pResult[LOGPARSE_STNDDEV] = sqrtf(fSqrMean);
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserAddrCalcImpact

    \Description
        Calculate bounding circle of specified stall records

    \Input *pApp            - parser app state
    \Input *pResult         - storage for lat, long, and radius of event
    \Input *pStallRecords   - stall records
    \Input uNumRecords      - number of stall records

    \Output
        uint32_t            - number of valid records

    \Version 09/27/2013 (jbrookes)
*/
/********************************************************************************F*/
#if defined(LOGPARSE_GEOIP)
static uint32_t _LogParserAddrCalcImpact(LogParserAppT *pApp, float *pResult, LogParserStallRecordT *pStallRecords, uint32_t uNumRecords)
{
    uint32_t uRecord, uValidRecords;

    // find number of valid records
    for (uRecord = 0, uValidRecords = 0; uRecord < uNumRecords; uRecord += 1)
    {
        // skip addresses we failed to look up
        if (!pStallRecords[uRecord].bValid)
        {
            continue;
        }
        uValidRecords += 1;
    }
    // make sure we have at least one valid record
    if (uValidRecords == 0)
    {
        pResult[0] = pResult[1] = pResult[2] = pResult[3] = pResult[4] = 0.0f;
        return(0);
    }

    // calc center of coordinates (mean)
    _LogParserCalcCenter(pApp, pResult, pStallRecords, uNumRecords);

    // calc distance from center for each coordinate
    _LogParserCalcCenterDist(pApp, pResult, pStallRecords, uNumRecords);

    // calc standard deviation of dataset
    _LogParserCalcStdDev(pApp, pResult, pStallRecords, uNumRecords);

    // return number of valid records
    return(uValidRecords);
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserExcludeOutliers

    \Description
        Calculate outliers in the population using modified thompson tau test.
        See https://en.wikipedia.org/wiki/Outlier

    \Input *pApp            - parser app state
    \Input *pResult         - storage for lat, long, and radius of event
    \Input *pStallRecords   - stall records
    \Input uNumRecords      - number of stall records

    \Output
        uint32_t            - ?

    \Version 04/04/2019 (jbrookes)
*/
/********************************************************************************F*/
#if defined(LOGPARSE_GEOIP)
static uint32_t _LogParserExcludeOutliers(LogParserAppT *pApp, float *pResult, LogParserStallRecordT *pStallRecords, uint32_t uNumRecords)
{
    uint32_t uRecord, uValidRecords;
    uint32_t uMinDist = 0, uMaxDist = 0;
    float fMinDist = 32000.0f, fMaxDist = 0.0f;
    float fAvgDist = pResult[3];
    float fStdDev = pResult[4];
    float fValue, fMinValue, fMaxValue;
    float fTable, fThompson;

    // make sure there is some variation first
    if (fStdDev == 0.0f)
    {
        return(1);
    }

    // find extremes of dataset (smallest and largest)
    for (uRecord = 0, uValidRecords = 0; uRecord < uNumRecords; uRecord += 1)
    {
        // skip addresses we failed to look up
        if (pStallRecords[uRecord].bValid == FALSE)
        {
            continue;
        }
        uValidRecords += 1;

        // keep track of smallest distance
        if (fMinDist > pStallRecords[uRecord].fDist)
        {
            fMinDist = pStallRecords[uRecord].fDist;
            uMinDist = uRecord;
        }

        // keep track of largest distance
        if (fMaxDist < pStallRecords[uRecord].fDist)
        {
            fMaxDist = pStallRecords[uRecord].fDist;
            uMaxDist = uRecord;
        }
    }

    fMinValue = fabsf(fMinDist-fAvgDist);
    fMaxValue = fabsf(fMaxDist-fAvgDist);
    uRecord = (fMaxValue < fMinValue) ? uMinDist : uMaxDist;

    ZPrintf("min=%f, max=%f, uRecord=%d\n", fMinValue, fMaxValue, uRecord);

    // apply modified thompson tau test to consider whether furthest point is an outlier or not
    fValue = fabsf(pStallRecords[uRecord].fDist-fAvgDist);
    fTable = _LogParserGetThompsonTauValue(uValidRecords);
    fThompson = fTable*fStdDev;

    ZPrintf("fValue=%f, fTable=%f, fThompson=%f\n", fValue, fTable, fThompson);
    ZPrintf("record %d is%san outlier\n", uRecord, fValue < fThompson ? " not " : " ");
    pStallRecords[uRecord].bValid = fValue < fThompson ? TRUE : FALSE;

    return(pStallRecords[uRecord].bValid);
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserFormatAddress

    \Description
        Format address for output, with anonymizing if specified

    \Input *pApp            - parser app state
    \Input *pLogParse   - logparse to process

    \Notes
        Assumes only stall entries are present

    \Todo
        Figure out a way to move this logic out of 'generic' log parser app

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static const char *_LogParserFormatAddress(LogParserAppT *pApp, uint32_t uAddr, char *pBuffer, int32_t iBufSize)
{
    // mask off last octuple of IP to anonymize
    if (pApp->bAnonymize)
    {
        uAddr &= ~0xff;
    }
    // format the address
    ds_snzprintf(pBuffer, iBufSize, "%a", uAddr);
    // return string for convenience in printing
    return(pBuffer);
}

#if defined(LOGPARSE_GEOIP)
/*F********************************************************************************/
/*!
    \Function _LogParserStallEventGeoIPProcess

    \Description
        GeoIP lookup, ISP filtering, ASN/ISP list updating, epicenter/radius calc
       
    \Input *pApp            - parser app state
    \Input *pLogParse       - logparse to process
    \Input uNumAddrs        - number of addresses in stall event
    \Input *pStallRecords   - stall records for event
    \Input *pGeoInfo        - GeoIP info to fill in
    \Input *fImpact         - impact array

    \Output
        uint32_t            - updated stall count

    \Version 07/06/2020 (jbrookes) Split from _LogParserStallEventProcess
*/
/********************************************************************************F*/
static uint32_t _LogParserStallEventGeoIPProcess(LogParserAppT *pApp, LogParseRefT *pLogParse, uint32_t uNumAddrs, LogParserStallRecordT *pStallRecords, LogParserGeoInfoT *pGeoInfo, float *fImpact)
{
    uint32_t uAddr, uStall;
    uint32_t uAsnIndex, uIspIndex;

    if ((uNumAddrs == 0) || (pApp->pGeoIP == NULL))
    {
        return(0);
    }

    // generate geoip info from source addresses
    for (uAddr = 0; uAddr < uNumAddrs; uAddr += 1)
    {
        _LogParserAddrGeoIPRecordLookup(pApp, pStallRecords[uAddr].uAddr, &pStallRecords[uAddr]);
        _LogParserAddrGeoIPOrgLookup(pApp, pStallRecords[uAddr].uAddr, &pStallRecords[uAddr]);
        _LogParserAddrGeoIPAsnLookup(pApp, pStallRecords[uAddr].uAddr, &pStallRecords[uAddr]);
    }

    // filter based on asn list
    if (pApp->strAsnFilter[0] != 0)
    {
        uint32_t uAsnFilter = strtol(pApp->strAsnFilter, NULL, 10);
        uint8_t bStrEqual;

        for (uAddr = 0; uAddr < uNumAddrs; uAddr += 1)
        {
            bStrEqual = pStallRecords[uAddr].uAsn == uAsnFilter;
            if (bStrEqual == pApp->bAsnExclude)
            {
                pStallRecords[uAddr].bValid = FALSE;
            }
        }
    }
    // filter based on isp list
    if (pApp->strIspFilter[0] != 0)
    {
        uint8_t bStrEqual;
        for (uAddr = 0; uAddr < uNumAddrs; uAddr += 1)
        {
            bStrEqual = ds_stricmp(pStallRecords[uAddr].strOrg, pApp->strIspFilter) ? FALSE : TRUE;
            if (bStrEqual == pApp->bIspExclude)
            {
                pStallRecords[uAddr].bValid = FALSE;
            }
        }
    }
    // update stall count after filter
    for (uAddr = 0, uStall = 0; uAddr < uNumAddrs; uAddr += 1)
    {
        uStall += pStallRecords[uAddr].bValid;
    }

    // update asn list
    for (uAddr = 0, uAsnIndex = 0; uAddr < uNumAddrs; uAddr += 1)
    {
        if (!pStallRecords[uAddr].bValid)
        {
            continue;
        }
        // update asn list
        pGeoInfo->uNumAsns += _LogParserNumberListInsert(pGeoInfo->aAsnList, pGeoInfo->uNumAsns, MAX_ASNS, pStallRecords[uAddr].uAsn, FALSE, &uAsnIndex);
        // update asn count
        pGeoInfo->aAsnCounts[uAsnIndex] += 1;
        // save an address for later lookup
        pGeoInfo->aAsnAddrs[uAsnIndex] = pStallRecords[uAddr].uAddr;
    }

    // update isp list
    for (uAddr = 0, uIspIndex = 0; uAddr < uNumAddrs; uAddr += 1)
    {
        if (!pStallRecords[uAddr].bValid)
        {
            continue;
        }
        // update isp list
        pGeoInfo->uNumIsps += _LogParserIspListInsert(pGeoInfo->aIspRecords, pGeoInfo->uNumIsps, MAX_ISPS, pStallRecords[uAddr].strOrg, pStallRecords[uAddr].strCountry, FALSE, &uIspIndex);
        // update isp count
        pGeoInfo->aIspRecords[uIspIndex].uIspCount += 1;
    }

    // calculate 'epicenter' and radius of impact
    for ( ; ;)
    {
        if (_LogParserAddrCalcImpact(pApp, fImpact, pStallRecords, uNumAddrs) == 0)
        {
            break;
        }

        // weed out events that are geographic outliers
        if (pApp->bOutliers)
        {
            if (_LogParserExcludeOutliers(pApp, fImpact, pStallRecords, uNumAddrs))
            {
                break;
            }
        }
        else
        {
            break;
        }
    }

    // calculate exclusion $$TODO
    #if 0
    for (uAddr = 0; uAddr < uNumAddrs; uAddr += 1)
    {
        uint8_t bExcluded = StallRecords[uAddr].fDist > fImpact[4]*4.0f;
    }
    #endif

    // return updated stall count
    return(uStall);
}
#endif

#if defined(LOGPARSE_GEOIP)
/*F********************************************************************************/
/*!
    \Function _LogParserEventAsnLog

    \Description
        Log ASNs in event
       
    \Input *pApp            - parser app state
    \Input *pGeoInfo        - GeoIP info
    \Input uMinAsnReport    - minimum number of counts to display an ASN
    \Input bDiscs           - log disconnects

    \Version 07/10/2020 (jbrookes) Split from _LogParserStallEventProcess
*/
/********************************************************************************F*/
static void _LogParserEventAsnLog(LogParserAppT *pApp, const LogParserGeoInfoT *pGeoInfo, uint32_t uMinAsnReport, uint8_t bDiscs)
{
    LogParserStallRecordT StallRecord;
    AsnRecordT aRecords[MAX_ASNS];
    uint32_t uAsn;

    if (!pApp->bAsnReport)
    {
        return;
    }

    // build asn record list for sorting
    for (uAsn = 0; uAsn < pGeoInfo->uNumAsns; uAsn += 1)
    {
        aRecords[uAsn].uAsn = pGeoInfo->aAsnList[uAsn];
        aRecords[uAsn].uAsnAddr = pGeoInfo->aAsnAddrs[uAsn];
        aRecords[uAsn].uAsnCount = pGeoInfo->aAsnCounts[uAsn];
        aRecords[uAsn].uAsnDiscs = pGeoInfo->aAsnDiscs[uAsn];
    }

    // sort it
    qsort(aRecords, pGeoInfo->uNumAsns, sizeof(aRecords[0]), _LogParserAsnSort);

    // look up asn info and log to output
    ZPrintf("\nlogparser: ASN Report - %d ASNs\n", pGeoInfo->uNumAsns);
    for (uAsn = 0; (uAsn < pGeoInfo->uNumAsns) && (aRecords[uAsn].uAsnCount >= uMinAsnReport); uAsn += 1)
    {
        _LogParserAddrGeoIPAsnLookup(pApp, aRecords[uAsn].uAsnAddr, &StallRecord);
        if (!bDiscs)
        {
            ZPrintf("logparser: %-10u %-48s %5d\n", StallRecord.uAsn, StallRecord.strAsnOrg, aRecords[uAsn].uAsnCount);
        }
        else
        {
            ZPrintf("logparser: %-10u %-48s %5d %5d\n", StallRecord.uAsn, StallRecord.strAsnOrg, aRecords[uAsn].uAsnCount, aRecords[uAsn].uAsnDiscs);
        }
    }
    // sum up the rest in a single "other" report
    if (uAsn < pGeoInfo->uNumAsns)
    {
        uint32_t uAsnOtherCount, uAsnOtherDiscs;
        for (uAsnOtherCount = uAsnOtherDiscs = 0; uAsn < pGeoInfo->uNumAsns; uAsn += 1)
        {
            uAsnOtherCount += aRecords[uAsn].uAsnCount;
            uAsnOtherDiscs += aRecords[uAsn].uAsnDiscs;
        }
        if (!bDiscs)
        {
            ZPrintf("logparser: %-10s %-48s %5d\n", "-----", "Other", uAsnOtherCount);
        }
        else
        {
            ZPrintf("logparser: %-10s %-48s %5d %5d\n", "-----", "Other", uAsnOtherCount, uAsnOtherDiscs);
        }
    }
}
#endif

#if defined(LOGPARSE_GEOIP)
/*F********************************************************************************/
/*!
    \Function _LogParserEventIspLog

    \Description
        Log ISPs in event
       
    \Input *pApp            - parser app state
    \Input *pGeoInfo        - GeoIP info
    \Input uMinIspReport    - minimum number of counts to display an ISP
    \Input bDiscs           - log disconnects

    \Version 07/10/2020 (jbrookes) Split from _LogParserStallEventProcess
*/
/********************************************************************************F*/
static void _LogParserEventIspLog(LogParserAppT *pApp, /* const */ LogParserGeoInfoT *pGeoInfo, uint32_t uMinIspReport, uint8_t bDiscs) //$$todo - fix this so it can be const
{
    uint32_t uIsp;

    if (!pApp->bIspReport)
    {
        return;
    }

    // sort ISP list
    qsort(pGeoInfo->aIspRecords, pGeoInfo->uNumIsps, sizeof(pGeoInfo->aIspRecords[0]), _LogParserIspSort);

    ZPrintf("\nlogparser: ISP Report - %d ISPs\n", pGeoInfo->uNumIsps);
    for (uIsp = 0; (uIsp < pGeoInfo->uNumIsps) && (pGeoInfo->aIspRecords[uIsp].uIspCount >= uMinIspReport); uIsp += 1)
    {
        if (!bDiscs)
        {
            ZPrintf("logparser: %-48s %-20s %5d\n", pGeoInfo->aIspRecords[uIsp].strOrg, pGeoInfo->aIspRecords[uIsp].strCountry, pGeoInfo->aIspRecords[uIsp].uIspCount);
        }
        else
        {
            ZPrintf("logparser: %-48s %-20s %5d %5d\n", pGeoInfo->aIspRecords[uIsp].strOrg, pGeoInfo->aIspRecords[uIsp].strCountry, pGeoInfo->aIspRecords[uIsp].uIspCount, pGeoInfo->aIspRecords[uIsp].uIspDiscs);
        }
    }
    // sum up the rest in a single "other" report
    if (uIsp < pGeoInfo->uNumIsps)
    {
        uint32_t uIspOtherCount, uIspOtherDiscs;
        for (uIspOtherCount = uIspOtherDiscs = 0; uIsp < pGeoInfo->uNumIsps; uIsp += 1)
        {
            uIspOtherCount += pGeoInfo->aIspRecords[uIsp].uIspCount;
            uIspOtherDiscs += pGeoInfo->aIspRecords[uIsp].uIspDiscs;
        }
        if (!bDiscs)
        {
            ZPrintf("logparser: %-48s %-20s %5d\n", "Other", "-----", uIspOtherCount);
        }
        else
        {
            ZPrintf("logparser: %-48s %-20s %5d %5d\n", "Other", "-----", uIspOtherCount, uIspOtherDiscs);
        }
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserClientListPopulation

    \Description
        Calculate population at the time of an event for ISP and ASN lists
       
    \Input *pApp            - parser app state
    \Input *pClientList     - list of clients on server(s)
    \Input uNumClients      - number of clients
    \Input *pStallRecords   - stall/disconnect records for current event
    \Input uNumStalls       - number of stalls/disconnects for current event

    \Version 08/22/2020 (jbrookes) 
*/
/********************************************************************************F*/
static void _LogParserClientListPopulation(LogParserAppT *pApp, LogParserClientInfoT *pClientList, uint32_t uNumClients, LogParserStallRecordT *pStallRecords, uint32_t uNumStalls)
{
    LogParserStallRecordT *pPopulRecords, *pStallRecord;
    LogParserGeoInfoT GeoInfo, *pGeoInfo = &GeoInfo;
    uint32_t uClient, uStall, uAsnIndex, uIspIndex;
    uint32_t uAsn, uIsp;

    ZPrintf("logparser: population report (%d clients)\n", uNumClients);
    // allocate a stall record list big enough to hold all clients for geoip lookup
    if ((pPopulRecords = ZMemAlloc(sizeof(*pPopulRecords)*uNumClients)) == NULL)
    {
        return;
    }

    ds_memclr(&GeoInfo, sizeof(GeoInfo));
    ds_memclr(pPopulRecords, sizeof(*pPopulRecords)*uNumClients);

    // fill in client addresses and do the lookup
    for (uClient = 0; uClient < uNumClients; uClient += 1)
    {
        pPopulRecords[uClient].uAddr = pClientList[uClient].uAddr;
        _LogParserAddrGeoIPRecordLookup(pApp, pPopulRecords[uClient].uAddr, &pPopulRecords[uClient]);
        _LogParserAddrGeoIPOrgLookup(pApp, pPopulRecords[uClient].uAddr, &pPopulRecords[uClient]);
        _LogParserAddrGeoIPAsnLookup(pApp, pPopulRecords[uClient].uAddr, &pPopulRecords[uClient]);
        // update asn list
        pGeoInfo->uNumAsns += _LogParserNumberListInsert(pGeoInfo->aAsnList, pGeoInfo->uNumAsns, MAX_ASNS, pPopulRecords[uClient].uAsn, FALSE, &uAsnIndex);
        // update asn count
        pGeoInfo->aAsnCounts[uAsnIndex] += 1;
        // save an address for later lookup
        pGeoInfo->aAsnAddrs[uAsnIndex] = pPopulRecords[uClient].uAddr;
        // update isp list
        pGeoInfo->uNumIsps += _LogParserIspListInsert(pGeoInfo->aIspRecords, pGeoInfo->uNumIsps, MAX_ISPS, pPopulRecords[uClient].strOrg, pPopulRecords[uClient].strCountry, FALSE, &uIspIndex);
        // update isp count
        pGeoInfo->aIspRecords[uIspIndex].uIspCount += 1;
    }

    // add disconnects to asn list
    for (uStall = 0; uStall < uNumStalls; uStall += 1)
    {
        pStallRecord = pStallRecords+uStall;
        // search asn list for matching ASK
        for (uAsn = 0; uAsn < pGeoInfo->uNumAsns; uAsn += 1)
        {
            if (pGeoInfo->aAsnList[uAsn] == pStallRecord->uAsn)
            {
                pGeoInfo->aAsnDiscs[uAsn] += 1;
                break;
            }
        }
    }

    // add disconnects to isp list
    for (uStall = 0; uStall < uNumStalls; uStall += 1)
    {
        pStallRecord = pStallRecords+uStall;
        // search asn list for matching ASK
        for (uIsp = 0; uIsp < pGeoInfo->uNumIsps; uIsp += 1)
        {
            if (!strcmp(pGeoInfo->aIspRecords[uIsp].strOrg, pStallRecord->strOrg))
            {
                pGeoInfo->aIspRecords[uIsp].uIspDiscs += 1;
                break;
            }
        }
    }

    // log ASN list
    _LogParserEventAsnLog(pApp, &GeoInfo, pApp->uMinAsnReport, TRUE);

    // log ISP list
    _LogParserEventIspLog(pApp, &GeoInfo, pApp->uMinIspReport, TRUE);

    // blank line before event list
    ZPrintf("\n");

    ZMemFree(pPopulRecords);
}

#if defined(LOGPARSE_GEOIP)
/*F********************************************************************************/
/*!
    \Function _LogParserEventLogEntry

    \Description
        Log a GeoIP event entry
       
    \Input *pApp            - parser app state
    \Input *pOutBuf         - [out] buffer to store formatted output
    \Input iBufLen          - length of output buffer
    \Input iOffset          - offset in output buffer
    \Input *pFormat         - format string to use
    \Input *pEntry          - entry string to write

    \Version 07/20/2020 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LogParserEventLogEntry(LogParserAppT *pApp, char *pOutBuf, int32_t iBufLen, int32_t iOffset, const char *pFormat, const char *pEntry)
{
    const char *pFmt = !pApp->bCsvOutput ? pFormat : (iOffset == 0) ? "%s" : ",%s";
    return(ds_snzprintf(pOutBuf+iOffset, iBufLen-iOffset, pFmt, pEntry));
}
#endif

#if defined(LOGPARSE_GEOIP)
/*F********************************************************************************/
/*!
    \Function _LogParserEventLog

    \Description
        Log an event to output
       
    \Input *pApp            - parser app state
    \Input *pLogParse       - logparse to process
    \Input uNumAddrs        - number of addresses in stall event
    \Input *pStallRecords   - stall records for event
    \Input *pGeoInfo        - GeoIP info to fill in
    \Input *fImpact         - impact array

    \Notes
        Filter format for event output:
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        Timestamp   Duration    Client ID   IP address  Lat/Long  CenterDist  ASN  ASN Name Country  Region  City  Org  Valid
        1           1           1           1           1         1           1    1        1        1       1     1    1

        For the filter string, an x represents the feature being enabled, an o means
        it is disabled.  For example:
        xxxxxxxxxxxxx - all enabled
        ooooooooooooo - all disabled
        oooxooxxxxxxo - addr, asn, asn name, country, region, city, org enabled, Valid disabled

    \Version 07/06/2020 (jbrookes) Split from _LogParserStallEventProcess
*/
/********************************************************************************F*/
static void _LogParserEventLog(LogParserAppT *pApp, LogParseRefT *pLogParse, uint32_t uNumAddrs, const LogParserStallRecordT *pStallRecords, const LogParserGeoInfoT *pGeoInfo, const float *fImpact, uint8_t bStallEvent)
{
    char strAddr[16];
    uint32_t uAddr; 

    if ((uNumAddrs == 0) || (pApp->pGeoIP == NULL))
    {
        return;
    }

    // log geoip-derived information
    for (uAddr = 0; uAddr < uNumAddrs; uAddr += 1)
    {
        if ((pStallRecords[uAddr].bValid) && pStallRecords[uAddr].uAddr != 0)
        {
            char strLatLon[16], strRadius[12], strTime[32], strTemp[32], *pTemp;
            char strLine[256];
            int32_t iOffset, iBufLen = sizeof(strLine);

            ds_snzprintf(strLatLon, sizeof(strLatLon), "[%3.2f,%3.2f]", pStallRecords[uAddr].fLatitude, pStallRecords[uAddr].fLongitude);
            ds_snzprintf(strRadius, sizeof(strRadius), "%.2fkm", pStallRecords[uAddr].fDist);

            strLine[0] = 0;
            iOffset = 0;

            if (pApp->strOutFilter[0] == 'x')
            {
                ds_secstostrms(pStallRecords[uAddr].uTime, TIMETOSTRING_CONVERSION_ISO_8601, TRUE, TRUE, strTime, sizeof(strTime));
                if (pApp->bCsvOutput && ((pTemp = strchr(strTime, 'T')) != NULL))
                {
                    *pTemp = ',';
                }
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %-25s", strTime);
            }
            if ((pApp->strOutFilter[1] == 'x') && bStallEvent)
            {
                ds_snzprintf(strTime, sizeof(strTime), "%5dms", pStallRecords[uAddr].uDuration);
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %s", strTime);
            }
            if (pApp->strOutFilter[2] == 'x')
            {
                ds_snzprintf(strTemp, sizeof(strTemp), "%08x", pStallRecords[uAddr].uClientId);
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %s", strTemp);
            }
            if (pApp->strOutFilter[3] == 'x')
            {
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %-16s", _LogParserFormatAddress(pApp, pStallRecords[uAddr].uAddr, strAddr, sizeof(strAddr)));
            }
            if (pApp->strOutFilter[4] == 'x')
            {
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %16s", strLatLon);
            }
            if (pApp->strOutFilter[5] == 'x')
            {
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %10s", strRadius);
            }
            if (pApp->strOutFilter[6] == 'x')
            {
                ds_snzprintf(strTemp, sizeof(strTemp), "%u", pStallRecords[uAddr].uAsn);
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %-10s", strTemp);
            }
            if (pApp->strOutFilter[7] == 'x')
            {
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %-48s", pStallRecords[uAddr].strAsnOrg);
            }
            if (pApp->strOutFilter[8] == 'x')
            {
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %-20s", pStallRecords[uAddr].strCountry);
            }
            if (pApp->strOutFilter[9] == 'x')
            {
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %-6s", pStallRecords[uAddr].strRegion);
            }
            if (pApp->strOutFilter[10] == 'x')
            {
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %-22s", pStallRecords[uAddr].strCity);
            }
            if (pApp->strOutFilter[11] == 'x')
            {
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %-20s", pStallRecords[uAddr].strOrg);
            }
            if (pApp->strOutFilter[12] == 'x')
            {
                iOffset += _LogParserEventLogEntry(pApp, strLine, iBufLen, iOffset, " %s", pStallRecords[uAddr].bValid ? "" : "(invalid)");
            }

            ZPrintf(!pApp->bCsvOutput ? "logparser: %s\n" : "%s\n", strLine);
        }
    }

    // log center of event if we are logging lat/long
    if (pApp->strOutFilter[4] == 'x')
    {
        ZPrintf("logparser: center of event is [%.2f,%.2f] with radius of %.2fkm and average distance of %.2fkm, stddev=%.2f\n",
            fImpact[0], fImpact[1], fImpact[2], fImpact[3], fImpact[4]);
    }
}
#endif

/*F********************************************************************************/
/*!
    \Function _LogParserStallEventProcess

    \Description
        VoipServer-specific stall processing logic

    \Input *pApp        - parser app state
    \Input *pLogParse   - logparse to process

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
static void _LogParserStallEventProcess(LogParserAppT *pApp, LogParseRefT *pLogParse)
{
    uint32_t uLine, uStall, uStartLine, uNumStalls;
    uint32_t uDurationSum;
    char strLineBuf[256], strTimeBegin[32], strTimeEnd[32], strAddr[16], *pDuration, *pClientId;
    uint32_t uAddr, uNumAddrs, uNumAddrsTotal;
    #if defined(LOGPARSE_GEOIP)
    float fImpact[LOGPARSE_NUMSTAT];
    #endif

    if (pLogParse->uNumLineEntries == 0)
    {
        return;
    }

    // clear lists
    ds_memclr(&pApp->GeoInfo, sizeof(pApp->GeoInfo));

    ZPrintf("logparser: searching for stall events from %s through %s\n", ds_secstostrms(pLogParse->pLineBuf[0].uLineTime, TIMETOSTRING_CONVERSION_ISO_8601, TRUE, FALSE, strTimeBegin, sizeof(strTimeBegin)),
        ds_secstostrms(pLogParse->pLineBuf[pLogParse->uNumLineEntries-1].uLineTime, TIMETOSTRING_CONVERSION_ISO_8601, TRUE, FALSE, strTimeEnd, sizeof(strTimeEnd)));

    // process stall log
    for (uLine = 0, uNumStalls = 0, uNumAddrsTotal = 0; uLine < pLogParse->uNumLineEntries; uLine += 1)
    {
        /* look for stall events; a stall event is defined as a set of stalls
           happening within a small (default 250ms) window of one another */
        for (uStall = 0, uDurationSum = 0, uStartLine = uLine, uNumAddrs = 0; ; uLine += 1)
        {
            LogParseGetLine(&pLogParse->pLineBuf[uLine], strLineBuf, sizeof(strLineBuf));
            if ((pDuration = strstr(strLineBuf, "voipserver: ")) != NULL)
            {
                uint32_t uClientId = 0, uDuration = strtol(pDuration+12, NULL, 10);
                uDurationSum += uDuration;

                // extract client id
                if ((pClientId = strstr(strLineBuf, "clientid=0x")) != NULL)
                {
                    uClientId = strtol(pClientId+11, NULL, 16);
                }

                // try and extract IP address
                if (((pDuration = strchr(pDuration, '(')) != NULL) && ((uAddr = SocketInTextGetAddr(pDuration+1)) != 0))
                {
                    //$$todo - understand why some stall durations are logged that have very large durations
                    if (uDuration > 999999)
                    {
                        continue;
                    }
                    ds_memclr(&pApp->StallRecords[uStall], sizeof(pApp->StallRecords[0]));
                    pApp->StallRecords[uStall].uClientId = uClientId;
                    pApp->StallRecords[uStall].uAddr = uAddr;
                    pApp->StallRecords[uStall].uTime = pLogParse->pLineBuf[uLine].uLineTime;
                    pApp->StallRecords[uStall].uDuration = uDuration;
                    uStall += 1;
                }
            }
            if (((pLogParse->pLineBuf[uLine+1].uLineTime - pLogParse->pLineBuf[uLine].uLineTime)) >= pApp->uStallWindow)
            {
                break;
            }
        }

        // skip events that are too small
        if (uStall <= pApp->uStallThreshold)
        {
            continue;
        }

        uNumAddrs = uStall;

        // geoip lookup and assorted processing
        #if defined(LOGPARSE_GEOIP)
        uStall = _LogParserStallEventGeoIPProcess(pApp, pLogParse, uNumAddrs, pApp->StallRecords, &pApp->GeoInfo, fImpact);

        // skip events that are too small; already checked this but we check it again after possible ISP filtering
        if ((uStall <= pApp->uStallThreshold) || (uNumAddrs == 0))
        {
            continue;
        }
        #endif

        // sort by address if desired
        if (pApp->bAddrSort)
        {
            qsort(pApp->StallRecords, uStall, sizeof(pApp->StallRecords[0]), _LogParserStallEventSort);
        }

        // count and log stall
        uNumStalls += 1;
        ZPrintf("logparser: [%3d] %3d-client stall event detected at %s (%3dms window, %dms avg duration", uNumStalls, uStall,
            ds_secstostrms(pLogParse->pLineBuf[uStartLine].uLineTime, TIMETOSTRING_CONVERSION_ISO_8601, TRUE, FALSE, strTimeBegin, sizeof(strTimeBegin)),
            (uint32_t)(pLogParse->pLineBuf[uLine].uLineTime - pLogParse->pLineBuf[uStartLine].uLineTime),
            uDurationSum / ((uLine - uStartLine + 1)));
        if (uNumAddrs > 0)
        {
            ZPrintf(", addrs=%s", _LogParserFormatAddress(pApp, pApp->StallRecords[0].uAddr, strAddr, sizeof(strAddr)));
            for (uAddr = 1; uAddr < uNumAddrs; uAddr += 1)
            {
                ZPrintf(",%s", _LogParserFormatAddress(pApp, pApp->StallRecords[uAddr].uAddr, strAddr, sizeof(strAddr)));
            }
        }
        ZPrintf(")\n");

        // add to addr total
        uNumAddrsTotal += uNumAddrs;

        #if defined(LOGPARSE_GEOIP)
        if (!pApp->bHeadersOnly)
        {
            _LogParserEventLog(pApp, pLogParse, uNumAddrs, pApp->StallRecords, &pApp->GeoInfo, fImpact, TRUE);
        }
        #endif
    }

    ZPrintf("logparser: %d total stall events detected (%d clients)\n", uNumStalls, uNumAddrsTotal);

    #if defined(LOGPARSE_GEOIP)
    // log ASN list
    _LogParserEventAsnLog(pApp, &pApp->GeoInfo, pApp->uMinAsnReport, FALSE);
    // log ISP list
    _LogParserEventIspLog(pApp, &pApp->GeoInfo, pApp->uMinIspReport, FALSE);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _LogParserDiscEventProcess

    \Description
        VoipServer-specific disconnect processing logic

    \Input *pApp        - parser app state
    \Input *pLogParse   - logparse to process

    \Notes
        Assumes only disconnect entries are present

    \Version 07/08/2030 (jbrookes)
*/
/********************************************************************************F*/
static void _LogParserDiscEventProcess(LogParserAppT *pApp, LogParseRefT *pLogParse)
{
    uint32_t uLine, uDisc, uDiscCt, uStartLine, uNumDiscs;
    char strLineBuf[256], strTimeBegin[32], strTimeEnd[32], *pReason;
    uint32_t aDiscReasons[32], uNumClients, uNumClientsTotal;
    #if defined(LOGPARSE_GEOIP)
    float fImpact[LOGPARSE_NUMSTAT];
    #endif
    uint64_t uTimeBegin;

    if (pLogParse->uNumLineEntries == 0)
    {
        return;
    }

    // clear lists
    ds_memclr(&pApp->GeoInfo, sizeof(pApp->GeoInfo));
    ds_memclr(&pApp->ClientList, sizeof(pApp->ClientList));

    // calculate beginning time from event range, if present; if not, use first logparse time
    if (pApp->strStartTime[0] != '\0')
    {
        uTimeBegin = LogParseDatetime(pApp->strStartTime);
        ds_strnzcpy(strTimeBegin, pApp->strStartTime, sizeof(strTimeBegin));
    }
    else
    {
        uTimeBegin = pLogParse->pLineBuf[0].uLineTime;
        ds_secstostrms(pLogParse->pLineBuf[0].uLineTime, TIMETOSTRING_CONVERSION_ISO_8601, TRUE, FALSE, strTimeBegin, sizeof(strTimeBegin));
    }

    ZPrintf("logparser: searching for disc events from %s through %s\n", strTimeBegin,
        ds_secstostrms(pLogParse->pLineBuf[pLogParse->uNumLineEntries-1].uLineTime, TIMETOSTRING_CONVERSION_ISO_8601, TRUE, FALSE, strTimeEnd, sizeof(strTimeEnd)));

    // process stall log
    for (uLine = 0, uNumDiscs = 0, uNumClients = 0, uNumClientsTotal = 0; uLine < pLogParse->uNumLineEntries; uLine += 1)
    {
        // update client list
        uNumClients = _LogParserClientListUpdate(pApp, pApp->ClientList, uNumClients, pLogParse->pLineBuf[uLine].uLineTime);

        /* look for disconnect events; a disconnect event is defined as a set of disconnections
           happening within a small (default 250ms) window of one another */
        for (uDisc = 0, uStartLine = uLine; ; uLine += 1)
        {
            LogParseGetLine(&pLogParse->pLineBuf[uLine], strLineBuf, sizeof(strLineBuf));

            // check for client add
            if ((pReason = strstr(strLineBuf, "matching")) != NULL)
            {
                // 2020/07/07 04:12:29.474 CI |  voipserver: matching clientid=0xda4823dc to address 1.2.3.4:45164 (virtual address 1.3.248.143)
                // parse client id and addr
                uint32_t uClientIdent = (uint32_t)strtoll(pReason+18, NULL, 16);
                uint32_t uClientAddr = SocketInTextGetAddr(pReason+40);
                // add client to client list
                uNumClients += _LogParserClientListAdd(pApp, pApp->ClientList, uNumClients, sizeof(pApp->ClientList)/sizeof(pApp->ClientList[0]), uClientIdent, uClientAddr, pLogParse->pLineBuf[uLine].uLineTime, NULL);
            }
            // check for client removal
            else if ((pReason = strstr(strLineBuf, "processing 'dusr'")) != NULL)
            {
                // 2020/07/07 04:12:07.841 CI |  voipgameserver: [gs025] processing 'dusr' command clientId=0x9e118853 reason=0x00070000, ( 2)
                // parse client info and disconnect reason
                uint32_t uClientIdent = (uint32_t)strtoll(pReason+35, NULL, 16);
                uint32_t uReason = (strtol(pReason+53, NULL, 16) >> 16) & 31;
                uint32_t uClientIndex;
                LogParserClientInfoT *pClientInfo = (LogParserClientInfoT *)_LogParserClientListGet(pApp, pApp->ClientList, uNumClients, uClientIdent, &uClientIndex);

                if (pClientInfo != NULL)
                {
                    // add disconnect record if the disconnect reason was less than PLAYER_LEFT (PLAYER_JOIN_TIMEOUT, PLAYER_CONN_POOR_QUALITY, PLAYER_CONN_LOST, BLAZESERVER_CONN_LOST, MIGRATION_FAILED, GAME_DESTROYED, GAME_ENDED)
                    if (uReason < 7)
                    {
                        ds_memclr(&pApp->StallRecords[uDisc], sizeof(pApp->StallRecords[0]));
                        pApp->StallRecords[uDisc].uClientId = uClientIdent;
                        pApp->StallRecords[uDisc].uAddr = pClientInfo->uAddr;
                        pApp->StallRecords[uDisc].uTime = pLogParse->pLineBuf[uLine].uLineTime;
                        pApp->StallRecords[uDisc].uDuration = uReason;
                        uDisc += 1;
                    }

                    // mark client as disconnected for removal
                    pClientInfo->bDisc = TRUE;
                }
                else
                {
                    NetPrintf(("logparser: couldn't find reference for clientid 0x%08x\n", uClientIdent));
                }
            }
            if (((pLogParse->pLineBuf[uLine+1].uLineTime - pLogParse->pLineBuf[uLine].uLineTime)) >= pApp->uStallWindow)
            {
                break;
            }
        }

        // skip events if they're before our begin time
        if (pLogParse->pLineBuf[uLine].uLineTime < uTimeBegin)
        {
            continue;
        }

        // skip events that are too small
        if (uDisc <= pApp->uStallThreshold)
        {
            continue;
        }

        // sort by address if desired
        if (pApp->bAddrSort)
        {
            qsort(pApp->StallRecords, uDisc, sizeof(pApp->StallRecords[0]), _LogParserStallEventSort);
        }

        // geoip lookup and assorted processing
        #if defined(LOGPARSE_GEOIP)
        uDisc = _LogParserStallEventGeoIPProcess(pApp, pLogParse, uDisc, pApp->StallRecords, &pApp->GeoInfo, fImpact);

        // skip events that are too small; already checked this but we check it again after possible ISP filtering
        if (uDisc <= pApp->uStallThreshold)
        {
            continue;
        }
        #endif

        // calculate reason percentages
        ds_memclr(aDiscReasons, sizeof(aDiscReasons));
        for (uDiscCt = 0; uDiscCt < uDisc; uDiscCt += 1)
        {
            if (pApp->StallRecords[uDiscCt].uAddr == 0)
            {
                continue;
            }
            aDiscReasons[pApp->StallRecords[uDiscCt].uDuration] += 1;
            uNumClientsTotal += 1;
        }

        // count and log stall
        uNumDiscs += 1;
        if (!pApp->bCsvOutput)
        {
            float fPlayerDiscPct = (float)aDiscReasons[2]/(float)uDisc;
            float fBlazeDiscPct = (float)aDiscReasons[3]/(float)uDisc;
            float fOtherDiscPct = (float)(uDisc-(aDiscReasons[2]+aDiscReasons[3]))/(float)uDisc;

            ZPrintf("logparser: [%3d] %4d-client (%5d total clients) disc event detected at %s (%4dms window) - PLAYER_CONN_LOST=%1.3f%%, BLAZESERVER_CONN_LOST=%1.3f%%, OTHER=%1.3f%%\n", uNumDiscs, uDisc, uNumClients,
                ds_secstostrms(pLogParse->pLineBuf[uStartLine].uLineTime, TIMETOSTRING_CONVERSION_ISO_8601, TRUE, FALSE, strTimeBegin, sizeof(strTimeBegin)),
                (uint32_t)(pLogParse->pLineBuf[uLine].uLineTime - pLogParse->pLineBuf[uStartLine].uLineTime),
                fPlayerDiscPct, fBlazeDiscPct, fOtherDiscPct);

            if (pApp->iEventPopulationIdx == (signed)uNumDiscs)
            {
                _LogParserClientListPopulation(pApp, pApp->ClientList, uNumClients, pApp->StallRecords, uDisc);
            }            
        }

        #if defined(LOGPARSE_GEOIP)
        if (!pApp->bHeadersOnly)
        {
            _LogParserEventLog(pApp, pLogParse, uDisc, pApp->StallRecords, &pApp->GeoInfo, fImpact, FALSE);
        }
        #endif
    }

    ZPrintf("logparser: %d total disconnect events detected (%d clients)\n", uNumDiscs, uNumClientsTotal);

    #if defined(LOGPARSE_GEOIP)
    // log ASN list
    _LogParserEventAsnLog(pApp, &pApp->GeoInfo, pApp->uMinAsnReport, FALSE);
    // log ISP list
    _LogParserEventIspLog(pApp, &pApp->GeoInfo, pApp->uMinIspReport, FALSE);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _LogParserCommandlineProcess

    \Description
        Process logparser commmandline arguments

    \Input *pApp        - application state
    \Input iArgc        - number of commandline arguments
    \Input *pArgv[]     - commandline arguments

    \Version 09/29/2013 (jbrookes) Split from main
*/
/********************************************************************************F*/
static int32_t _LogParserCommandlineProcess(LogParserAppT *pApp, int32_t iArgc, const char *pArgv[])
{
    int32_t iStartIndex;

    // usage
    if (iArgc < 2)
    {
        ZPrintf("%s: usage %s [options] [logfile1] <logfile2> ... <logfileN>\n", pArgv[0]);
        ZPrintf("    -a: specify geoip ASN file (csv only)\n");
        ZPrintf("    -A: anonymize IP address in log output\n");
        ZPrintf("    -c: output event entries as csv with no event headers\n");
        ZPrintf("    -d: filter for disconnect events\n");
        ZPrintf("    -D: enable disconnect event processing\n");
        ZPrintf("    -e: set event threshold\n");
        ZPrintf("    -E: event address sorting\n");
        ZPrintf("    -f: specify file holding newline-separated list of logfiles\n");
        ZPrintf("    -F: set event output filter\n");
        ZPrintf("    -g: specify geoip city data file\n");
        ZPrintf("    -G: specify geoip org data file\n");
        ZPrintf("    -h: output event headers only\n");
        ZPrintf("    -i: specify ISP selection filter\n");
        ZPrintf("    -I: specify ISP exclusion filter\n");
        ZPrintf("    -n: specify ASN selection filter (expects ASN number)\n");
        ZPrintf("    -N: specify ASN exclusion filter (expects ASN number)\n");
        ZPrintf("    -o: set output filename\n");
        ZPrintf("    -O: enable outliers\n");
        ZPrintf("    -P: print extended population analysis for specified event");
        ZPrintf("    -r: enable ASN report; argument specifies min number of instances to be displayed");
        ZPrintf("    -R: enable ISP report; argument specifies min number of instances to be displayed");
        ZPrintf("    -s: filter for stall events\n");
        ZPrintf("    -S: enable stall event processing\n");
        ZPrintf("    -t: specify a timerange [YYYY-MM-DDTHH:MM:SS,YYYY-MM-DDTHH:MM:SS]\n");
        ZPrintf("    -v: enable log entry validation\n");
        ZPrintf("    -w: specify a timing window for stall event identification (default 250ms)\n");
        ZPrintf("    -0: dynamic output (to cmd window, not a file)\n");
        return(0);
    }

    // check for command-line args
    for (iStartIndex = 1; iStartIndex < iArgc; iStartIndex += 1)
    {
        // done with args?
        if (pArgv[iStartIndex][0] != '-')
        {
            break;
        }

        if (pArgv[iStartIndex][1] == 'A')
        {
            pApp->bAnonymize = TRUE;
            ZPrintf("logparser: anonymize enabled\n");
        }
        else if ((pArgv[iStartIndex][1] == 'a') && (iStartIndex < iArgc))
        {
            iStartIndex += 1;
            ds_strnzcpy(pApp->strAsnFile, pArgv[iStartIndex], sizeof(pApp->strAsnFile));
            ZPrintf("logparser: asnfile=%s\n", pApp->strAsnFile);
        }
        else if (pArgv[iStartIndex][1] == 'c')
        {
            pApp->bCsvOutput = TRUE;
            ZPrintf("logparser: event output set to csv format\n");
        }
        else if (pArgv[iStartIndex][1] == 'd')
        {
            pApp->ExtractList.pStrings[pApp->ExtractList.uNumStrings++] = "processing 'dusr'";
            pApp->ExtractList.pStrings[pApp->ExtractList.uNumStrings++] = "to address";
            ZPrintf("logparser: filtering for disconnect events\n");
        }
        else if (pArgv[iStartIndex][1] == 'D')
        {
            pApp->bDisconnects = TRUE;
            ZPrintf("logparser: processing disconnect events\n");
        }
        else if ((pArgv[iStartIndex][1] == 'e') && (iStartIndex < iArgc))
        {
            iStartIndex += 1;
            pApp->uStallThreshold = strtol(pArgv[iStartIndex], NULL, 10);
            ZPrintf("logparser: event threshold=%d\n", pApp->uStallThreshold);
        }
        else if (pArgv[iStartIndex][1] == 'E')
        {
            pApp->bAddrSort = TRUE;
            ZPrintf("logparser: event address sorting enabled\n");
        }
        else if ((pArgv[iStartIndex][1] == 'f') && (iStartIndex < iArgc))
        {
            iStartIndex += 1;
            ds_strnzcpy(pApp->strInpFile, pArgv[iStartIndex], sizeof(pApp->strInpFile));
            ZPrintf("logparser: inpfile=%s\n", pApp->strInpFile);
        }
        else if ((pArgv[iStartIndex][1] == 'F') && (iStartIndex < iArgc))
        {
            iStartIndex += 1;
            ds_strnzcpy(pApp->strOutFilter, pArgv[iStartIndex], sizeof(pApp->strOutFilter));
            ZPrintf("logparser: outfilter=%s\n", pApp->strOutFilter);
        }
        else if ((pArgv[iStartIndex][1] == 'g') && (iStartIndex < iArgc))
        {
            iStartIndex += 1;
            ds_strnzcpy(pApp->strGeoFile, pArgv[iStartIndex], sizeof(pApp->strGeoFile));
            ZPrintf("logparser: geofile=%s\n", pApp->strGeoFile);
        }
        else if ((pArgv[iStartIndex][1] == 'G') && (iStartIndex < iArgc))
        {
            iStartIndex += 1;
            ds_strnzcpy(pApp->strOrgFile, pArgv[iStartIndex], sizeof(pApp->strOrgFile));
            ZPrintf("logparser: orgfile=%s\n", pApp->strOrgFile);
        }
        else if (pArgv[iStartIndex][1] == 'h')
        {
            pApp->bHeadersOnly = TRUE;
            ZPrintf("logparser: output event headers only\n");
        }
        else if (((pArgv[iStartIndex][1] == 'i') || (pArgv[iStartIndex][1] == 'I')) && (iStartIndex < iArgc))
        {
            pApp->bIspExclude = (pArgv[iStartIndex][1] == 'I') ? TRUE : FALSE;
            iStartIndex += 1;
            ds_strnzcpy(pApp->strIspFilter, pArgv[iStartIndex], sizeof(pApp->strIspFilter));
            ZPrintf("logparser: ispfilter=%s (%s)\n", pApp->strIspFilter, pApp->bIspExclude ? "exclusion" : "selection");
        }
        else if (((pArgv[iStartIndex][1] == 'n') || (pArgv[iStartIndex][1] == 'N')) && (iStartIndex < iArgc))
        {
            pApp->bAsnExclude = (pArgv[iStartIndex][1] == 'N') ? TRUE : FALSE;
            iStartIndex += 1;
            ds_strnzcpy(pApp->strAsnFilter, pArgv[iStartIndex], sizeof(pApp->strAsnFilter));
            ZPrintf("logparser: asnfilter=%s (%s)\n", pApp->strAsnFilter, pApp->bAsnExclude ? "exclusion" : "selection");
        }
        else if ((pArgv[iStartIndex][1] == 'o') && (iStartIndex < iArgc))
        {
            iStartIndex += 1;
            ds_strnzcpy(pApp->strOutFile, pArgv[iStartIndex], sizeof(pApp->strOutFile));
            ZPrintf("logparser: outfile=%s\n", pApp->strOutFile);
        }
        else if (pArgv[iStartIndex][1] == 'O')
        {
            pApp->bOutliers = TRUE;
            ZPrintf("logparser: outliers enabled\n");
        }
        else if ((pArgv[iStartIndex][1] == 'P') && (iStartIndex < iArgc))
        {
            iStartIndex += 1;
            pApp->iEventPopulationIdx = (int16_t)strtol(pArgv[iStartIndex], NULL, 10);
            ZPrintf("logparser: extended population report enabled for event %d\n", pApp->iEventPopulationIdx);
        }
        else if ((pArgv[iStartIndex][1] == 'r') && (iStartIndex < iArgc))
        {
            pApp->bAsnReport = TRUE;
            iStartIndex += 1;
            pApp->uMinAsnReport = (uint8_t)strtol(pArgv[iStartIndex], NULL, 10);
            ZPrintf("logparser: ASN report enabled; threshold=%d\n", pApp->uMinAsnReport);
        }
        else if ((pArgv[iStartIndex][1] == 'R') && (iStartIndex < iArgc))
        {
            pApp->bIspReport = TRUE;
            iStartIndex += 1;
            pApp->uMinIspReport = (uint8_t)strtol(pArgv[iStartIndex], NULL, 10);
            ZPrintf("logparser: ISP report enabled; threshold=%d\n", pApp->uMinIspReport);
        }
        else if ((pArgv[iStartIndex][1] == 's') && (iStartIndex < iArgc))
        {
            pApp->ExtractList.pStrings[pApp->ExtractList.uNumStrings++] = "stall";
            ZPrintf("logparser: filtering for stall events\n");
        }
        else if ((pArgv[iStartIndex][1] == 'S') && (iStartIndex < iArgc))
        {
            pApp->bStalls = TRUE;
            ZPrintf("logparser: processing stall events\n");
        }
        else if ((pArgv[iStartIndex][1] == 't') && (iStartIndex < iArgc))
        {
            char strTimeRange[64], *pTimeStr;
            iStartIndex += 1;
            ds_strnzcpy(strTimeRange, pArgv[iStartIndex], sizeof(strTimeRange));
            if ((pTimeStr = strstr(strTimeRange, ",")) != NULL)
            {
                *pTimeStr++ = '\0';
                ds_strnzcpy(pApp->strStartTime, strTimeRange, sizeof(pApp->strStartTime));
                ds_strnzcpy(pApp->strEndTime, pTimeStr, sizeof(pApp->strEndTime));
                ZPrintf("logparser: timerange=[%s,%s]\n", pApp->strStartTime, pApp->strEndTime);
            }
        }
        else if (pArgv[iStartIndex][1] == 'v')
        {
            pApp->bValidate = TRUE;
            ZPrintf("logparser: line validation enabled\n");
        }
        else if ((pArgv[iStartIndex][1] == 'w') && (iStartIndex < iArgc))
        {
            iStartIndex += 1;
            pApp->uStallWindow = strtol(pArgv[iStartIndex], NULL, 10);
            ZPrintf("logparser: stall window threshold=%d\n", pApp->uStallWindow);
        }
        else if (pArgv[iStartIndex][1] == '0')
        {
            pApp->bDynamicOutput = TRUE;
            ZPrintf("logparser: dynamic output enabled\n");
        }        
    }
    return(iStartIndex);
}

/*F********************************************************************************/
/*!
    \Function main

    \Description
        Main routine for logparser application

    \Input iArgc        - argument count
    \Input *pArgv[]     - argument list

    \Version 10/23/2012 (jbrookes)
*/
/********************************************************************************F*/
int main(int argc, const char *argv[])
{
    LogParserAppT *pApp;
    LogParseRefT *pLogParse;
    uint32_t uStartTick;
    int32_t iStartIndex, iFileSize;
    char strDuration[32];

    // set up zprintf
    #if defined(DIRTYCODE_PC)
    ZPrintfHook(_LogParserPrintfHook, NULL);
    #endif

    // allocate app memory
    if ((pApp = ZMemAlloc(sizeof(*pApp))) == NULL)
    {
        ZPrintf("%s: could not allocate application memory (%d bytes)\n", sizeof(*pApp));
    }

    // init logparser ref state
    ds_memclr(pApp, sizeof(*pApp));
    pApp->uStallThreshold = 2;
    pApp->uStallWindow = 250;
    pApp->iEventPopulationIdx = -1;
    // default output filter to show everything
    ds_strnzcpy(pApp->strOutFilter, "xxxxxxxxxxxxx", sizeof(pApp->strOutFilter));

    // print argument list
    for (iStartIndex = 0; iStartIndex < argc; iStartIndex += 1)
    {
        ZPrintf("%s ", argv[iStartIndex]);
    }
    ZPrintf("\n");

    // parse commandline arguments
    if ((iStartIndex = _LogParserCommandlineProcess(pApp, argc, argv)) == 0)
    {
        return(-1);
    }

    // for NetTick();
    NetLibCreate(0, 0, 0);

    #if defined(LOGPARSE_GEOIP)
    // init GeoIP main database
    if ((pApp->strGeoFile[0] != '\0') && ((pApp->pGeoIP = GeoIP_open(pApp->strGeoFile, GEOIP_MEMORY_CACHE)) == NULL))
    {
        ZPrintf("logparser: failed to load geoip main database '%s'\n", pApp->strGeoFile);
    }
    // init GeoOrg org database
    if ((pApp->strOrgFile[0] != '\0') && ((pApp->pGeoIPOrg = GeoIP_open(pApp->strOrgFile, GEOIP_MEMORY_CACHE)) == NULL))
    {
        ZPrintf("logparser: failed to load geoip org database '%s'\n", pApp->strOrgFile);
    }
    // init GeoOrg asn database (use csv version of asn database)
    if (pApp->strAsnFile[0] != '\0')
    {
        if ((pApp->pGeoIPAsn = ZFileLoad(pApp->strAsnFile, &iFileSize, FALSE)) != NULL)
        {
            pApp->pGeoIPAsnParsed = LogParseCreate(pApp->pGeoIPAsn, iFileSize);
        }
        else
        {
            ZPrintf("logparser: failed to load geoip asn database '%s'\n", pApp->strAsnFile);
        }
    }
    #endif // defined(LOGPARSE_GEOIP)

    // load and consolidate input files from command-line
    ZPrintf("logparser: processing logs\n");
    uStartTick = NetTick();
    pLogParse = (pApp->strInpFile[0] != '\0') ? _LogParserLoadFilesFromInputfile(pApp, pApp->strInpFile, argv, &pApp->ExtractList) : _LogParserLoadFilesFromCmdline(pApp, iStartIndex, argc, argv, &pApp->ExtractList);
    ZPrintf("logparser: processed logs in %s\n", _LogParserGetDuration(strDuration, sizeof(strDuration), NetTickDiff(NetTick(), uStartTick)));

    // validate parse lines
    _LogParseValidate(pApp, pLogParse);

    // process stall events
    if (pLogParse != NULL)
    {
        if (pApp->bStalls)
        {
            ZPrintf("logparser: processing stall events\n");
            uStartTick = NetTick();
            _LogParserStallEventProcess(pApp, pLogParse);
            ZPrintf("\nlogparser: processed stall events in %s\n", _LogParserGetDuration(strDuration, sizeof(strDuration), NetTickDiff(NetTick(), uStartTick)));
        }

        if (pApp->bDisconnects)
        {
            ZPrintf("logparser: processing disconnect events\n", argv[0]);
            uStartTick = NetTick();
            _LogParserDiscEventProcess(pApp, pLogParse);
            ZPrintf("\nlogparser: processed disconnect events in %s\n", _LogParserGetDuration(strDuration, sizeof(strDuration), NetTickDiff(NetTick(), uStartTick)));
        }

        // if we have an output file, save logparse to file
        if (pApp->strOutFile[0] != '\0')
        {
            _LogParserSaveFile(pLogParse, pApp->strOutFile);
        }

        // clean up and exit
        LogParseDestroy(pLogParse);
    }

    // note if client list was full at any point
    if (pApp->iClientListFull != 0)
    {
        ZPrintf("logparser: WARNING: client list max size exceeded; %d clients were not considered for disconnect processing\n", pApp->iClientListFull);
    }

    NetLibDestroy(0);
    return(0);
}

/*F********************************************************************************/
/*!
     \Function DirtyMemAlloc

     \Description
        Required memory allocation function for DirtySock client code.

     \Input iSize               - size of memory to allocate
     \Input iMemModule          - memory module
     \Input iMemGroup           - memory group
     \Input *pMemGroupUserData  - user data associated with memory group

     \Output
        void *                     - pointer to memory, or NULL

     \Version 08/16/2007 (jbrookes)
*/
/********************************************************************************F*/
void *DirtyMemAlloc(int32_t iSize, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    return(malloc(iSize));
}

/*F********************************************************************************/
/*!
     \Function DirtyMemFree

     \Description
        Required memory free function for DirtySock client code.

     \Input *pMem               - pointer to memory to dispose of
     \Input iMemModule          - memory module
     \Input iMemGroup           - memory group
     \Input *pMemGroupUserData  - user data associated with memory group

     \Output
        None.

     \Version 08/16/2007 (jbrookes)
*/
/********************************************************************************F*/
void DirtyMemFree(void *pMem, int32_t iMemModule, int32_t iMemGroup, void *pMemGroupUserData)
{
    free(pMem);
}
