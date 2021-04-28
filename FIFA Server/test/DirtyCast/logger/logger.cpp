/*H********************************************************************************/
/*!
    \File logger.c

    \Description
        This module implements log capture and writing routines.  It can be
        compiled either as a library module or as an external application capturing
        stdin and writing to a rotating log file.  Configuration parameters are
        read from the logger.cfg file.

    \Copyright
        Copyright (c) 2004-2010 Electronic Arts Inc.

    \Version 1.0 12/13/2004 (tchen)    First Version
    \Version 1.1 08/16/2007 (jbrookes) Added command-line config override functionality
    \Version 2.0 07/06/2010 (jbrookes) Implemented "internal" logger option, ported to Windows
*/
/********************************************************************************H*/

/*** Include files ****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef _WIN32
 #include <windows.h>
 #include <io.h>
 #define open _open
#endif
#ifdef __linux__
 #include <unistd.h>
 #include <wait.h>
 #include <bzlib.h>
 #include <zlib.h>
#endif
#include <errno.h>
#include "DirtySDK/platform.h"
#include "DirtySDK/dirtysock/dirtymem.h"
#include "DirtySDK/dirtysock/netconn.h"
#include "hasher.h"
#include "configfile.h"
#include "LegacyDirtySDK/util/tagfield.h"

#include "eathread/eathread.h"
#include "eathread/eathread_condition.h"
#include "eathread/eathread_mutex.h"
#include "eathread/eathread_semaphore.h"
#include "eathread/eathread_thread.h"

#include "logger.h"

/*** Defines **********************************************************************/

#define MAX_FILE_NAME_LEN           (512)
#define MAX_ROTATION_TIME_DIGITS    (2)         // max number of digits for the rotation time digits (in days)
#define MAX_ROTATION_DIGITS         (10)        // max number of digits to specify rotation max file size (in kbytes)
#define MAX_LOG_ENTRY_LEN           (10000)     // max number of chars in a log entry
#define MAX_CATEGORY_LEN            (250)       // max number of chars in a category
#define FORMAT_STRING_LEN           (35 + MAX_CATEGORY_LEN)
#define DEFAULT_CATEGORY            ("DEFAULT")
#define MAX_WRITE_BUFFER_DIGITS     (8)         // max number of digits to specify log queue size
#define INCREMENTAL_ALLOC           (20)        // allocate write buffer incrementally for X many times until max
#define SEC_IN_DAY                  (3600 * 24) // 3600 sec in 1 hour * 24 hour in a day

#ifdef DIRTYCODE_LINUX

// this is needed to stop MS compiler from complaining about POSIXs calls
#define _fileno    fileno
#define _close     close
#define _dup2      dup2

#endif

/*** Variables **********************************************************************/

typedef struct LogEntryT
{
    char *pEntry;
    uint32_t uCurrentUsedSize;
    uint32_t uCurrentSize;
    uint32_t uMaxSize;
} LogEntryT;

typedef struct OutputBufferT
{
    FILE *pOutFileHandle;
    LogEntryT *pLogEntry;
    LogEntryT *pLogEntrySwap;
} OutputBufferT;

typedef struct CategoryCfgT
{
    char iCompressFlag;
    char iFormatFlag;
    uint32_t uRotation;
    uint32_t uRotationTime;
    uint32_t uFlush;
    uint32_t uCurrentOutFileSize;
    uint32_t uLastFlushTime;
    char strOutFile[MAX_FILE_NAME_LEN];
    char strCategoryName[MAX_CATEGORY_LEN];
    uint8_t bStartupRotate;
    OutputBufferT *pOutBuf;
    time_t uLastRotateTime;
    EA::Thread::Mutex *pMutex;
} CategoryCfgT;

// module state
struct LoggerRefT
{
    // below are variables that store configuration values read from logger.cfg
    FILE *pErrorLog;                            //!< pointer to error log file; NULL if we are using stderr
    HasherRef *pCfgTbl;                         //!< storage for category specific configuration params
    HasherRef *pOutBufTbl;                      //!< storage for output file handles and buffered log entries
    char strErrorLog[MAX_FILE_NAME_LEN];        //!< error log file name
    char strCompressTool[24];                   //!< compress tool 
    uint32_t iReadBufferSize;                   //!< size of read buffer
    char iDelimiter;                            //!< the char that signals the end of log entries
    uint32_t uWarnInterval;
    EA::Thread::Condition *pHaveEntriesForWrite;
    EA::Thread::Mutex *pCondMutex;
    EA::Thread::Mutex *pHashTblMutex;
    volatile int32_t iWriterStatus;
    volatile uint8_t bLoop;
    uint8_t bNewLine;
};

/*** Private Functions *****************************************************************/


/*F********************************************************************************/
/*!
    \Function _LogGetCurrentTime

    \Description
       Gets and parses the current time into components

    \Input *pYear   - pointer to uint32_t var to store 'year' in 
    \Input *pMonth  - pointer to uint8_t var to store 'month' in 
    \Input *pDay    - pointer to uint8_t var to store 'day' in 
    \Input *pHour   - pointer to uint8_t var to store 'hour' in 
    \Input *pMin    - pointer to uint8_t var to store 'min' in 
    \Input *pSec    - pointer to uint8_t var to store 'sec' in 
    \Input *pMillis - pointer to uint32_t var to store 'milliseconds' in
    \Input *pEpoch  - pointer to uint32_t var to store 'seconds since epoch' in

    \Version 12/13/2004 (tchen)
*/
/********************************************************************************F*/
static void _LogGetCurrentTime(uint32_t *pYear, uint8_t *pMonth, uint8_t *pDay, uint8_t *pHour, uint8_t *pMin, uint8_t *pSec, uint32_t *pMillis, uint32_t *pEpoch)
{
    struct tm Time;
    int32_t iMillis;

    // get epoch
    *pEpoch = ds_timeinsecs();

    // get the rest of the data needed
    ds_plattimetotimems(&Time, &iMillis);
    *pYear = 1900 + Time.tm_year;
    *pMonth = Time.tm_mon + 1;
    *pDay = Time.tm_mday;
    *pHour = Time.tm_hour;
    *pMin = Time.tm_min;
    *pSec = Time.tm_sec;
    *pMillis = iMillis;
}

