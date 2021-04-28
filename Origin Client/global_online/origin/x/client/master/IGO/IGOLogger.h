///////////////////////////////////////////////////////////////////////////////
// IGOLogger.h
// 
// Copyright (c) 2012 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef IGOLOGGER_H
#define IGOLOGGER_H

#include "IGOTrace.h"

namespace OriginIGO {
    #if !defined(ORIGIN_MAC)

#ifdef _DEBUG
//#define USE_TRACE_FOR_LOGGING
#endif

    #define IGOLogInfo(fmt, ...) IGOLogger::instance()->info(__FILE__, __LINE__, false, fmt, __VA_ARGS__)
    #define IGOForceLogInfo(fmt, ...) IGOLogger::instance()->info(__FILE__, __LINE__, true, fmt, __VA_ARGS__)
    #define IGOForceLogInfoOnce(fmt, ...) \
    { \
        static bool alreadyLogged = false; \
        if (!alreadyLogged) \
        { \
            alreadyLogged = true; \
            IGOLogger::instance()->info(__FILE__, __LINE__, true, fmt, __VA_ARGS__); \
        } \
    }
    #define IGOLogWarn(fmt, ...) IGOLogger::instance()->warn(__FILE__, __LINE__, false, fmt, __VA_ARGS__)
    #define IGOForceLogWarn(fmt, ...) IGOLogger::instance()->warn(__FILE__, __LINE__, true, fmt, __VA_ARGS__)
    #define IGOForceLogWarnOnce(fmt, ...) \
    { \
        static bool alreadyLogged = false; \
        if (!alreadyLogged) \
        { \
            alreadyLogged = true; \
            IGOLogger::instance()->warn(__FILE__, __LINE__, true, fmt, __VA_ARGS__); \
        } \
    }
    #define IGOLogDebug(fmt, ...) IGOLogger::instance()->debug(__FILE__, __LINE__, fmt, __VA_ARGS__)
    #define IGOLogEnabled() IGOLogger::instance()->enabled()

    #else    

#ifdef _DEBUG
//#define USE_TRACE_FOR_LOGGING
#endif
    
    #define IGOLogInfo(...) IGOLogger::instance()->info(__FILE__, __LINE__, false, __VA_ARGS__)
    #define IGOForceLogInfo(...) IGOLogger::instance()->info(__FILE__, __LINE__, true, __VA_ARGS__)
    #define IGOForceLogInfoOnce(...) \
    { \
        static bool alreadyLogged = false; \
        if (!alreadyLogged) \
        { \
            alreadyLogged = true; \
            IGOLogger::instance()->info(__FILE__, __LINE__, true, __VA_ARGS__); \
        } \
    }

    #define IGOLogWarn(...) IGOLogger::instance()->warn(__FILE__, __LINE__, false, __VA_ARGS__)
    #define IGOForceLogWarn(...) IGOLogger::instance()->warn(__FILE__, __LINE__, true, __VA_ARGS__)
    #define IGOForceLogWarnOnce(...) \
    { \
        static bool alreadyLogged = false; \
        if (!alreadyLogged) \
        { \
            alreadyLogged = true; \
            IGOLogger::instance()->warn(__FILE__, __LINE__, true, __VA_ARGS__); \
        } \
    }

    #define IGOLogDebug(...) IGOLogger::instance()->debug(__FILE__, __LINE__, __VA_ARGS__)
    #define IGOLogEnabled() IGOLogger::instance()->enabled()
    
    #endif

    class IGOLogger
    {
    public:
        static IGOLogger *instance();
        void startAsyncMode();   // start the use of an async thread to save the logs to disc.
        void stopAsyncMode();    // flush the pending logs to disc and destroy the thread. You can still log but it will happen synchronuously.

#ifdef ORIGIN_MAC
        static void setInstance(IGOLogger* logger);
        IGOLogger(const char* filePrefix);
#endif
        ~IGOLogger();

        void enableLogging(bool enable);
#ifndef USE_TRACE_FOR_LOGGING
        bool enabled() { return mEnabled; }
#else
        bool enabled() { return true; }
#endif
        void info(const char *file, int line, bool alwaysLogMessage, const char *fmt, ...);
        void infoWithArgs(const char *file, int line, bool alwaysLogMessage, const char *fmt, va_list arglist);
        void warn(const char *file, int line, bool alwaysLogMessage, const char *fmt, ...);
        void warnWithArgs(const char *file, int line, bool alwaysLogMessage, const char *fmt, va_list arglist);
        void debug(const char *file, int line, const char *fmt, ...);

    private:
        IGOLogger();

#ifdef ORIGIN_MAC
        char mLogFile[MAX_PATH];
        char mFilePrefix[32];
        static IGOLogger* mLogger;
        pthread_t mAsyncThread;
        bool mNullLogger;
#else
        WCHAR mLogFile[MAX_PATH];
        HANDLE mAsyncThread;
#endif
        HANDLE mLogHandle;

        int mPopIndex;
        int mPushIndex;
        char** mAsyncMsgQueue;

        bool mEnabled;
        volatile bool mInAsyncMode;
        
////

        const char *timeString();
#ifdef ORIGIN_MAC
        const char* appName();
#else
        const WCHAR *appName();
#endif
        const char *basename(const char *path);

        void writeLog(const char *type, const char *file, int line, const char *fmt, va_list arglist);
        void checkLogFile();

#if defined(ORIGIN_PC)
    private:
        HRESULT writeParameters(STRSAFE_LPSTR string, int stringSize, const char *fmt, ...);
        HRESULT writeParameters(STRSAFE_LPSTR string, int stringSizeInBytes, const char *fmt, va_list arglist);
        static void asyncRunTrampoline(void* param);
#elif defined(ORIGIN_MAC)
        static void* asyncRunTrampoline(void* param);
#endif
        
        void asyncRun();
        bool popAsyncMsg(char* msg, size_t maxLen);
        bool pushAsyncMsg(const char* msg);
    };
}
#endif

