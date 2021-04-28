/*H********************************************************************************/
/*!
    \File dirtycast.c

    \Description
        Shared routines for DirtyCast server.

    \Copyright
        Copyright (c) 2007 Electronic Arts Inc.

    \Version 08/03/2007 (jbrookes) First Version
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#ifdef WIN32
 /* no function prototype given : converting from () to (void) */
 #pragma warning(push, 0)
 #include <Ws2tcpip.h>
 #include <windows.h>
 #pragma warning(pop)
#else
 #include <unistd.h>
 #include <errno.h>
 #include <netdb.h>
 #include <sys/inotify.h>
 #include <sys/stat.h>
 #include <sys/sysinfo.h>
 #include <sys/time.h>
 #include <sys/times.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "DirtySDK/dirtysock/dirtylib.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "LegacyDirtySDK/util/tagfield.h"
#include "dirtycast.h"
#include "logger.h"

/*** Defines **********************************************************************/

#define LOG_SEPARATOR_STRING "\026"

/*** Type Definitions *************************************************************/

// Logging levels
typedef enum Level { LEVEL_ERR = 1, LEVEL_WARN = 2, LEVEL_INFO = 4, LEVEL_DBG = 8 } Level;

// Logging categories
typedef enum Category
{
    PLACEHOLDER_AT  = 0,   // @:
    ABUSE           = 1,   // A: Abuse reports
    MSTR            = 2,   // B: Master server
    CORE            = 3,   // C: Core server
    AUDIT           = 4,   // D: Aries session audit
    EXT             = 5,   // E: Extension server
    PLACEHOLDER_F   = 6,   // F:
    GAME            = 7,   // G: Game result processing
    PERSIST         = 8,   // H: Persistent game services
    LEAGUE          = 9,   // I: Interactive leagues
    PLACEHOLDER_J   = 10,  // J:
    CUST            = 11,  // K: mas/slv/tourney-cust module logging
    LOGIN           = 12,  // L: User authentication
    METRIC          = 13,  // M: XML server metrics
    PLACEHOLDER_N   = 14,  // N:
    PLACEHOLDER_O   = 15,  // O:
    PLACEHOLDER_P   = 16,  // P:
    PLACEHOLDER_Q   = 17,  // Q:
    RDIR            = 18,  // R: Redirector
    STATUS          = 19,  // S: Server status
    TOUR            = 20,  // T: Tournament server
    PLACEHOLDER_U   = 21,  // U:
    SLAV            = 22,  // V: Slave
    PLACEHOLDER_W   = 23,  // W:
    XMLGAME         = 24,  // X: XML game reports
    TELEMETRY       = 25,  // Y: Client telemetry data
    PLACEHOLDER_Z   = 26,  // Z:
    PLACEHOLDER_0   = 27,  // 0:
    PLACEHOLDER_1   = 28,  // 1:
    PLACEHOLDER_2   = 29,  // 2:
    PLACEHOLDER_3   = 30,  // 3:
    PLACEHOLDER_4   = 31,  // 4:
} Category;

//! logging config
typedef struct LogConfigT
{
    Level eLevel;           //<! configured level to log

    uint32_t aDbgLevels[32];//<! level for debug out for each of the categories
    uint32_t uCategoryMask; //<! mask of categories to log

    char strContext[16];    //!< optional context we log

    uint8_t bDualLogging;   //<! If TRUE, log to both file and stdout
    uint8_t _pad[3];        //<! padding
} LogConfigT;

/*** Variables ********************************************************************/

//! singleton logger ref
static LoggerRefT *_DirtyCast_pLoggerRef = NULL;
static LogConfigT  _DirtyCast_LogConfig = { LEVEL_INFO, { 0 }, 0xffffffff, { 0 }, FALSE, { 0 } };

