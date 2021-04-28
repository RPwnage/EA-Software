///////////////////////////////////////////////////////////////////////////////
// IGOLogger.cpp
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "IGOLogger.h"

#if defined(ORIGIN_PC)

#include "resource.h"

#include <Windows.h>
#include <WinError.h>
#include <Shlobj.h>
#include <Shlwapi.h>
#include "eathread/eathread_futex.h"
#include "Helpers.h"

namespace OriginIGO {

    static wchar_t* LOGFILE = L"%s\\Origin\\Logs\\IGO_Log.%s_%i.txt";
#ifndef USE_TRACE_FOR_LOGGING
    static char* LOGFORMAT = "%s\t%s\t%d\t%20s:%5d\t\t%s\r\n";
#else
    static const char* LOGFORMAT = "%s - %s - %d - %s: %d - %s\n";
#endif

    static const size_t AsyncMaxMsgSize = 512;
    static const int AsyncMsgQueueSize = 1024;
    static const int AsyncThreadSleepInMS = 2000;
    static const char* AsyncMsgQueueOverflowWarning = "AsyncMsgQueue Overflow";

    extern HINSTANCE gInstDLL;
    static EA::Thread::Futex gIGOLoggerMutex;
    static EA::Thread::Futex gIGOLoggerAsyncMutex;

    IGOLogger *IGOLogger::instance()
    {
        static IGOLogger logger;
        return &logger;
    }

    IGOLogger::IGOLogger() :
        mAsyncThread(NULL),
        mLogHandle(NULL),
        mPopIndex(0),
        mPushIndex(0),
        mAsyncMsgQueue(NULL),
        mEnabled(false),
        mInAsyncMode(false)
    {
        // Allocate one big pool so that it's not too slow when cleaning up in debug
        int poolSize = sizeof(char*) * AsyncMsgQueueSize + sizeof(char) * AsyncMsgQueueSize * AsyncMaxMsgSize;
        char* pool = static_cast<char*>(malloc(poolSize));
        ZeroMemory(pool, poolSize);
        
        mAsyncMsgQueue = reinterpret_cast<char**>(pool);
        
        char* msgPtr = reinterpret_cast<char*>(mAsyncMsgQueue + AsyncMsgQueueSize);
        for (int idx = 0; idx < AsyncMsgQueueSize; ++idx)
        {
            mAsyncMsgQueue[idx] = msgPtr;
            msgPtr += AsyncMaxMsgSize;
        }
    }


    IGOLogger::~IGOLogger()
    {
        // Only clean up if the async mode was stopped properly, otherwise leak
        if (!mInAsyncMode)
        {
            free(mAsyncMsgQueue);
            mAsyncMsgQueue = NULL;
        }

        if (mLogHandle)
        {
            CloseHandle(mLogHandle);
            mLogHandle = NULL;
        }
    }

    void IGOLogger::startAsyncMode()
    {
        EA::Thread::AutoFutex m(gIGOLoggerMutex);

        if (!mInAsyncMode)
        {
            mAsyncThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)asyncRunTrampoline, (void*)this, 0, NULL);
            mInAsyncMode = mAsyncThread != NULL;