/*F********************************************************************************/
/*!
    \Function _LogError

    \Description
        Prints error message to logger error log file 
  
    \Input *pLoggerRef - module state
    \Input *strError   - string error message 

    \Version 12/13/2004 (tchen)
*/
/********************************************************************************F*/
static void _LogError(LoggerRefT *pLoggerRef, const char *strError)
{
    uint32_t uYear = 0, uMillis = 0, uEpoch = 0;
    uint8_t uMonth = 0, uDay = 0, uHour = 0, uMin = 0, uSec = 0;
    char strMessage[1024];

    _LogGetCurrentTime(&uYear, &uMonth, &uDay, &uHour, &uMin, &uSec, &uMillis, &uEpoch);
    ds_snzprintf(strMessage, sizeof(strMessage), "%02u/%02u/%02u %02u:%02u:%02u.%03u LG |  %s\n", uYear, uMonth, uDay, uHour, uMin, uSec, uMillis, strError);
    if (pLoggerRef->pErrorLog != NULL)
    {
        fprintf(pLoggerRef->pErrorLog, "%s", strMessage);
        fflush(pLoggerRef->pErrorLog);
    }
    else
    {
        fwrite(strMessage, sizeof(char), strlen(strMessage), stderr);
        fflush(stderr);
    }
}

/*F********************************************************************************/
/*!
    \Function _LogWriteCompressed

    \Description
        Compresses buffer and writes it out to file

    \Input *pLoggerRef - module state
    \Input *pBuf       - pointer to buffer to write out
    \Input *iBytes     - number of bytes to write from buffer
    \Input *pFile      - pointer to file handle

    \Output
        uint32_t       - number of bytes written to file (before compression)

    \Version 12/13/2004 (tchen)
*/
/********************************************************************************F*/
static uint32_t _LogWriteCompressed(LoggerRefT *pLoggerRef, char *pBuf, int32_t iBytes, FILE *pFile)
{
#ifdef __linux__
    BZFILE* bzfile = NULL;
    gzFile gzfile = NULL;
    int32_t error;
    uint32_t uBytesIn;
    uint32_t uBytesOut;

    if (strcmp(pLoggerRef->strCompressTool, "bzip2") == 0)
    {
        bzfile = BZ2_bzWriteOpen(&error, pFile, 9, 0, 0);
        if (error != BZ_OK)
        {
            BZ2_bzWriteClose(&error, bzfile, 0, NULL, NULL);
            _LogError(pLoggerRef, "Can't open bz2 file stream");
            return 0;
        } 
        BZ2_bzWrite(&error, bzfile, pBuf, iBytes);
        if (error == BZ_IO_ERROR)
        {
            BZ2_bzWriteClose(&error, bzfile, 0, NULL, NULL);
            _LogError(pLoggerRef, "Encountered BZ_IO_ERROR");
            return 0;
        }
        BZ2_bzWriteClose(&error, bzfile, 0, &uBytesIn, &uBytesOut);
        if (error != BZ_OK)
        {
            _LogError(pLoggerRef, "Can't close the bz2 file stream");
            return(0);
        }
        else
        {
            return(iBytes);
        }
    }
    else if (strcmp(pLoggerRef->strCompressTool,"gzip") == 0)
    {
        gzfile = gzdopen(dup(_fileno(pFile)), "a");
        error = gzwrite(gzfile, pBuf, iBytes);
        iBytes = error;
        error = gzclose(gzfile);
        if (error != Z_OK)
        {
            _LogError(pLoggerRef, gzerror(gzfile, &error));
            return(0);
        }
        return(iBytes);
    }
    else
#endif
    {
        return((int32_t)fwrite(pBuf, sizeof(char), iBytes/sizeof(char), pFile));
    }
}


/*F********************************************************************************/
/*!
    \Function _LogFileExists

    \Description
        Checks existence of file
        
    \Input *pFileName   - file name to check

    \Output
        char            - 0=file does not exist, 1=file exists

    \Version 12/13/2004 (tchen)
*/
/********************************************************************************F*/
static char _LogFileExists(const char *pFileName)
{
    struct stat buf;
    char i = stat(pFileName, &buf);
    if (i==0)
    {
        return(TRUE);
    }
    return(FALSE);
}

/*F********************************************************************************/
/*!
    \Function _CreateLogDirectory

    \Description
        Checks existence of necessary parent directoris of a log file and
        creates them if they don't exists
        
    \Input *pLoggerRef  - logger module state 
    \Input *pFileName   - path of filename

    \Version 12/13/2004 (tchen)
*/
/********************************************************************************F*/
static void _CreateLogDirectory(LoggerRefT *pLoggerRef, char *fileName)
{
#ifdef __linux__
    char *dirName;
    char *buf;
    char *tok;
    struct stat file;
    char ret;

    dirName = (char *)malloc(MAX_FILE_NAME_LEN * sizeof(char));
    buf = (char *)malloc(MAX_FILE_NAME_LEN * sizeof(char));
    dirName[0] = '\0';
    buf[0] = '\0';
    strcpy(buf, fileName);
    tok = strtok(buf, "/");
    if(!_LogFileExists(fileName))
    {
        while(tok != NULL)
        {
            strcat(dirName, tok);
            ret = stat(dirName, &file);
            tok = strtok(NULL, "/"); 
            if ((ret < 0) && (tok != NULL)) // directory doesn't exist
            {
                if (mkdir(dirName, 0775) != 0)
                {
                    _LogError(pLoggerRef, "_CreateLogDirectory: failed to create log directory");
                    perror(dirName);
                    free(dirName);
                    free(buf);
                    return;
                }
            } 
            strcat(dirName, "/");
        }
    }
    free(dirName);
    free(buf);
#endif // #ifdef __linux__
}