//! debug filter table
static DirtyCastFilterEntryT _DbgFilterTable[] =
{
#if defined(DIRTYCODE_PC)
    { DIRTYCAST_DBGLVL_RECURRING, "dirtynetwin: connection open", 0    },
    { DIRTYCAST_DBGLVL_RECURRING, "dirtynetwin: connection closed", 0  },
    { DIRTYCAST_DBGLVL_HIGHEST, "dirtynetwin: unhandled global SocketInfo() selector 'hwid'", 0 },
#endif
#if defined(DIRTYCODE_LINUX)
    { DIRTYCAST_DBGLVL_RECURRING, "dirtynetunix: connection open", 0   },
    { DIRTYCAST_DBGLVL_RECURRING, "dirtynetunix: connection closed", 0 },
    { DIRTYCAST_DBGLVL_HIGHEST, "dirtynetunix: unhandled control option 'spam'", 0 },
    { DIRTYCAST_DBGLVL_HIGHEST, "dirtynetunix: unhandled global SocketInfo() selector 'hwid'", 0 },
#endif
    { 0, NULL, 0                                                       }
};

static const char _DirtyCast_strCategories[] = "@ABCDEFGHIJKLMNOPQRSTUVWXYZ01234";

/*** Private Functions ************************************************************/

/*F********************************************************************************/
/*!
    \Function _DirtyCastLog

    \Description
        In charge of doing the actual logging when called by the DirtyCastLogPrintf
        functions

    \Input  eCategory   - category we are logging to
    \Input  eLevel      - level we are printing to
    \Input  *pText      - text to log
 
    \Version    06/17/2016 (eesponda)
*/
/********************************************************************************F*/
static void _DirtyCastLog(Category eCategory, Level eLevel, const char *pText)
{
    char strBuffer[2048];
    int32_t iBufLen = 2048, iBufOff = 0;
    char cLevelChar;

    switch (eLevel)
    {
        case LEVEL_ERR:
            cLevelChar = 'E';
            break;

        case LEVEL_WARN:
            cLevelChar = 'W';
            break;

        case LEVEL_INFO:
            cLevelChar = 'I';
            break;

        case LEVEL_DBG:
            cLevelChar = 'D';
            break;

        default:
            cLevelChar = '*';
            break;
    }

    if (_DirtyCast_LogConfig.strContext[0] == '\0')
    {
        iBufOff += ds_snzprintf(strBuffer+iBufOff, iBufLen-iBufOff, "%c%c ", cLevelChar, _DirtyCast_strCategories[eCategory]);
    }
    else
    {
        iBufOff += ds_snzprintf(strBuffer+iBufOff, iBufLen-iBufOff, "%c%c %s ", cLevelChar, _DirtyCast_strCategories[eCategory], _DirtyCast_LogConfig.strContext);
    }

    iBufOff += ds_snzprintf(strBuffer+iBufOff, iBufLen-iBufOff, "%s", pText);

    /* if logger not configured or if dual logging is turned on, we log to stdout using fprintf (we add
       our own timestamping as we disabled the internal timestamping features of DirtySDK / BlazeSDK) */
    if ((_DirtyCast_pLoggerRef == NULL) || _DirtyCast_LogConfig.bDualLogging)
    {
        struct tm Tm;
        int32_t iMsec, iGmtOffset;
        char strTimeBuf[256];

        ds_plattimetotimems(&Tm, &iMsec);
        iGmtOffset = ds_gmtoffset() / 3600;
        ds_snzprintf(strTimeBuf, sizeof(strTimeBuf), "%d/%02d/%02d-%02d:%02d:%02d.%03.3d%+03d ", Tm.tm_year + 1900, Tm.tm_mon + 1, Tm.tm_mday, Tm.tm_hour, Tm.tm_min, Tm.tm_sec, iMsec, iGmtOffset);

        fprintf(stdout, "%s%s", strTimeBuf, strBuffer);
        fflush(stdout);
    }

    /* if logger configured, log to file using the logger (the log line seperator is only
       needed when logging to file) */
    if (_DirtyCast_pLoggerRef != NULL)
    {
        iBufOff += ds_snzprintf(strBuffer+iBufOff, iBufLen-iBufOff, LOG_SEPARATOR_STRING);
        LoggerWrite(_DirtyCast_pLoggerRef, strBuffer, iBufOff);
    }

    #if defined(DIRTYCODE_PC)
    /* the pc server writes log output to its output
       window so we want to redundantly output debug
       output for log printing as well */
    OutputDebugString(strBuffer);
    #endif
}