            if (!mAsyncThread)
                IGOLogWarn("Unable to start logger async mode");
        }
    }

    void IGOLogger::stopAsyncMode()
    {
        EA::Thread::AutoFutex m(gIGOLoggerMutex);

        if (mInAsyncMode)
        {
            mInAsyncMode = false;
            if (::WaitForSingleObject(mAsyncThread, AsyncThreadSleepInMS * 2) != WAIT_OBJECT_0)
            {
                // Don't terminate the thread, could cause a deadlock when the process terminates (since we keep on recording logs synchronuously
                // after stopping the async mode)
            }
        }
    }

    void IGOLogger::asyncRunTrampoline(void* param)
    {
        return static_cast<IGOLogger*>(param)->asyncRun();
    }

    void IGOLogger::asyncRun()
    {
        char msg[AsyncMaxMsgSize];

        while (mInAsyncMode)
        {
            // Flush pending messages
            while (popAsyncMsg(msg, AsyncMaxMsgSize))
            {
                EA::Thread::AutoFutex m(gIGOLoggerMutex);

                DWORD writeSize = 0;
                WriteFile(mLogHandle, msg, static_cast<DWORD>(strlen(msg)), &writeSize, NULL);
            }

            FlushFileBuffers(mLogHandle);

            // let's take a breather
            Sleep(AsyncThreadSleepInMS);
        }

        // Flush pending messages
        while (popAsyncMsg(msg, AsyncMaxMsgSize))
        {
            EA::Thread::AutoFutex m(gIGOLoggerMutex);

            DWORD writeSize = 0;
            WriteFile(mLogHandle, msg, static_cast<DWORD>(strlen(msg)), &writeSize, NULL);
        }

        FlushFileBuffers(mLogHandle);

        CloseHandle(mAsyncThread);
        mAsyncThread = NULL;
    }

    bool IGOLogger::popAsyncMsg(char* msg, size_t maxLen)
    {
        EA::Thread::AutoFutex m(gIGOLoggerAsyncMutex);

        if (mPopIndex != mPushIndex || mAsyncMsgQueue[mPopIndex][0] != '\0')
        {
            strncpy(msg, mAsyncMsgQueue[mPopIndex], maxLen);
            msg[maxLen-1] = '\0';
            mAsyncMsgQueue[mPopIndex][0] = '\0';

            ++mPopIndex;
            if (mPopIndex == AsyncMsgQueueSize)
                mPopIndex = 0;

            return true;
        }

        return false;
    }

    bool IGOLogger::pushAsyncMsg(const char* msg)
    {
        if (msg && msg[0])
        {
            EA::Thread::AutoFutex m(gIGOLoggerAsyncMutex);

            if (mPushIndex != mPopIndex || mAsyncMsgQueue[mPushIndex][0] == '\0')
            {
                strncpy(mAsyncMsgQueue[mPushIndex], msg, AsyncMaxMsgSize);
                mAsyncMsgQueue[mPushIndex][AsyncMaxMsgSize-1] = '\0';

                ++mPushIndex;
                if (mPushIndex == AsyncMsgQueueSize)
                    mPushIndex = 0;

                return true;
            }
        }

        return false;
    }

    void IGOLogger::enableLogging(bool enable)
    {
        mEnabled = enable;
    }

    void IGOLogger::info(const char *file, int line, bool alwaysLogMessage, const char *fmt, ...)
    {
#ifndef USE_TRACE_FOR_LOGGING
        if (!mEnabled && !alwaysLogMessage)
            return;

        EA::Thread::AutoFutex m(gIGOLoggerMutex);
        checkLogFile();

        if (mLogHandle == INVALID_HANDLE_VALUE)
            return;
#endif

        va_list args;
        va_start (args, fmt);
        writeLog("INFO", file, line, fmt, args);
        va_end (args);
    }

    void IGOLogger::infoWithArgs(const char *file, int line, bool alwaysLogMessage, const char *fmt, va_list arglist)
    {
#ifndef USE_TRACE_FOR_LOGGING
        if (!mEnabled && !alwaysLogMessage)
            return;

        EA::Thread::AutoFutex m(gIGOLoggerMutex);
        checkLogFile();

        if (mLogHandle == INVALID_HANDLE_VALUE)
            return;
#endif

        writeLog("INFO", file, line, fmt, arglist);
    }

    void IGOLogger::warn(const char *file, int line, bool alwaysLogMessage, const char *fmt, ...)
    {
#ifndef USE_TRACE_FOR_LOGGING
        if (!mEnabled && !alwaysLogMessage)
            return;

        EA::Thread::AutoFutex m(gIGOLoggerMutex);
        checkLogFile();

        if (mLogHandle == INVALID_HANDLE_VALUE)
            return;
#endif

        va_list args;
        va_start (args, fmt);
        writeLog("WARN", file, line, fmt, args);
        va_end (args);
    }

    void IGOLogger::warnWithArgs(const char *file, int line, bool alwaysLogMessage, const char *fmt, va_list arglist)
    {
#ifndef USE_TRACE_FOR_LOGGING
        if (!mEnabled && !alwaysLogMessage)
            return;

        EA::Thread::AutoFutex m(gIGOLoggerMutex);
        checkLogFile();

        if (mLogHandle == INVALID_HANDLE_VALUE)
            return;
#endif

        writeLog("WARN", file, line, fmt, arglist);
    }

    void IGOLogger::debug(const char *file, int line, const char *fmt, ...)
    {
        // only log "debug" in the debug build
#ifdef _DEBUG
#ifndef USE_TRACE_FOR_LOGGING
        if (!mEnabled)
            return;

        EA::Thread::AutoFutex m(gIGOLoggerMutex);
        checkLogFile();

        if (mLogHandle == INVALID_HANDLE_VALUE)
            return;
#endif

        va_list args;
        va_start (args, fmt);
        writeLog("DEBUG", file, line, fmt, args);
        va_end (args);
#endif
    }

    const char * IGOLogger::timeString()
    {
        static char szTime[128];
        SYSTEMTIME time;
        GetLocalTime(&time);

        GetTimeFormatA(LOCALE_USER_DEFAULT, 0, &time, "hh:mm:ss tt", szTime, sizeof(szTime)/sizeof(szTime[0]));
        return szTime;
    }

    const WCHAR *IGOLogger::appName()
    {
        // NOTE: we can't use strings.h because we don't want to depend on msvcrt.lib
        static WCHAR szExePath[MAX_PATH] = {0};
        DWORD size = GetModuleFileName(GetModuleHandle(NULL), szExePath, MAX_PATH);
        int i = size - 1;

        // search backwards for path delimiter
        while (i >= 0 && szExePath[i] != '\\')
        {
            // remove .exe
            if (szExePath[i] == '.')
                szExePath[i] = 0;
            i--;
        }

        if (i >= 0 && szExePath[i] == '\\')
            i++;

        return ((i >= 0) ? (&szExePath[i]) : (NULL) );
    }

    const char *IGOLogger::basename(const char *path)
    {
        // NOTE: we can't use strings.h because we don't want to depend on msvcrt.lib

        // path must be null terminated
        const char *ptr = path;
        while (*ptr)
            ptr++;

        __int64 i = ptr - path - 1;

        // search backwards for path delimiter
        while (i >= 0 && path[i] != '\\')
            i--;

        if (i >= 0 && path[i] == '\\')
            i++;

        return ((i >= 0) ? (&path[i]) : (NULL) );
    }

    HRESULT IGOLogger::writeParameters(STRSAFE_LPSTR string, int stringSizeInBytes, const char *fmt, ...)
    {
        va_list args;
        va_start (args, fmt);
        HRESULT hr = StringCbVPrintfA(string, stringSizeInBytes, fmt, args);
        va_end (args);
        return hr;
    }
    
    HRESULT IGOLogger::writeParameters(STRSAFE_LPSTR string, int stringSizeInBytes, const char *fmt, va_list arglist)
    {
        HRESULT hr = StringCbVPrintfA(string, stringSizeInBytes, fmt, arglist);
        return hr;
    }    
    
    void IGOLogger::writeLog(const char *type, const char *file, int line, const char *fmt, va_list args)
    {
#ifndef USE_TRACE_FOR_LOGGING
        // NOTE: we can't use strings.h because we don't want to depend on msvcrt.lib
        IGO_ASSERT(mLogHandle);
#endif

        char msg[4096] = {0};
        if (STRSAFE_E_INVALID_PARAMETER != writeParameters(msg, sizeof(msg), fmt, args))
        {
            char log[2 * 4096] = {0};
            const char *baseName = basename(file); 
            if (STRSAFE_E_INVALID_PARAMETER != writeParameters(log, sizeof(log), LOGFORMAT, type, timeString(), GetCurrentThreadId(), baseName != NULL ? baseName : "NULL", line, msg))
            {
                size_t size = 0;
                if (SUCCEEDED(StringCbLengthA(log, sizeof(log), &size)))
                {
#ifndef USE_TRACE_FOR_LOGGING
                    if (mInAsyncMode)
                    {
                        if (!pushAsyncMsg(log))
                        {
                            DWORD writeSize = 0;

                            static bool warnedAboutMsgQueueOverflow = false;
                            if (!warnedAboutMsgQueueOverflow)
                            {
                                warnedAboutMsgQueueOverflow = true;
                                WriteFile(mLogHandle, AsyncMsgQueueOverflowWarning, static_cast<DWORD>(strlen(AsyncMsgQueueOverflowWarning)), &writeSize, NULL);
                            }

                            WriteFile(mLogHandle, log, static_cast<DWORD>(size), &writeSize, NULL);
                            FlushFileBuffers(mLogHandle);
                        }
                    }

                    else
                    {
                        DWORD writeSize = 0;
                        WriteFile(mLogHandle, log, static_cast<DWORD>(size), &writeSize, NULL);

                        FlushFileBuffers(mLogHandle);
                    }
#else
                    IGO_TRACE(log);
#endif
                }
            }
        }
    }

    void IGOLogger::checkLogFile()
    {
        if (!mEnabled)
            return;

        if (mLogHandle)
            return;

        // determine the location of the log file
        WCHAR commonDataPath[MAX_PATH] = {0};
        if (GetCommonDataPath(commonDataPath))
        {
            const WCHAR *appname = appName();
            wsprintf(mLogFile, LOGFILE, commonDataPath,  appname != NULL ? appname : L"NULL", GetCurrentProcessId());
            mLogHandle = CreateFile(mLogFile, (GENERIC_READ | GENERIC_WRITE), FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            IGO_TRACEF("mLogHandle = %p", mLogHandle);
            if (mLogHandle != INVALID_HANDLE_VALUE)
            {
                DWORD bytesWritten = 0;
    
                // write header
                char tmp[256];
                int tmpSize = _snprintf(tmp,sizeof(tmp), "Process Information\r\n");

                // header
                WriteFile(mLogHandle, tmp, tmpSize, &bytesWritten, NULL);
            
                tmpSize = _snprintf(tmp, sizeof(tmp), "    PID: %d\r\n", GetCurrentProcessId());
                WriteFile(mLogHandle, tmp, tmpSize, &bytesWritten, NULL);
                
                WCHAR szExePath[MAX_PATH];
                GetModuleFileName(GetModuleHandle(NULL), szExePath, MAX_PATH);
                tmpSize = _snprintf(tmp, sizeof(tmp), "    EXE: %S\r\n", szExePath);
                WriteFile(mLogHandle, tmp, tmpSize, &bytesWritten, NULL);
                
                // format the date
                char szTime[128];
                SYSTEMTIME time;
                GetLocalTime(&time);
                int size = GetDateFormatA(NULL, 0, &time, "ddd',' MMM dd yyyy ", szTime, sizeof(szTime)/sizeof(szTime[0]));
                if (size > 0)
                    GetTimeFormatA(NULL, 0, &time, "hh:mm:ss tt", &szTime[size - 1], sizeof(szTime)/sizeof(szTime[0]) - size);
                else
                    szTime[0] = 0;

                tmpSize = _snprintf(tmp, sizeof(tmp), "STARTED: %s\r\n", szTime);
                WriteFile(mLogHandle, tmp, tmpSize, &bytesWritten, NULL);
                            
            }
        }
        else
        {
            mLogHandle = INVALID_HANDLE_VALUE;
            mLogFile[0] = 0;
        }
    }
}
#elif defined(ORIGIN_MAC) // MACOSX