/*F********************************************************************************/
/*!
    \Function _AddCategoryCfgUsingTemplate

    \Description
        Gets and parses the current time into components

    \Input *pLoggerRef      - module state
    \Input *pFrom           - pointer to CategoryCfgT to copy from
    \Input *pCategoryName   - pointer to string name

    \Output
        CategoryCfgT *      - pointer to new category

    \Version 12/13/2004 (tchen)
*/
/********************************************************************************F*/
static CategoryCfgT *_AddCategoryCfgUsingTemplate(LoggerRefT *pLoggerRef, CategoryCfgT *pFrom, char *pCategoryName)
{
    char *pFind;
    char *pNext;
    OutputBufferT *pOutBuf;
    LogEntryT *pLogEntry;
    LogEntryT *pLogEntrySwap;
    CategoryCfgT *pTo = (CategoryCfgT *)malloc(sizeof(CategoryCfgT));
    struct stat file;

    pTo->iCompressFlag = pFrom->iCompressFlag;
    pTo->iFormatFlag = pFrom->iFormatFlag;
    pTo->uRotation = pFrom->uRotation;
    pTo->uFlush = pFrom->uFlush;

    pTo->uLastFlushTime = time(NULL);
    pTo->uLastRotateTime = time(NULL);
    pTo->bStartupRotate = FALSE;
    pTo->uCurrentOutFileSize = 0;
    strcpy(pTo->strCategoryName, pCategoryName);
    pTo->pMutex = new EA::Thread::Mutex;

    ds_memclr(pTo->strOutFile, sizeof(pTo->strOutFile));
    pFind = pFrom->strOutFile;
    pNext = strchr(pFind, '/');

    while(pNext != NULL)
    {
        pFind = pNext;
        pNext = strchr(pFind+1, '/');
    }
    pNext = pFrom->strOutFile;

    if(pNext != pFind)
    {
        strncpy(pTo->strOutFile, pNext, (size_t)(pFind - pNext) + 1);
    }    
    strcat(pTo->strOutFile, pCategoryName);
    if (*pFind != '/')
    {
        strcat(pTo->strOutFile, "/");
    }
    strcat(pTo->strOutFile, pFind);

    pOutBuf = (OutputBufferT *)HashStrFind(pLoggerRef->pOutBufTbl, pTo->strOutFile);

    if(pOutBuf == NULL)
    {
        pOutBuf = (OutputBufferT*)malloc(sizeof(OutputBufferT));
        pLogEntry = (LogEntryT*)malloc(sizeof(LogEntryT));
        pLogEntrySwap = (LogEntryT*)malloc(sizeof(LogEntryT));

        pLogEntry->uCurrentUsedSize = 0;
        pLogEntry->uMaxSize = pFrom->pOutBuf->pLogEntry->uMaxSize;
        pLogEntry->pEntry = (char*)malloc(sizeof(char) * (pLogEntry->uMaxSize/INCREMENTAL_ALLOC));
        pLogEntry->uCurrentSize = (sizeof(char) * (pLogEntry->uMaxSize/INCREMENTAL_ALLOC));
        ds_memclr(pLogEntry->pEntry, pLogEntry->uCurrentSize);
        pOutBuf->pLogEntry = pLogEntry;

        pLogEntrySwap->uCurrentUsedSize = 0;
        pLogEntrySwap->uMaxSize = pFrom->pOutBuf->pLogEntry->uMaxSize;
        pLogEntrySwap->pEntry = (char *)malloc(sizeof(char) * (pLogEntry->uMaxSize/INCREMENTAL_ALLOC));
        pLogEntrySwap->uCurrentSize = (sizeof(char) * (pLogEntry->uMaxSize/INCREMENTAL_ALLOC));
        ds_memclr(pLogEntrySwap->pEntry, pLogEntrySwap->uCurrentSize);
        pOutBuf->pLogEntrySwap = pLogEntrySwap;

        _CreateLogDirectory(pLoggerRef, pTo->strOutFile);

        // if log file exits rotate
        if (_LogFileExists(pTo->strOutFile))
        {
            pTo->bStartupRotate = TRUE;
        }

        pOutBuf->pOutFileHandle = fopen(pTo->strOutFile, "a");
        if(pOutBuf->pOutFileHandle == NULL)
        {
            _LogError(pLoggerRef, "_AddCategoryCfgUsingTemplate: failed to open output file");
            perror(pTo->strOutFile);
            strcpy(pTo->strOutFile, pFrom->strOutFile);
            free(pOutBuf);
            pOutBuf = pFrom->pOutBuf; // use default outbuf instead
        }
        else
        {
            if (fstat(_fileno(pOutBuf->pOutFileHandle), &file) == 0)
            {
                pTo->uCurrentOutFileSize = file.st_size;
            }
            // if error file matches this out file, share file ref
            if (!strcmp(pLoggerRef->strErrorLog, pTo->strOutFile))
            {
                pLoggerRef->pErrorLog = pOutBuf->pOutFileHandle;
            }
            HashStrAdd(pLoggerRef->pOutBufTbl, pTo->strOutFile, pOutBuf);
        }
    }
    pTo->pOutBuf = pOutBuf;

    HashStrAdd(pLoggerRef->pCfgTbl, pTo->strCategoryName, pTo);

    return(pTo);
}