/*F********************************************************************************/
/*!
    \Function _DirtyCastLogFilter

    \Description
        Log printing for DirtyCast servers.  This version is intended for use by
        the DirtySock debug printf hook.

    \Input *pString         - string to check for filtering
    \Input *pFilterTbl      - filter table to check string against
    \Input iVerbosityLevel  - current verbosity level
    
    \Version 07/30/2010 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _DirtyCastLogFilter(const char *pString, DirtyCastFilterEntryT *pFilterTbl, int32_t iVerbosityLevel)
{
    int32_t iFilter, iResult;
    for (iFilter = 0, iResult = 0; pFilterTbl[iFilter].pFilterText != NULL; iFilter +=1)
    {
        if (pFilterTbl[iFilter].uFilterSize == 0)
        {
            pFilterTbl[iFilter].uFilterSize = (uint32_t)strlen(pFilterTbl[iFilter].pFilterText);
        }
        if ((iVerbosityLevel <= (signed)pFilterTbl[iFilter].uDebugLevel) && !strncmp(pString, pFilterTbl[iFilter].pFilterText, pFilterTbl[iFilter].uFilterSize))
        {
            iResult = pFilterTbl[iFilter].uDebugLevel;
            break;
        }
    }
    return(iResult);
}


/*** Public functions *************************************************************/


/*F********************************************************************************/
/*!
    \Function DirtyCastPidFileCreate

    \Description
        Create pid file for startup/shutdown scripts to use (linux only).

    \Input *pStrPidName - [out] buffer to hold filename of pid file
    \Input iBufSize     - size of output buffer
    \Input *pExecName   - name of executable
    \Input iPortNumber  - diagnostic port number used to create filename
    
    \Output
        int32_t         - negative=failure, else success

    \Version 09/14/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyCastPidFileCreate(char *pStrPidName, int32_t iBufSize, const char *pExecName, int32_t iPortNumber)
{
    #if defined(DIRTYCODE_LINUX)
    const char *pStr;
    int32_t iPid;
    FILE *pFp;

    // Create PID file based on executable name
    pStr = strrchr(pExecName, '/');
    if (pStr == NULL)
    {
        pStr = pExecName;
    }
    else
    {
        pStr++;
    }
    iPid = getpid();
    ds_snzprintf(pStrPidName, iBufSize, "%s%d.pid", pStr, iPortNumber);
    pFp = fopen(pStrPidName, "w");
    if (pFp != NULL)
    {
        fprintf(pFp, "%d\n", iPid);
        fclose(pFp);
    }
    else
    {
        DirtyCastLogPrintf("dirtycast: failed to write pid file (%s): %s\n", pStrPidName, strerror(errno));
        return(-1);
    }
    #endif
    return(0);
}

/*F********************************************************************************/
/*!
    \Function DirtyCastPidFileRemove

    \Description
        Create pid file for startup/shutdown scripts to use (linux only).

    \Input *pStrPidName - name of pid file to remove
    
    \Version 09/14/2007 (jbrookes)
*/
/********************************************************************************F*/
void DirtyCastPidFileRemove(const char *pStrPidName)
{
    #if defined(DIRTYCODE_LINUX)
    if (pStrPidName[0] != '\0')
    {
        remove(pStrPidName);
    }
    #endif
}

/*F********************************************************************************/
/*!
    \Function DirtyCastReadinessFileCreate

    \Description
        Create file used to implement kubernetes readiness probe (linux only).

    \Output
        int32_t - 0=success, negative=failure

    \Version 06/19/2020 (mclouatre)
*/
/********************************************************************************F*/
int32_t DirtyCastReadinessFileCreate(void)
{
    #if defined(DIRTYCODE_LINUX)
    FILE *pFp;

    pFp = fopen("readiness_probe.kube", "w");
    if (pFp != NULL)
    {
        fprintf(pFp, "I am READY!\n");
        fclose(pFp);

        DirtyCastLogPrintf("dirtycast: readiness file created\n");
    }
    else
    {
        DirtyCastLogPrintf("dirtycast: failed to create file used by kubernetes readiness probe: %s\n", strerror(errno));
        return(-1);
    }
    #endif
    return(0);
}

/*F********************************************************************************/
/*!
    \Function DirtyCastReadinessFileRemove

    \Description
        Remove file used to implement kubernetes readiness probe (linux only).

    \Version 06/19/2020 (mclouatre)
*/
/********************************************************************************F*/
void DirtyCastReadinessFileRemove(void)
{
    #if defined(DIRTYCODE_LINUX)
    remove("readiness_probe.kube");
    DirtyCastLogPrintf("dirtycast: readiness file removed\n");
    #endif
}