#include <stdarg.h>
#include <fcntl.h>

#include <MacWindows.h>
#include "eathread/eathread_futex.h"

static const char* LOGFILE = "%s/Library/Application Support/Origin/Logs/%s_Log.%s_%i.txt";
#ifndef USE_TRACE_FOR_LOGGING
static const char* LOGFORMAT = "%s\t%s\t%d\t%s:%d\t\t%s\n";
#else
static const char* TRACE_LOGFORMAT = "%s - %s - %d - %s: %d - %s\n";
#endif

static const size_t AsyncMaxMsgSize = 512;
static const int AsyncMsgQueueSize = 1024;
static const int AsyncThreadSleepInMS = 2000;
static const char* AsyncMsgQueueOverflowWarning = "AsyncMsgQueue Overflow";

static EA::Thread::Futex gIGOLoggerMutex;
static EA::Thread::Futex gIGOLoggerAsyncMutex;

namespace OriginIGO
{
    IGOLogger* IGOLogger::mLogger = NULL;
    
    IGOLogger *IGOLogger::instance()
    {
        static IGOLogger nullLogger;
        
        if (mLogger)
            return mLogger;
        
        else
            return &nullLogger;
    }

    void IGOLogger::setInstance(IGOLogger* logger)
    {
        if (mLogger)
            delete mLogger;
        
        mLogger = logger;
    }
    