/*F********************************************************************************/
/*!
    \Function _LogAddEntry

    \Description
        Add an entry to the output queue.
        
    \Input *pLoggerRef      - module state
    \Input *strReadBuffer   - source text to add
    \Input iBytesRead       - length of text

    \Version 07/06/2010 (jbrookes) Split from _LogReaderFunc
*/
/********************************************************************************F*/
static void _LogAddEntry(LoggerRefT *pLoggerRef, const char *strReadBuffer, int32_t iBytesRead)
{
    char strCurrentEntry[MAX_LOG_ENTRY_LEN] = "";
    char strCategory[MAX_CATEGORY_LEN] = "";
    char iLevel = 0, iChar;
    char *pCategory = NULL;
    int32_t iCounter = 0;
    int32_t iCurrentEntryCounter = 0;
    unsigned char uMonth, uDay, uHour, uMin, uSec;
    uint32_t uYear, uMillis, uEpoch;
    uint32_t uLastWarning = 0;
    char bCategoryParsed = FALSE;
    char bSameAsLastCategory = TRUE;
    CategoryCfgT *pCfg = NULL;

    _LogGetCurrentTime(&uYear, &uMonth, &uDay, &uHour, &uMin, &uSec, &uMillis, &uEpoch);

    for (int32_t i = 0; i < iBytesRead; i++)
    {
        iChar = strReadBuffer[i];

        if ((pLoggerRef->iDelimiter == iChar) && (pCfg != NULL)) //reached end of a log entry
        {
            bCategoryParsed = FALSE;
            bSameAsLastCategory = TRUE;
            strCurrentEntry[iCurrentEntryCounter] = '\0';

            pCfg->pMutex->Lock();
            if ((pCfg->pOutBuf->pLogEntry->uCurrentSize - pCfg->pOutBuf->pLogEntry->uCurrentUsedSize) < (iCurrentEntryCounter+1)*sizeof(char))
            {
                if (pCfg->pOutBuf->pLogEntry->uCurrentSize == pCfg->pOutBuf->pLogEntry->uMaxSize)
                {
                    if((uEpoch - uLastWarning) >= pLoggerRef->uWarnInterval)
                    {
                        uLastWarning = uEpoch;
                        _LogError(pLoggerRef, "WARNING: Write buffer is full, log entries will be lost");
                    }
                }
                else
                {
                    while((pCfg->pOutBuf->pLogEntry->uCurrentSize - pCfg->pOutBuf->pLogEntry->uCurrentUsedSize) < (iCurrentEntryCounter+1)*sizeof(char))
                    {
                        if ((pCfg->pOutBuf->pLogEntry->uCurrentSize + (pCfg->pOutBuf->pLogEntry->uMaxSize / INCREMENTAL_ALLOC)) > pCfg->pOutBuf->pLogEntry->uMaxSize)
                        {
                            pCfg->pOutBuf->pLogEntry->uCurrentSize = pCfg->pOutBuf->pLogEntry->uMaxSize;
                        }
                        else
                        {
                            pCfg->pOutBuf->pLogEntry->uCurrentSize = (pCfg->pOutBuf->pLogEntry->uCurrentSize + (pCfg->pOutBuf->pLogEntry->uMaxSize / INCREMENTAL_ALLOC));
                        }
                        pCfg->pOutBuf->pLogEntry->pEntry = (char*)realloc(pCfg->pOutBuf->pLogEntry->pEntry, pCfg->pOutBuf->pLogEntry->uCurrentSize);
                    }
                    strcat(pCfg->pOutBuf->pLogEntry->pEntry, strCurrentEntry);
                    pCfg->pOutBuf->pLogEntry->uCurrentUsedSize += iCurrentEntryCounter*sizeof(char);
                }
            }
            else
            {
                strcat(pCfg->pOutBuf->pLogEntry->pEntry, strCurrentEntry);
                pCfg->pOutBuf->pLogEntry->uCurrentUsedSize += iCurrentEntryCounter*sizeof(char);
            }
            pCfg->pMutex->Unlock();
            pLoggerRef->pHaveEntriesForWrite->Signal();
            if (strCurrentEntry[iCurrentEntryCounter-1] == '\n')
            {
                pLoggerRef->bNewLine = TRUE;
            }
            strCurrentEntry[0] = '\0';
            iCounter = 0;
            iCurrentEntryCounter = 0;
        }
        else //continue reading
        {
            if (iCounter==0)
            {
                iLevel = iChar;
            }
            else if ((iCounter>=1) && !bCategoryParsed && (iChar != ' '))
            {
                if (strCategory[iCounter-1] != iChar)
                {
                    bSameAsLastCategory = FALSE;
                }

                strCategory[iCounter-1] = iChar;

                if ((iCounter == 1) && (iChar != '_'))
                {
                    bCategoryParsed = TRUE;
                }
            }
            else
            {
                if (strCurrentEntry[0] == '\0')
                {
                    bCategoryParsed = TRUE;

                    if (strCategory[iCounter-1] != '\0')
                    {
                        bSameAsLastCategory = FALSE;
                        strCategory[iCounter-1] = '\0';
                    }

                    if (!bSameAsLastCategory)
                    {
                        pCategory = (strCategory[0] == '_') ? &strCategory[1] : & strCategory[0];

                        pCfg = (CategoryCfgT *)HashStrFind(pLoggerRef->pCfgTbl, pCategory);
                        if (pCfg == NULL)
                        {
                            pCfg = (CategoryCfgT *)HashStrFind(pLoggerRef->pCfgTbl, DEFAULT_CATEGORY);
                            if (strCategory[0] == '_')
                            {
                                pCfg = _AddCategoryCfgUsingTemplate(pLoggerRef, pCfg, pCategory);
                            }
                        }
                    }

                    if ((pCfg != NULL) && (pCfg->iFormatFlag == '1') && (pLoggerRef->bNewLine))
                    {
                        iCurrentEntryCounter = sprintf(strCurrentEntry, "%02u/%02u/%02u %02u:%02u:%02u.%03u %s%c | ", uYear, uMonth, uDay, uHour, uMin, uSec, uMillis, pCategory, iLevel);
                        pLoggerRef->bNewLine = FALSE;
                    }
                }
                strCurrentEntry[iCurrentEntryCounter] = iChar;
                iCurrentEntryCounter++;
            }
            iCounter++;
        }
    }
}