/*F********************************************************************************/
/*!
    \Function DirtyCastLoggerCreate
     
    \Description
        Parse command-line for logger startup parameters, and create logger.
  
    \Input argc         - command-line argument count
    \Input *argv[]      - command-line argument list
 
    \Output
        int32_t         - negative=error, zero=not started, positive=success
 
    \Version 07/06/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyCastLoggerCreate(int32_t argc, const char *argv[])
{
    int32_t iArg, iResult = 0;
    const char *pCfgName = NULL;
    const char *pErrName = NULL;
    const char *pCatDflt = NULL;
    const char *pPruneTime = NULL;
    const char *pDualLogging = NULL;
    const char *pContext = NULL;

    // find log args
    for (iArg = 0, pCfgName = NULL; iArg < argc; iArg += 1)
    {
        if (!strncmp(argv[iArg], "-logger.cfgfile=", 16))
        {
            pCfgName = argv[iArg] + 16;
            fprintf(stdout, "parsed cfgfile %s\n", pCfgName);
        }
        if (!strncmp(argv[iArg], "-logger.errfile=", 16))
        {
            pErrName = argv[iArg] + 16;
            fprintf(stdout, "parsed errfile %s\n", pErrName);
        }
        //alias of errfile
        if (!strncmp(argv[iArg], "-logger.logfile=", 16))
        {
            pErrName = argv[iArg] + 16;
            fprintf(stdout, "parsed logfile %s\n", pErrName);
        }        
        if (!strncmp(argv[iArg], "-logger.catdflt=", 16))
        {
            pCatDflt = argv[iArg] + 16;
            fprintf(stdout, "parsed catdflt %s\n", pCatDflt);
        }
        if (!strncmp(argv[iArg], "-logger.prunetime=", 18))
        {
            pPruneTime = argv[iArg] + 18;
            fprintf(stdout, "parsed prunetime: %s (days)\n", pPruneTime);
        }
        if (!strncmp(argv[iArg], "-logger.dual=", 13))
        {
            pDualLogging = argv[iArg] + 13;
            _DirtyCast_LogConfig.bDualLogging = (strcmp(pDualLogging, "0") ? TRUE : FALSE);
            fprintf(stdout, "logger mode: %s\n", (_DirtyCast_LogConfig.bDualLogging ? "dual logging (stdout + file)" : "logging to file only"));
        }
        if (!strncmp(argv[iArg], "-logger.context=", 16))
        {
            pContext = argv[iArg] + 16;
            ds_strnzcpy(_DirtyCast_LogConfig.strContext, pContext, sizeof(_DirtyCast_LogConfig.strContext));
            fprintf(stdout, "parsed context %s\n", pContext);
        }
    }
    // if we have a log name start up logger; otherwise assume external logger is used
    if ((pCfgName != NULL) && (pErrName != NULL))
    {
        char strErrName[256], strCatDflt[256], strPruneTime[32];
        const char *aLogParams[5];
        int iArgc = 0;

        // set up log params
        aLogParams[iArgc++] = "logger";
        aLogParams[iArgc++] = pCfgName;
        ds_snzprintf(strErrName, sizeof(strErrName), "-ERROR_LOG=%s", pErrName);
        aLogParams[iArgc++] = strErrName;
        if (pCatDflt != NULL)
        {
            ds_snzprintf(strCatDflt, sizeof(strCatDflt), "-CATEGORY_DEFAULT=%s", pCatDflt);
            aLogParams[iArgc++] = strCatDflt;
        }
        if (pPruneTime != NULL)
        {
            ds_snzprintf(strPruneTime, sizeof(strPruneTime), "-PRUNE_TIME=%s", pPruneTime);
            aLogParams[iArgc++] = strPruneTime;
        }

        // start up the logger
        if ((_DirtyCast_pLoggerRef = LoggerCreate(iArgc, aLogParams, FALSE)) != NULL)
        {
            iResult = 1;
        }
        else
        {
            iResult = -1;
        }
    }

    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function DirtyCastLoggerConfig
     
    \Description
        Configure the logger
  
    \Input *pConfigTags - tags used to configure

    \Version 10/6/2015 (eesponda)
*/
/********************************************************************************F*/
void DirtyCastLoggerConfig(const char *pConfigTags)
{
    const char *pCat;
    char strTag[8];
    int32_t iIdx;
    LogConfigT *pLogConfig = &_DirtyCast_LogConfig;

    pLogConfig->uCategoryMask = TagFieldGetFlags(TagFieldFind(pConfigTags, "log.categoryMask"), 0xffffffff),
    pLogConfig->eLevel = (Level) TagFieldGetNumber(TagFieldFind(pConfigTags, "log.level"), LEVEL_INFO);

    strcpy(strTag, "log.-");
    strTag[strlen(strTag)] = '\0';
    for (pCat = _DirtyCast_strCategories, iIdx = 0; (*pCat != '\0') && (iIdx < 32); pCat++, iIdx++)
    {
        strTag[strlen(strTag)-1] = *pCat;
        pLogConfig->aDbgLevels[iIdx] = TagFieldGetNumber(TagFieldFind(pConfigTags, strTag), 0);
    } 
}