    IGOLogger::IGOLogger() :
        mAsyncThread(NULL),
        mNullLogger(true),
        mLogHandle(NULL),
        mPopIndex(0),
        mPushIndex(0),
        mAsyncMsgQueue(NULL),
        mEnabled(false),
        mInAsyncMode(false)
    {
    
    }
    
    IGOLogger::IGOLogger(const char* filePrefix) :
        mAsyncThread(NULL),
        mNullLogger(false),
        mLogHandle(NULL),
        mPopIndex(0),
        mPushIndex(0),
        mAsyncMsgQueue(NULL),
        mEnabled(false),
        mInAsyncMode(false)
    {
        memset(mLogFile, 0, sizeof(mLogFile));
        
        const char* prefix = filePrefix ? filePrefix : "IGO";
        snprintf(mFilePrefix, sizeof(mFilePrefix), "%s", prefix);
        
        // Don't use any C++ allocation here since the logger is used by the IGOLoader, ie used before the game is settled
        // -> we could have C++ allocation conflicts that would cause the game to crash (because of ReleaseDebug builds and the like)
        int poolSize = sizeof(char*) * AsyncMsgQueueSize + sizeof(char) * AsyncMsgQueueSize * AsyncMaxMsgSize;
        char* pool = static_cast<char*>(malloc(poolSize));
        ZeroMemory(pool, poolSize);
        
        mAsyncMsgQueue = reinterpret_cast<char**>(pool);
        
        char* msgPtr = reinterpret_cast<char*>(mAsyncMsgQueue + AsyncMsgQueueSize);
        for (int idx = 0; idx < AsyncMsgQueueSize; ++idx)
        {
            mAsyncMsgQueue[idx] = msgPtr;
            msgPtr += AsyncMaxMsgSize;
        }
    }