/*F********************************************************************************/
/*!
    \Function _LogWriterFunc

    \Description
        Writer thread function responsible for writing log entries to appropriate
        files and rotating logs at specified time intervals

    \Input *pParam  - module state

    \Version 12/13/2004 (tchen)
*/
/********************************************************************************F*/
static intptr_t _LogWriterFunc(void *pParam)
{
    uint32_t uEpoch;
    CategoryCfgT *pCfg = NULL;
    bool bLastIterDone = FALSE;
    LogEntryT *pTemp;
    char bNoEntries = FALSE;
    LoggerRefT *pLoggerRef = (LoggerRefT *)pParam;
    #ifdef __linux__
    int32_t iPid;
    #endif

    while(pLoggerRef->bLoop || !bLastIterDone) //loop if STDIN not closed or queue not empty
    {
        if (!pLoggerRef->bLoop)
        {
            bLastIterDone = TRUE;
        }

        if (bNoEntries && pLoggerRef->bLoop)
        {
            pLoggerRef->pCondMutex->Lock();
            pLoggerRef->pHaveEntriesForWrite->Wait(pLoggerRef->pCondMutex);
            pLoggerRef->pCondMutex->Unlock();
        }

        pLoggerRef->pHashTblMutex->Lock();
        HasherEnumInit(pLoggerRef->pCfgTbl);
        bNoEntries = TRUE;
        while ((pCfg = (CategoryCfgT *)HasherEnum(pLoggerRef->pCfgTbl, NULL)) != NULL)
        {
            #ifdef __linux__
            waitpid((pid_t)-1, &iPid, WNOHANG); // pickup any zombie child processes
            #endif
            if (pCfg == NULL)
            {
                fprintf(stdout, "logger: no config\n");
                fflush(stdout);
                break;
            }

            pCfg->pMutex->Lock();
            if (pCfg->pOutBuf->pLogEntry->uCurrentUsedSize == 0)
            {
                pCfg->pMutex->Unlock();
                continue;
            }
            else
            {
                bNoEntries = FALSE;
                pTemp = pCfg->pOutBuf->pLogEntry;
                pCfg->pOutBuf->pLogEntry = pCfg->pOutBuf->pLogEntrySwap;
                pCfg->pOutBuf->pLogEntrySwap = pTemp; 
            }

            if (pCfg->iCompressFlag == '2')
            {
                pCfg->uCurrentOutFileSize += _LogWriteCompressed(pLoggerRef, pCfg->pOutBuf->pLogEntrySwap->pEntry, pCfg->pOutBuf->pLogEntrySwap->uCurrentUsedSize * sizeof(char), pCfg->pOutBuf->pOutFileHandle);
            }
            else
            {
                pCfg->uCurrentOutFileSize += (uint32_t)fwrite(pCfg->pOutBuf->pLogEntrySwap->pEntry, sizeof(char), pCfg->pOutBuf->pLogEntrySwap->uCurrentUsedSize/sizeof(char), pCfg->pOutBuf->pOutFileHandle);
            }
            pCfg->pOutBuf->pLogEntrySwap->uCurrentUsedSize = 0;
            pCfg->pOutBuf->pLogEntrySwap->pEntry[0] = '\0';

            uEpoch = ds_timeinsecs();

            if (pCfg->uFlush <= (uEpoch - pCfg->uLastFlushTime))
            {
                fflush(pCfg->pOutBuf->pOutFileHandle);
                pCfg->uLastFlushTime = uEpoch;
            }
            
            pCfg->pMutex->Unlock();
        }
        pLoggerRef->pHashTblMutex->Unlock();
    }

    pLoggerRef->iWriterStatus = -1;
    EA::Thread::ThreadEnd(0);
    return(0);
}

/*F********************************************************************************/
/*!
    \Function _LogCommandLineProcess

    \Description
        Processes command-line arguments starting with the third argument,
        converting them to tagfield entries in the process.

    \Input *pTagBuf     - output buffer to hold tagfield entries
    \Input iTagBufSize  - size of pTagBuf
    \Input iArgCt       - total number of arguments
    \Input *pArgs[]     - argument list

    \Version 08/16/2007 (jbrookes)
*/
/********************************************************************************F*/
static void _LogCommandLineProcess(char *pTagBuf, int32_t iTagBufSize, int32_t iArgCt, const char *pArgs[])
{
    int32_t iArg;
    pTagBuf[0] = '\0';
    for (iArg = 2; iArg < iArgCt; iArg++)
    {
        ds_strnzcat(pTagBuf, pArgs[iArg] + 1, iTagBufSize);
        ds_strnzcat(pTagBuf, "\t", iTagBufSize);
    }
}

/*F********************************************************************************/
/*!
    \Function _LogConfigGetString

    \Description
        Find and get a TagField string field.  The command buffer is checked
        first, and if the entry does not exist in the command buffer the config
        file buffer is checked.

    \Input *pCommandTags    - command-line tagfield buffer
    \Input *pConfigTags     - config file tagfield buffer
    \Input *pTagName        - tagfield name to look for
    \Input *pBuffer         - buffer to copy into
    \Input iBufLen          - size of pBuffer
    \Input *pDefval         - default value if pTagName not found

    \Output
        int32_t             - result of TagFieldGetString()

    \Version 08/16/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LogConfigGetString(const char *pCommandTags, const char *pConfigTags, const char *pTagName, char *pBuffer, int32_t iBufLen, const char *pDefval)
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
    \Function _LogConfigGetNumber

    \Description
        Find and get a TagField numerical field.  The command buffer is checked
        first, and if the entry does not exist in the command buffer the config
        file buffer is checked.

    \Input *pCommandTags    - command-line tagfield buffer
    \Input *pConfigTags     - config file tagfield buffer
    \Input *pTagName        - tagfield name to look for
    \Input iDefval          - default value if pTagName not found

    \Output
        int32_t             - result of TagFieldGetNumber()

    \Version 08/16/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LogConfigGetNumber(const char *pCommandTags, const char *pConfigTags, const char *pTagName, int32_t iDefval)
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
    \Function _LogConfigGetCategory

    \Description
        Find and get a TagField category field.  The command buffer is checked
        first, and if the entry does not exist in the command buffer the config
        file buffer is checked.

    \Input *pCommandTags    - command-line tagfield buffer
    \Input *pConfigTags     - config file tagfield buffer
    \Input *pBuffer         - buffer to copy entry into
    \Input iBufLen          - size of pBuffer

    \Output
        int32_t             - result of TagFieldGetString()

    \Version 08/16/2007 (jbrookes)
*/
/********************************************************************************F*/
static int32_t _LogConfigGetCategory(const char *pCommandTags, const char *pConfigTags, char *pBuffer, int32_t iBufLen)
{
    const char *pTagField, *pOptionName;
    char strOptionName[64];

    // find option name from config file buffer
    for (pOptionName = pConfigTags; *pOptionName != '\n'; pOptionName--)
        ;
    pOptionName++;

    // get config option name
    ds_strnzcpy(strOptionName, pOptionName, pConfigTags-pOptionName);

    // if option exists in commandtag buffer, get it from there
    if ((pCommandTags != NULL) && ((pTagField = TagFieldFind(pCommandTags, strOptionName)) != NULL))
    {
        pConfigTags = pTagField;
    }
    return(TagFieldGetString(pConfigTags, pBuffer, iBufLen, ""));
}