/*F********************************************************************************/
/*!
    \Function DirtyCastLoggerDestroy
     
    \Description
        Shut down logging
  
    \Input None
 
    \Version 07/06/2010 (jbrookes)
*/
/********************************************************************************F*/
void DirtyCastLoggerDestroy(void)
{
    if (_DirtyCast_pLoggerRef != NULL)
    {
        LoggerDestroy(_DirtyCast_pLoggerRef);
        _DirtyCast_pLoggerRef = NULL;
    }
}

/*F********************************************************************************/
/*!
    \Function DirtyCastCommandLineProcess
     
    \Description
        Format command-line options into a tagfield-style buffer.
  
    \Input *pTagBuf     - [out] tagfield buffer storage
    \Input iTagBufSize  - size of output buffer
    \Input iArgStart    - first argument to parse
    \Input iArgCount    - number of arguments
    \Input *pArgs[]     - list of arguments
    \Input *pPrefix     - option prefix
 
    \Output
        char *          - pointer to output buffer
 
    \Version 04/02/2007 (jbrookes)
*/
/********************************************************************************F*/
char *DirtyCastCommandLineProcess(char *pTagBuf, int32_t iTagBufSize, int32_t iArgStart, int32_t iArgCount, const char *pArgs[], const char *pPrefix)
{
    int32_t iArg;
    pTagBuf[0] = '\0';
    for (iArg = iArgStart; iArg < iArgCount; iArg++)
    {
        ds_strnzcat(pTagBuf, pPrefix, iTagBufSize);
        ds_strnzcat(pTagBuf, pArgs[iArg] + 1, iTagBufSize);
        ds_strnzcat(pTagBuf, "\t", iTagBufSize);
    }
    return(pTagBuf);
}

/*F********************************************************************************/
/*!
    \Function DirtyCastConfigFind
     
    \Description
        Find a configuration entry specified by pTagName.  Looks first in the
        command-line tagbuffer and then the command-file tagbuffer.
  
    \Input *pCommandTags    - command-line tagbuffer
    \Input *pConfigTags     - command-file tagbuffer
    \Input *pTagName        - tagname
 
    \Output
        const char *        - tag entry, or NULL if not found
 
    \Version 07/30/2009 (jbrookes)
*/
/********************************************************************************F*/
const char *DirtyCastConfigFind(const char *pCommandTags, const char *pConfigTags, const char *pTagName)
{
    const char *pTagField;
    if ((pCommandTags != NULL) && ((pTagField = TagFieldFind(pCommandTags, pTagName)) != NULL))
    {
        return(pTagField);
    }
    return(TagFieldFind(pConfigTags, pTagName));
}