    IGOLogger::~IGOLogger()
    {
        if (mNullLogger)
            return;
        
        // Only clean up if the async mode was stopped properly, otherwise leak
        if (!mInAsyncMode)
        {
            free(mAsyncMsgQueue);
            mAsyncMsgQueue = NULL;
        }
        
        if (mLogHandle)
        {
            close(mLogHandle);
            mLogHandle = NULL;
        }
    }

    void IGOLogger::startAsyncMode()
    {
        if (mNullLogger)
            return;
        
        EA::Thread::AutoFutex m(gIGOLoggerMutex);
        
        if (!mInAsyncMode)
        {
            int result = pthread_create(&mAsyncThread, NULL, asyncRunTrampoline, this);
            mInAsyncMode = result == 0 && mAsyncThread != NULL;
            
            if (!mInAsyncMode)
                IGOLogWarn("Unable to start logger async mode (err=%d)", result);
        }
    }
    
    void IGOLogger::stopAsyncMode()
    {
        if (mNullLogger)
            return;
        
        EA::Thread::AutoFutex m(gIGOLoggerMutex);
        
        if (mInAsyncMode)
        {
            pthread_t h = mAsyncThread;
            mInAsyncMode = false;
            
            void* notUsed = NULL;
            pthread_join(h, &notUsed);
            
            Sleep(AsyncThreadSleepInMS * 2);
        }
    }
    
    void* IGOLogger::asyncRunTrampoline(void* param)
    {
        static_cast<IGOLogger*>(param)->asyncRun();
        return 0;
    }
    
    void IGOLogger::asyncRun()
    {
        char msg[AsyncMaxMsgSize];
        
        while (mInAsyncMode)
        {
            // Flush pending messages
            while (popAsyncMsg(msg, AsyncMaxMsgSize))
            {
                EA::Thread::AutoFutex m(gIGOLoggerMutex);
                write(mLogHandle, msg, strlen(msg));
            }
            
            // let's take a breather
            Sleep(AsyncThreadSleepInMS);
        }
        
        // Flush pending messages
        while (popAsyncMsg(msg, AsyncMaxMsgSize))
        {
            EA::Thread::AutoFutex m(gIGOLoggerMutex);
            write(mLogHandle, msg, strlen(msg));
        }
        
        mAsyncThread = NULL;
    }
    