/*F********************************************************************************/
/*!
    \Function _LogLoadConfiguration

    \Description
        Loads configuration params from config file and stores them in the global
        array _Log_Configuration[]

    \Input *pLoggerRef  - module state
    \Input *pCfgFile    - string file name 

    \Version 12/13/2004 (tchen)
*/
/********************************************************************************F*/
static void _LogLoadConfiguration(LoggerRefT *pLoggerRef, const char *pCfgFile, const char *pCommandData)
{
    const char *pConfigData;
    ConfigFileT *pConfig;
    CategoryCfgT *pCatCfg;
    LogEntryT *pLogEntry;
    LogEntryT *pLogEntrySwap;
    OutputBufferT *pOutBuf;
    char *pCfgData = (char *)calloc(1024 , sizeof(char));
    char strRotation[MAX_ROTATION_DIGITS+1];
    char strCompress[2];
    char strFormat[2];
    char strFlush[MAX_ROTATION_DIGITS+1];
    char strTag[MAX_CATEGORY_LEN+9];
    char strWriteBufferSize[MAX_WRITE_BUFFER_DIGITS+1];
    char strRotationTime[MAX_ROTATION_TIME_DIGITS + 1];
    struct stat file;

    pConfig = ConfigFileCreate();
    ConfigFileLoad(pConfig, pCfgFile, pCfgData);
    ConfigFileWait(pConfig);
    pConfigData = ConfigFileData(pConfig,"");

    pLoggerRef->iReadBufferSize = (uint32_t)_LogConfigGetNumber(pCommandData, pConfigData, "READ_BUFFER_SIZE", 4000);
    pLoggerRef->uWarnInterval = (uint32_t)_LogConfigGetNumber(pCommandData, pConfigData, "OVERFLOW_WARN_INTERVAL", 5);
    if (pLoggerRef->iReadBufferSize <= 0)
    {
        _LogError(pLoggerRef, "Invalid configuration for WRITE_QUEUE_SIZE and/or READ_BUFFER_SIZE: must be greater than 0");
        exit(LOGGER_EXIT_ERROR);
    }
    _LogConfigGetString(pCommandData, pConfigData, "ERROR_LOG", pLoggerRef->strErrorLog, sizeof(pLoggerRef->strErrorLog), "logger.log");
    _LogConfigGetString(pCommandData, pConfigData, "COMPRESS_TOOL", pLoggerRef->strCompressTool, sizeof(pLoggerRef->strCompressTool), "gzip");
    pLoggerRef->iDelimiter = _LogConfigGetNumber(pCommandData, pConfigData, "DELIMITER", 22);

    pConfigData = strstr(pConfigData,"CATEGORY_DEFAULT");
    if (pConfigData == NULL)
    {
        _LogError(pLoggerRef, "No CATEGORY_DEFAULT definition found");
        exit(LOGGER_EXIT_ERROR);
    }

    while((pConfigData = TagFieldFindNext(pConfigData, strTag, sizeof(strTag))) != NULL)
    {
        // get category info
        _LogConfigGetCategory(pCommandData, pConfigData, pCfgData, 1024);
        if (pCfgData == NULL)
        {
            break;
        }

        pCatCfg = (CategoryCfgT*)malloc(sizeof(CategoryCfgT));

        TagFieldGetDelim(pCfgData, strRotation, sizeof(strRotation), NULL, 0, ',');
        TagFieldGetDelim(pCfgData, pCatCfg->strOutFile, MAX_FILE_NAME_LEN, NULL, 1, ',');
        TagFieldGetDelim(pCfgData, strCompress, sizeof(strCompress), NULL, 2, ',');
        TagFieldGetDelim(pCfgData, strFormat, sizeof(strFormat), NULL, 3, ',');
        TagFieldGetDelim(pCfgData, strWriteBufferSize, sizeof(strWriteBufferSize), NULL, 4, ',');
        TagFieldGetDelim(pCfgData, strFlush, sizeof(strFlush), NULL, 5, ',');
        TagFieldGetDelim(pCfgData, strRotationTime, sizeof(strRotationTime), NULL, 6, ',');
        
        //override strOutFile with what was already specified in strErrorLog, we are not going to allow them to be different
        ds_snzprintf(pCatCfg->strOutFile, MAX_FILE_NAME_LEN, "%s", pLoggerRef->strErrorLog);

        if (atoi(strRotation) <= 0)
        {
            _LogError(pLoggerRef, "Invalid category configuration: rotation must be greater than 0");
            exit(LOGGER_EXIT_ERROR);
        }
        if (atoi(strRotationTime) <= 0)
        {
            _LogError(pLoggerRef, "Invalid category configuration: rotation time must be greater than 0");
            exit(LOGGER_EXIT_ERROR);
        }
        if (atoi(strWriteBufferSize) <= 0)
        {
            _LogError(pLoggerRef, "Invalid category configuration: write buffer size must be greater than 0");
            exit(LOGGER_EXIT_ERROR);
        }
        if ((strCompress[0] != '0') && (strCompress[0] != '1') && (strCompress[0] != '2'))
        {
            _LogError(pLoggerRef, "Invalid category configuration: compress flag must be 0,1 or 2");
            exit(LOGGER_EXIT_ERROR);
        }
        if ((strFormat[0] != '0') && (strFormat[0] != '1'))
        {
            _LogError(pLoggerRef, "Invalid category configuration: format flag must be 0 or 1");
            exit(LOGGER_EXIT_ERROR);
        }
        pCatCfg->uRotation = (uint32_t)atoi(strRotation);
        pCatCfg->uRotationTime = (uint32_t)atoi(strRotationTime);
        pCatCfg->iCompressFlag = strCompress[0];
        pCatCfg->iFormatFlag = strFormat[0];
        pCatCfg->uFlush = (uint32_t)atoi(strFlush);

        pCatCfg->bStartupRotate = FALSE;
        pCatCfg->uLastFlushTime = time(NULL);
        pCatCfg->uLastRotateTime = time(NULL);
        pCatCfg->pMutex = new EA::Thread::Mutex;
        pCatCfg->uCurrentOutFileSize = 0;
        strcpy(pCatCfg->strCategoryName, strstr(strTag, "_") + 1);

        pOutBuf = (OutputBufferT *)HashStrFind(pLoggerRef->pOutBufTbl, pCatCfg->strOutFile);
        if (pOutBuf == NULL) // add an entry to the outbuf hash table
        {
            pOutBuf = (OutputBufferT *)malloc(sizeof(OutputBufferT));
            pLogEntry = (LogEntryT *)malloc(sizeof(LogEntryT));
            pLogEntrySwap = (LogEntryT *)malloc(sizeof(LogEntryT));
            pLogEntry->uMaxSize = (uint32_t)atoi(strWriteBufferSize);
            pLogEntrySwap->uMaxSize = (uint32_t)atoi(strWriteBufferSize);
            pLogEntry->uCurrentUsedSize = 0;
            pLogEntrySwap->uCurrentUsedSize = 0;
            pLogEntry->pEntry = (char*)malloc(sizeof(char) * (pLogEntry->uMaxSize/INCREMENTAL_ALLOC));
            pLogEntrySwap->pEntry = (char*)malloc(sizeof(char) * (pLogEntry->uMaxSize/INCREMENTAL_ALLOC));
            pLogEntry->uCurrentSize = (sizeof(char) * (pLogEntry->uMaxSize/INCREMENTAL_ALLOC));
            pLogEntrySwap->uCurrentSize = (sizeof(char) * (pLogEntry->uMaxSize/INCREMENTAL_ALLOC));
            ds_memclr(pLogEntry->pEntry, pLogEntry->uCurrentSize);
            ds_memclr(pLogEntrySwap->pEntry, pLogEntry->uCurrentSize);

            _CreateLogDirectory(pLoggerRef, pCatCfg->strOutFile);

            pOutBuf->pLogEntry = pLogEntry;
            pOutBuf->pLogEntrySwap = pLogEntrySwap;

            // if log file exist rotate 
            if (_LogFileExists(pCatCfg->strOutFile))
            {
                pCatCfg->bStartupRotate = TRUE;
            }
            pOutBuf->pOutFileHandle = fopen(pCatCfg->strOutFile, "a");
            if (pOutBuf->pOutFileHandle == NULL)
            {
                perror(pCatCfg->strOutFile);
                exit(LOGGER_EXIT_ERROR);
            }
            if (fstat(_fileno(pOutBuf->pOutFileHandle), &file) == 0)
            {
                pCatCfg->uCurrentOutFileSize = file.st_size;
            }
            // if error file matches this out file, share file ref
            if (!strcmp(pLoggerRef->strErrorLog, pCatCfg->strOutFile))
            {
                pLoggerRef->pErrorLog = pOutBuf->pOutFileHandle;
            }
            HashStrAdd(pLoggerRef->pOutBufTbl, pCatCfg->strOutFile, pOutBuf);
        }

        pCatCfg->pOutBuf = pOutBuf;
        HashStrAdd(pLoggerRef->pCfgTbl, pCatCfg->strCategoryName, pCatCfg);

    }
    free(pCfgData);
    ConfigFileDestroy(pConfig);
}