/*F********************************************************************************/
/*!
    \Function DirtyCastConfigGetString
     
    \Description
        Get a configuration string specified by pTagName.  Looks first in the
        command-line tagbuffer and then the command-file tagbuffer.
  
    \Input *pCommandTags    - command-line tagbuffer
    \Input *pConfigTags     - command-file tagbuffer
    \Input *pTagName        - tagname
    \Input *pBuffer         - [out] storage for result
    \Input iBufLen          - size of output buffer
    \Input *pDefval         - pointer to default value
 
    \Output
        int32_t             - TagFieldGetString() result
 
    \Version 04/02/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyCastConfigGetString(const char *pCommandTags, const char *pConfigTags, const char *pTagName, char *pBuffer, int32_t iBufLen, const char *pDefval)
{
    const char *pTagField;
    if ((pCommandTags != NULL) && ((pTagField = TagFieldFind(pCommandTags, pTagName)) != NULL))
    {
        return(TagFieldGetString(pTagField, pBuffer, iBufLen, pDefval));
    }
    return(TagFieldGetString(TagFieldFind(pConfigTags, pTagName), pBuffer, iBufLen, pDefval));
}

/*F********************************************************************************/
/*!
    \Function DirtyCastConfigGetNumber
     
    \Description
        Get a configuration number specified by pTagName.  Looks first in the
        command-line tagbuffer and then the command-file tagbuffer.
  
    \Input *pCommandTags    - command-line tagbuffer
    \Input *pConfigTags     - command-file tagbuffer
    \Input *pTagName        - tagname
    \Input iDefval          - default value
 
    \Output
        int32_t             - TagFieldGetNumber() result
 
    \Version 04/02/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyCastConfigGetNumber(const char *pCommandTags, const char *pConfigTags, const char *pTagName, int32_t iDefval)
{
    const char *pTagField;
    if ((pCommandTags != NULL) && ((pTagField = TagFieldFind(pCommandTags, pTagName)) != NULL))
    {
        return(TagFieldGetNumber(pTagField, iDefval));
    }
    return(TagFieldGetNumber(TagFieldFind(pConfigTags, pTagName), iDefval));
}

/*F********************************************************************************/
/*!
    \Function DirtyCastLogPrintf

    \Description
        Log printing for DirtyCast servers (compiles in all builds, unlike debug output).

    \Input *pFormat     - printf format string
    \Input ...          - variable argument list
    
    \Version 08/03/2007 (jbrookes)
*/
/********************************************************************************F*/
void DirtyCastLogPrintf(const char *pFormat, ...)
{
    va_list Args;
    char strText[2048];

    va_start(Args, pFormat);
    ds_vsnprintf(strText, sizeof(strText), pFormat, Args);
    _DirtyCastLog(CORE, LEVEL_INFO, strText);
    va_end(Args);
}

/*F********************************************************************************/
/*!
    \Function DirtyCastLogPrintfVerbose

    \Description
        Log printing for DirtyCast servers, with verbose level check; only prints
        if iVerbosityLevel is > iCheckLevel.

    \Input iVerbosityLevel  - current verbosity level
    \Input iCheckLevel      - level to check against
    \Input *pFormat         - printf format string
    \Input ...              - variable argument list
    
    \Version 08/24/2009 (jbrookes)
*/
/********************************************************************************F*/
void DirtyCastLogPrintfVerbose(int32_t iVerbosityLevel, int32_t iCheckLevel, const char *pFormat, ...)
{
    va_list Args;
    char strText[2048];

    if (iVerbosityLevel > iCheckLevel)
    {
        va_start(Args, pFormat);
        ds_vsnprintf(strText, sizeof(strText), pFormat, Args);
        _DirtyCastLog(CORE, LEVEL_INFO, strText);
        va_end(Args);
    }
}