    bool IGOLogger::popAsyncMsg(char* msg, size_t maxLen)
    {
        if (mNullLogger)
            return false;
        
        EA::Thread::AutoFutex m(gIGOLoggerAsyncMutex);
        
        if (mPopIndex != mPushIndex || mAsyncMsgQueue[mPopIndex][0] != '\0')
        {
            strncpy(msg, mAsyncMsgQueue[mPopIndex], maxLen);
            msg[maxLen-1] = '\0';
            mAsyncMsgQueue[mPopIndex][0] = '\0';
            
            ++mPopIndex;
            if (mPopIndex == AsyncMsgQueueSize)
                mPopIndex = 0;
            
            return true;
        }
        
        return false;
    }
    
    bool IGOLogger::pushAsyncMsg(const char* msg)
    {
        if (mNullLogger)
            return false;
        
        if (msg && msg[0])
        {
            EA::Thread::AutoFutex m(gIGOLoggerAsyncMutex);
            
            if (mPushIndex != mPopIndex || mAsyncMsgQueue[mPushIndex][0] == '\0')
            {
                strncpy(mAsyncMsgQueue[mPushIndex], msg, AsyncMaxMsgSize);
                mAsyncMsgQueue[mPushIndex][AsyncMaxMsgSize-1] = '\0';
                
                ++mPushIndex;
                if (mPushIndex == AsyncMsgQueueSize)
                    mPushIndex = 0;
                
                return true;
            }
        }
        
        return false;
    }
    
    void IGOLogger::enableLogging(bool enable)
    {
        mEnabled = enable;
    }

    void IGOLogger::info(const char *file, int line, bool alwaysLogMessage, const char *fmt, ...)
    {
        if (mNullLogger)
            return;
        
#ifndef USE_TRACE_FOR_LOGGING
        if (!mEnabled && !alwaysLogMessage)
            return;
        
        EA::Thread::AutoFutex m(gIGOLoggerMutex);
        checkLogFile();
    
        if (!mLogHandle)
            return;
#endif
        
        va_list args;
        va_start (args, fmt);
        writeLog("INFO", file, line, fmt, args);
        va_end (args);
    
    }

    void IGOLogger::infoWithArgs(const char *file, int line, bool alwaysLogMessage, const char *fmt, va_list arglist)
    {
        if (mNullLogger)
            return;
        
#ifndef USE_TRACE_FOR_LOGGING
        if (!mEnabled && !alwaysLogMessage)
            return;
        
        EA::Thread::AutoFutex m(gIGOLoggerMutex);
        checkLogFile();
        
        if (!mLogHandle)
            return;
#endif
        
        writeLog("INFO", file, line, fmt, arglist);
    }

    void IGOLogger::warn(const char *file, int line, bool alwaysLogMessage, const char *fmt, ...)
    {
        if (mNullLogger)
            return;
        
#ifndef USE_TRACE_FOR_LOGGING
        if (!mEnabled && !alwaysLogMessage)
            return;
    
        EA::Thread::AutoFutex m(gIGOLoggerMutex);
        checkLogFile();
    
        if (!mLogHandle)
            return;
#endif
        
        va_list args;
        va_start (args, fmt);
        writeLog("WARN", file, line, fmt, args);
        va_end (args);
    }

    void IGOLogger::warnWithArgs(const char *file, int line, bool alwaysLogMessage, const char *fmt, va_list arglist)
    {
        if (mNullLogger)
            return;
        
#ifndef USE_TRACE_FOR_LOGGING
        if (!mEnabled && !alwaysLogMessage)
            return;
        
        EA::Thread::AutoFutex m(gIGOLoggerMutex);
        checkLogFile();
        
        if (!mLogHandle)
            return;
#endif
        
        writeLog("WARN", file, line, fmt, arglist);
    }

    void IGOLogger::debug(const char *file, int line, const char *fmt, ...)
    {
        if (mNullLogger)
            return;
        
#ifdef _DEBUG
#ifndef USE_TRACE_FOR_LOGGING
        if (!mEnabled)
            return;

        EA::Thread::AutoFutex m(gIGOLoggerMutex);
        checkLogFile();
    
        if (!mLogHandle)
            return;
#endif
        
        va_list args;
        va_start (args, fmt);
        writeLog("DEBUG", file, line, fmt, args);
        va_end (args);
#endif
    }
    