/*F********************************************************************************/
/*!
    \Function _LogDestroyConfiguration

    \Description
        Frees the memory allocated for configuration storage

    \Input *pLoggerRef      - module state

    \Version 12/13/2004 (tchen)
*/
/********************************************************************************F*/
static void _LogDestroyConfiguration(LoggerRefT *pLoggerRef)
{
    CategoryCfgT *pRec;
    OutputBufferT *pOutBuf;

    // Free all the entries in the table.
    while ((pRec = (CategoryCfgT *)HasherFlush(pLoggerRef->pCfgTbl)) != NULL)
    {
        delete pRec->pMutex;
        free(pRec);
    }
    HasherDestroy(pLoggerRef->pCfgTbl);
    while ((pOutBuf = (OutputBufferT *)HasherFlush(pLoggerRef->pOutBufTbl)) != NULL)
    {
        fclose(pOutBuf->pOutFileHandle);
        free(pOutBuf->pLogEntry->pEntry);
        free(pOutBuf->pLogEntrySwap->pEntry);
        free(pOutBuf->pLogEntry);
        free(pOutBuf->pLogEntrySwap);
        free(pOutBuf);
    }
    HasherDestroy(pLoggerRef->pOutBufTbl);
}


/*** Public functions *************************************************************/

        
/*F********************************************************************************/
/*!
    \Function LoggerCreate

    \Description
        Create the Logger module.

    \Input iArgc        - argument count
    \Input *pArgv[]     - argument list
    \Input bStandalone  - if TRUE, operate in standalone mode

    \Ouput
        LoggerRefT *    - module state, or null on failure

    \Version 07/06/2010 (jbrookes) Split from main
*/
/********************************************************************************F*/
LoggerRefT *LoggerCreate(int32_t iArgc, const char *pArgv[], uint8_t bStandalone)
{
    EA::Thread::Thread writer;
    const char *pConfigFile; //file to read configuraion params from
    char strTagBuf[1024]; 
    LoggerRefT *pLoggerRef;

    if (pArgv[1] == NULL)
    {
        printf("%s\n","Missing command line parameter for config file");
        return(NULL);
    }
    else
    {
        pConfigFile = pArgv[1];
        if(!_LogFileExists(pConfigFile))
        {
            printf("Cannot open configuration file: %s (No such file)\n", pConfigFile);
            return(NULL);
        }            
    }
    // alloc and init logger ref
    pLoggerRef = (LoggerRefT *)malloc(sizeof(*pLoggerRef));    
    ds_memclr(pLoggerRef, sizeof(*pLoggerRef));
    pLoggerRef->bLoop = TRUE;
    pLoggerRef->bNewLine = TRUE;
    pLoggerRef->pCfgTbl = HasherCreate(50, 500);
    pLoggerRef->pOutBufTbl = HasherCreate(50, 500);

    _LogCommandLineProcess(strTagBuf, sizeof(strTagBuf), iArgc, pArgv);
    _LogLoadConfiguration(pLoggerRef, pConfigFile, strTagBuf);

    // if we don't already have a file for errors, use stderr
    if (pLoggerRef->pErrorLog == NULL)
    {
        int32_t iFileDes = open(pLoggerRef->strErrorLog, O_CREAT | O_RDWR | O_APPEND, 0644);
        if(iFileDes == -1)
        {
            perror("Cannot open error log file");
            return(NULL);
        }
        else
        {
            _dup2(iFileDes, _fileno(stderr));
            _close(iFileDes);
        }
    }

    _LogError(pLoggerRef, "logger error channel available");

    // start reader/writer threads
    pLoggerRef->pCondMutex = new EA::Thread::Mutex;
    pLoggerRef->pHashTblMutex = new EA::Thread::Mutex;
    pLoggerRef->pHaveEntriesForWrite = new EA::Thread::Condition;
    writer.Begin(_LogWriterFunc, pLoggerRef);    
    
    // return ref to caller
    return(pLoggerRef);
}