/*F********************************************************************************/
/*!
    \Function DirtyCastLogDbgPrintf

    \Description
        Log printing for DirtyCast servers.  This version is intended for use by
        the DirtySock debug printf hook.

    \Input iVerbosityLevel  - current verbosity level
    \Input *pFormat         - printf format string
    \Input ...              - variable argument list

    \Output
        int32_t             - zero if not filtered, else filtered
    
    \Version 08/03/2007 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyCastLogDbgPrintf(int32_t iVerbosityLevel, const char *pFormat, ...)
{
    va_list Args;
    char strText[2048];
    int32_t iResult;

    va_start(Args, pFormat);
    ds_vsnprintf(strText, sizeof(strText), pFormat, Args);
    va_end(Args);

    if ((iResult = _DirtyCastLogFilter(strText, _DbgFilterTable, iVerbosityLevel)) == 0)
    {
        _DirtyCastLog(CORE, LEVEL_INFO, strText);
    }
    else
    {
        // show what would be filtered instead of filtering
        #if DIRTYCAST_FILTERDEBUG
        DirtyCastLogPrintf("%% FILTERED (%d/%d) %% %s", iVerbosityLevel, iResult, strText);
        #endif
    }
    return(iResult);
}

/*F********************************************************************************/
/*!
    \Function DirtyCastGetHostName

    \Description
        Get host name.

    \Input *pHostName   - [out] storage for host name
    \Input iNameLen     - length of host name buffer
    
    \Output
        int32_t         - 0=success, negative=failure

    \Version 02/19/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t DirtyCastGetHostName(char *pHostName, int32_t iNameLen)
{
    return(gethostname(pHostName, iNameLen));
}

/*F********************************************************************************/
/*!
    \Function DirtyCastGetHostNameByAddr

    \Description
        Get host name using the address

    \Input *pAddr       - address we are looking up by
    \Input iAddrLen     - size of address
    \Input *pHostname   - [out] storage for host name or NULL if not needed
    \Input iNameLen     - length of host name buffer
    \Input *pServiceName- [out] storage for service name or NULL if not needed
    \Input iServLen     - length of service name buffer

    \Output
        int32_t         - 0=success, negative=failure

    \Version 02/25/2016 (eesponda)
*/
/********************************************************************************F*/
int32_t DirtyCastGetHostNameByAddr(struct sockaddr *pAddr, int32_t iAddrLen, char *pHostName, int32_t iNameLen, char *pServiceName, int32_t iServLen)
{
    return(getnameinfo(pAddr, iAddrLen, pHostName, iNameLen, pServiceName, iServLen, NI_NAMEREQD));
}

/*F********************************************************************************/
/*!
    \Function DirtyCastCtime

    \Description
        Get ctime without trailing \n

    \Input *pBuffer     - [out] storage for time string
    \Input iBufSize     - size of output buffer
    \Input uTime        - time to get string version of
    
    \Output
        char *          - pointer to time string

    \Version 09/21/2010 (jbrookes)
*/
/********************************************************************************F*/
char *DirtyCastCtime(char *pBuffer, int32_t iBufSize, time_t uTime)
{
    // get ctime
    #if defined(DIRTYCODE_LINUX)
    ctime_r(&uTime, pBuffer);
    #else
    ds_strnzcpy(pBuffer, ctime(&uTime), iBufSize);
    #endif

    // remove trailing \n and return to caller
    pBuffer[strlen(pBuffer)-1] = '\0';
    return(pBuffer);
}

/*F********************************************************************************/
/*!
    \Function DirtyCastGetLastError

    \Description
        Get most recent network error code.

    \Input None.
    
    \Output
        uint32_t        - last network error

    \Version 11/12/2007 (jbrookes)
*/
/********************************************************************************F*/
uint32_t DirtyCastGetLastError(void)
{
    uint32_t uError;
#if defined(DIRTYCODE_PC)
    uError = WSAGetLastError();
#elif defined(DIRTYCODE_LINUX)
    uError = errno;
#else
#error not implemented on this platform
#endif
    return(uError);
}