    const char * IGOLogger::timeString()
    {
        static char szTime[128];
        time_t t;
        time(&t);
        struct tm* p = localtime(&t);
        
        snprintf(szTime, sizeof(szTime), "%02d:%02d:%02d", p->tm_hour, p->tm_min, p->tm_sec);
        return szTime;
    }
    
    const char *IGOLogger::appName()
    {
        static char szExePath[MAX_PATH] = {0};
        if (Origin::Mac::GetModuleFileName(NULL, szExePath, sizeof(szExePath)))
        {
            char* fpStart = strrchr(szExePath, '/');
            return (fpStart && *fpStart) ? fpStart + 1: szExePath;
        }
        
        return NULL;
    }
    
    const char *IGOLogger::basename(const char *path)
    {
        char* fpStart = path ? strrchr(path, '/') : NULL;
        return (fpStart && *fpStart) ? fpStart + 1 : path;
    }
    
    void IGOLogger::writeLog(const char *type, const char *file, int line, const char *fmt, va_list args)
    {
#ifndef USE_TRACE_FOR_LOGGING
        IGO_ASSERT(mLogHandle);
#endif
        
        char msg[1024];
        vsnprintf(msg, sizeof(msg), fmt, args);
        
        char html[1024];
        const char *baseName = basename(file);
        
#ifndef USE_TRACE_FOR_LOGGING
        int size = snprintf(html, sizeof(html), LOGFORMAT, type, timeString(), GetCurrentThreadId(), baseName != NULL ? baseName : "NULL", line, msg);
        write(mLogHandle, html, size);
        
        if (mInAsyncMode)
        {
            if (!pushAsyncMsg(html))
            {
                static bool warnedAboutMsgQueueOverflow = false;
                if (!warnedAboutMsgQueueOverflow)
                {
                    warnedAboutMsgQueueOverflow = true;
                    write(mLogHandle, AsyncMsgQueueOverflowWarning, strlen(AsyncMsgQueueOverflowWarning));
                }
                
                write(mLogHandle, html, size);
            }
        }
        
        else
        {
            write(mLogHandle, html, size);
        }
#else
        snprintf(html, sizeof(html), TRACE_LOGFORMAT, type, timeString(), GetCurrentThreadId(), baseName != NULL ? baseName : "NULL", line, msg);
        IGO_TRACE(html);
#endif
    }
    
    
    void IGOLogger::checkLogFile()
    {
        if (!mEnabled)
            return;
        
        if (mLogHandle)
            return;
        
        // determine the location of the log file
        char homeDir[MAX_PATH];
        if (Origin::Mac::GetHomeDirectory(homeDir, sizeof(homeDir)) == 0)
        {
            const char* appname = appName();
            snprintf(mLogFile, sizeof(mLogFile), LOGFILE, homeDir, mFilePrefix, appname != NULL ? appname : "NULL", GetCurrentProcessId());
            
            mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
            mLogHandle = open(mLogFile, O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, mode);
            IGO_TRACEF("mLogHandle = %d", mLogHandle);
            if (mLogHandle)
            {
                // write header
                char tmp[256];
                int tmpSize = snprintf(tmp,sizeof(tmp), "Process Information\n");
                write(mLogHandle, tmp, tmpSize);
                
                tmpSize = snprintf(tmp, sizeof(tmp), "PID: %d\n", getpid());
                write(mLogHandle, tmp, tmpSize);
                
                if (!Origin::Mac::GetModuleFileName(NULL, homeDir, sizeof(homeDir)))
                    homeDir[0] = '\0';
                tmpSize = snprintf(tmp, sizeof(tmp), "EXE: %s\n", homeDir);
                write(mLogHandle, tmp, tmpSize);
                
                time_t t;
                time(&t);
                struct tm* p = localtime(&t);
				char mbstr[100];
				strftime(mbstr, sizeof(mbstr), "%c", p);
                tmpSize = snprintf(tmp, sizeof(tmp), "STARTED: %s", mbstr);
                write(mLogHandle, tmp, tmpSize);
                
                tmpSize = snprintf(tmp, sizeof(tmp), "TYPE\tTIME\t\tTID\tFILE\t\t\t\tMESSAGE\n\n");
                write(mLogHandle, tmp, tmpSize);
            }
        }
        
        else
        {
            mLogFile[0] = 0;
            IGO_TRACE("Unable to retrieve home directory - No logging allowed");
        }
    }
}
#endif // MAC OSX