/*F********************************************************************************/
/*!
    \Function LoggerDestroy

    \Description
        Destroy the Logger

    \Input *pLoggerRef  - module state

    \Version 07/06/2010 (jbrookes) Split from main
*/
/********************************************************************************F*/
void LoggerDestroy(LoggerRefT *pLoggerRef)
{
    uint32_t uCount;

    /* signal writer thread to exit (in the standalone version, this is done
       by the reader thread detecting stdin is closed) */
    pLoggerRef->bLoop = FALSE;

    // wake up logger out of wait on condition vairable if it is sleeping there
    pLoggerRef->pHaveEntriesForWrite->Signal();

    // wait for thread to exit
    for (uCount = 1; pLoggerRef->iWriterStatus == 0; uCount += 1)
    {
        //$$ hack - should not have to do this, but sometimes we get stuck, so let it try again
        if ((uCount % 100) == 0)
        {
            pLoggerRef->pHaveEntriesForWrite->Signal();
        }
        NetConnSleep(10);
    }

    _LogDestroyConfiguration(pLoggerRef);
    delete pLoggerRef->pCondMutex;
    delete pLoggerRef->pHashTblMutex;
    delete pLoggerRef->pHaveEntriesForWrite;
    #ifdef __linux__
    // under Windows, winmain handles this for us
    _close(_fileno(stderr));
    #endif
}

/*F********************************************************************************/
/*!
    \Function LoggerTryRotate

    \Description
        Will rotate the file if the file max size is reached or rotation time is reached
        or on startup

    \Input *pLoggerRef  - logger ref

    \Version 4/18/2018 (tcho)
*/
/********************************************************************************F*/
void LoggerTryRotate(LoggerRefT* pLoggerRef)
{
    FILE *pOutputFile;
    CategoryCfgT* pCatCfg;
    char strReopenFlags[2] = "w";
    char strNewFile[MAX_FILE_NAME_LEN + 25];
    uint8_t uMonth, uDay, uHour, uMin, uSec;
    uint32_t uYear, uMillis, uEpoch;
    #ifdef __linux__
    int32_t iPid;
    #endif

    pLoggerRef->pHashTblMutex->Lock();
    HasherEnumInit(pLoggerRef->pCfgTbl);
    while ((pCatCfg = (CategoryCfgT *)HasherEnum(pLoggerRef->pCfgTbl, NULL)) != NULL)
    {
        #ifdef __linux__
        waitpid((pid_t)-1, &iPid, WNOHANG); // pickup any zombie child processes
        #endif

        // rotate if we reach log file size of 1MB or log file is older than the rotation time
        if (pCatCfg->bStartupRotate ||
           ((pCatCfg->uRotation * 1000) <= pCatCfg->uCurrentOutFileSize) ||
           ((difftime(time(NULL), pCatCfg->uLastRotateTime) / SEC_IN_DAY) >= pCatCfg->uRotationTime))
        {
            pCatCfg->pMutex->Lock();

            pCatCfg->bStartupRotate = FALSE;
            pCatCfg->uLastRotateTime = time(NULL);
            fclose(pCatCfg->pOutBuf->pOutFileHandle);

            _LogGetCurrentTime(&uYear, &uMonth, &uDay, &uHour, &uMin, &uSec, &uMillis, &uEpoch);

            if (pCatCfg->iCompressFlag == '2')
            {
                if (strcmp(pLoggerRef->strCompressTool, "bzip2") == 0)
                {
                    sprintf(strNewFile, "%s-%02u-%02u-%02uT%02u-%02u-%02u-%03u.bz2", pCatCfg->strOutFile, uYear, uMonth, uDay, uHour, uMin, uSec, uMillis);
                }
                else if (strcmp(pLoggerRef->strCompressTool, "gzip") == 0)
                {
                    sprintf(strNewFile, "%s-%02u-%02u-%02uT%02u-%02u-%02u-%03u.gz", pCatCfg->strOutFile, uYear, uMonth, uDay, uHour, uMin, uSec, uMillis);
                }
            }
            else
            {
                sprintf(strNewFile, "%s-%02u-%02u-%02uT%02u-%02u-%02u-%03u", pCatCfg->strOutFile, uYear, uMonth, uDay, uHour, uMin, uSec, uMillis);
            }

            if (_LogFileExists(pCatCfg->strOutFile))
            {
                if (rename(pCatCfg->strOutFile, strNewFile) >= 0)
                {
                    #ifdef __linux__
                    if (pCatCfg->iCompressFlag == '1')
                    {
                        iPid = fork();
                        if (iPid == 0) //child
                        {
                            if (execlp(pLoggerRef->strCompressTool, pLoggerRef->strCompressTool, strNewFile, (char*)NULL) == -1)
                            {
                                perror("execlp");
                                exit(LOGGER_EXIT_ERROR);
                            }
                            exit(LOGGER_EXIT_OK);
                        }
                    }
                    #endif
                }
                else
                {
                    // since we couldn't rename, try reopening file for append
                    strReopenFlags[0] = 'a';
                }
            }

            // reopen base logfile
            pOutputFile = fopen(pCatCfg->strOutFile, strReopenFlags);
            if (pOutputFile == NULL)
            {
                exit(LOGGER_EXIT_ERROR);
            }
            // if our error logging was going to this file, point to the new file
            if (pLoggerRef->pErrorLog == pCatCfg->pOutBuf->pOutFileHandle)
            {
                pLoggerRef->pErrorLog = pOutputFile;
            }

            pCatCfg->pOutBuf->pOutFileHandle = pOutputFile;
            pCatCfg->uCurrentOutFileSize = 0;

            pCatCfg->pMutex->Unlock();
        }
    }
    pLoggerRef->pHashTblMutex->Unlock();
}

/*F********************************************************************************/
/*!
    \Function LoggerWrite

    \Description
        Add a line of text to the log

    \Input *pLoggerRef  - module state
    \Input *pText       - pointer to text to add
    \Input iTextLen     - length of text to add

    \Version 07/06/2010 (jbrookes)
*/
/********************************************************************************F*/
int32_t LoggerWrite(LoggerRefT *pLoggerRef, const char *pText, int32_t iTextLen)
{
    int32_t iResult = 0;
    if (iTextLen <= 0)
    {
        iTextLen = (int32_t)strlen(pText);
    } 
    _LogAddEntry(pLoggerRef, pText, iTextLen);
    iResult = 1;
    return(iResult);
}