/*F********************************************************************************/
/*!
    \Function DirtyCastGetServerInfo

    \Description
        Update server information related to CPU performance, memory consumption, etc.

    \Input *pServerInfo - [out] buffer to store updated info
    \Input uCurTick     - current millisecond tick count
    \Input bResetMetrics- if true, reset metrics accumulator
    \Input uResetTicks  - ticks since last metrics reset

    \Version 04/11/2007 (jbrookes)
*/
/********************************************************************************F*/
void DirtyCastGetServerInfo(DirtyCastServerInfoT *pServerInfo, uint32_t uCurTick, uint8_t bResetMetrics, uint32_t uResetTicks)
{
    #if defined(DIRTYCODE_LINUX)
    struct sysinfo SysInfo;
    struct tms Times;
    uint32_t uClockTicksPerSecond = sysconf(_SC_CLK_TCK);

    if (times(&Times) != (clock_t)-1)
    {
        // sum user and system time and convert from clock ticks to milliseconds
        uint32_t uTime = ((Times.tms_utime + Times.tms_stime) * 1000) / uClockTicksPerSecond;
        if ((pServerInfo->fCpu = ((float)(uTime - pServerInfo->uLastTime) * 100.0f) / (float)uResetTicks) > 99.99f)
        {
            pServerInfo->fCpu = 99.99f;
        }
        pServerInfo->uTime = uTime;
        if (bResetMetrics)
        {
            pServerInfo->uLastTime = uTime;
        }
    }
    pServerInfo->iNumCPUCores = NetConnStatus('proc', 0, NULL, 0);

    // fetch load averages (note that we normalize based on the number of cpu cores)
    if (sysinfo(&SysInfo) == 0)
    {
        pServerInfo->aLoadAvg[0] = SysInfo.loads[0];
        pServerInfo->aLoadAvg[1] = SysInfo.loads[1];
        pServerInfo->aLoadAvg[2] = SysInfo.loads[2];
    }
    else
    {
        ds_memclr(pServerInfo->aLoadAvg, sizeof(pServerInfo->aLoadAvg));
    }

    // copy out memory info
    pServerInfo->uTotalRam = SysInfo.totalram;
    pServerInfo->uFreeRam = SysInfo.freeram;
    pServerInfo->uSharedRam = SysInfo.sharedram;
    pServerInfo->uBufferRam = SysInfo.bufferram;
    pServerInfo->uTotalSwap = SysInfo.totalswap;
    pServerInfo->uFreeSwap = SysInfo.freeswap;
    pServerInfo->uNumProcs = SysInfo.procs;
    pServerInfo->uTotalHighMem = SysInfo.totalhigh;
    pServerInfo->uFreeHighMem = SysInfo.freehigh;
    pServerInfo->uMemBlockSize = SysInfo.mem_unit;
    #endif
}

/*F********************************************************************************/
/*!
    \Function DirtyCastGetPid

    \Description
        Gets the process id for the application

    \Output
        int32_t             - process identifier

    \Version 02/27/2018 (eesponda)
*/
/********************************************************************************F*/
int32_t DirtyCastGetPid(void)
{
    #if defined(DIRTYCODE_PC)
    return(GetCurrentProcessId());
    #elif defined(DIRTYCODE_LINUX)
    return(getpid());
    #else
    #error not implemented on this platform
    #endif
}

/*F********************************************************************************/
/*!
    \Function DirtyCastWatchFileInit

    \Description
        Initializes a file watcher for modifications at the specified path

    \Input *pPath   - path to the file we want to watch

    \Output
        int32_t     - positive=file descriptor of the watcher, negative=failure/unsupported

    \Version 03/31/2020 (eesponda)
*/
/********************************************************************************F*/
int32_t DirtyCastWatchFileInit(const char *pPath)
{
    int32_t iFd;

    #if defined(DIRTYCODE_LINUX)
    // initialize the inotify system and make sure the file is set to nonblock
    if ((iFd = inotify_init1(IN_NONBLOCK)) < 0)
    {
        DirtyCastLogPrintf("dirtycast: inotify_init1 failed with %d\n", errno);
    }
    // add the watch for any event on the file
    if (inotify_add_watch(iFd, pPath, IN_MODIFY) < 0)
    {
        close(iFd);
        DirtyCastLogPrintf("dirtycast: inotify_add_watch failed with %d\n", errno);
    }
    #else
    iFd = -1;
    #endif

    return(iFd);
}

/*F********************************************************************************/
/*!
    \Function DirtyCastWatchFileProcess

    \Description
        Processes the file watcher for changes in the file

    \Input iFd  - the watcher file descriptor 

    \Output
        uint8_t - TRUE=file changed, FALSE=file unchanged

    \Version 03/31/2020 (eesponda)
*/
/********************************************************************************F*/
uint8_t DirtyCastWatchFileProcess(int32_t iFd)
{
    #if defined(DIRTYCODE_LINUX)
    struct inotify_event Event;
    // if we hit an error or no bytes were read then indicate the file didn't change
    if (read(iFd, &Event, sizeof(Event)) <= 0)
    {
        return(FALSE);
    }
    return((Event.mask & IN_MODIFY) != 0);
    #else
    return(FALSE);
    #endif
}

/*F********************************************************************************/
/*!
    \Function DirtyCastWatchFileClose

    \Description
        Closes the file descriptor

    \Input iFd  - the watcher file descriptor 

    \Version 03/31/2020 (eesponda)
*/
/********************************************************************************F*/
void DirtyCastWatchFileClose(int32_t iFd)
{
    #if defined(DIRTYCODE_LINUX)
    close(iFd);
    #endif
}
