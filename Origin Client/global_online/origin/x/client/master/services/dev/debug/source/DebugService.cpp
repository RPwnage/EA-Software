//  DebugService.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"

#include <EATrace/EATrace.h>
#include <QtCore>
#include <QMessageLogContext>


#include <stdio.h>

#ifdef Q_OS_WIN
#include <windows.h>
#include <ios>
#include <cstdio>
#include <io.h>
#include <fcntl.h>
#endif

namespace Origin
{
    namespace Services
    {
        // Toggle debug logging around specific areas in client.
#ifdef _DEBUG
        // Enables debug logging around entitlement management between client and ecommerce server
        bool DebugLogToggles::entitlementLoggingEnabled = false;

        // Enables debug logging around the catalog update monitor / dirty bits that notifies client of new game configurations
        bool DebugLogToggles::catalogUpdateLoggingEnabled = false;

        // Enables debug logging around the dirty bits engine
        bool DebugLogToggles::dirtyBitsLoggingEnabled = false;
#else
        // DO NOT CHANGE THESE.  Enabling these toggles debug level logging that user should not see in a release build
        bool DebugLogToggles::entitlementLoggingEnabled = false;
        bool DebugLogToggles::catalogUpdateLoggingEnabled = false;
        bool DebugLogToggles::dirtyBitsLoggingEnabled = false;
#endif

        // forward prototypes
        void debugMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg);
        void nullMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg);

        inline void PlatformWriteMessage(const char* msg)
        {
#ifdef Q_OS_WIN
            OutputDebugStringA(msg);
#else
            // For OSX, just write to STDERR
            fprintf(stderr,"%s\n", msg);
#endif
        }

        // statics
        QMutex                                              DebugService::mOriginTimerMutex;
        QMap<QString, DebugService::OriginTimerEntry>       DebugService::mOriginTimeMap;;
        QElapsedTimer                                       DebugService::mGlobalTimer;

        void DebugService::init()
        {
            qRegisterMetaType<const char*>("const char*");

            mGlobalTimer.start();

#if defined(_DEBUG)
            enableDebugMessages();
#endif
        }

        void DebugService::release()
        {
            ReportOriginTimers();

            qInstallMessageHandler(0);
        }

        void DebugService::disableDebugMessages()
        {
            qInstallMessageHandler(nullMessageHandler);
        }

        void DebugService::enableDebugMessages()
        {
            qInstallMessageHandler(debugMessageHandler);
        }


        void DebugService::startTimer(const QString& name)
        {
            QMutexLocker locker(&mOriginTimerMutex);
            mOriginTimeMap[name].mLastTimestamp = mGlobalTimer.elapsed();
        }

        qint64 DebugService::markTimer(const QString& name)
        {
            QMutexLocker locker(&mOriginTimerMutex);
            QMap<QString, OriginTimerEntry>::iterator it = mOriginTimeMap.find(name);
            if (it == mOriginTimeMap.end())
                return -1;

            // Add a record of this time
            qint64 nDelta = mGlobalTimer.elapsed() - (*it).mLastTimestamp;
            (*it).mMarkCount++;
            (*it).mTotalTime += nDelta;

            // Mark the peak time if it's higher
            if ((*it).mPeakTime < nDelta)
                (*it).mPeakTime = nDelta;

            return nDelta;
        }

        qint64 DebugService::getGlobalTimerElapsed()
        {
            return mGlobalTimer.elapsed();
        }

        void DebugService::ReportOriginTimers()
        {
            ORIGIN_LOG_EVENT << "Reporting on " << mOriginTimeMap.size() << " Origin Timers.";
            for (QMap<QString, DebugService::OriginTimerEntry>::iterator it = mOriginTimeMap.begin(); it != mOriginTimeMap.end(); it++)
            {
                QString sName = it.key();
                OriginTimerEntry& entry = *it;

                // In case the value is ever 0
                qint64 nAverageTime = 0;
                if (entry.mMarkCount > 0)
                    nAverageTime = entry.mTotalTime/entry.mMarkCount;

                ORIGIN_LOG_EVENT << "OriginTimer: \"" << sName << "\" Times Called:" << entry.mMarkCount << " Average:" << nAverageTime << "ms Peak: " << entry.mPeakTime << "ms";
            }
        }

        void debugMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
        {
            switch (type)
            {
            case QtDebugMsg:
            case QtWarningMsg:
                EA_TRACE_FORMATTED(("%s\n", msg.toLatin1().constData()));
                break;

            case QtFatalMsg:
                EA_FAIL_MESSAGE(msg.toLatin1().constData());

            // These cases are currently unhandled?  GCC complains...
            case QtCriticalMsg:
            default:
                break;
            }
        }

        void nullMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &)
        {
        }

#ifdef Q_OS_WIN
        const DWORD MS_VC_EXCEPTION=0x406D1388;

        #pragma pack(push,8)
        typedef struct tagTHREADNAME_INFO
        {
           DWORD dwType; // Must be 0x1000.
           LPCSTR szName; // Pointer to name (in user addr space).
           DWORD dwThreadID; // Thread ID (-1=caller thread).
           DWORD dwFlags; // Reserved for future use, must be zero.
        } THREADNAME_INFO;
        #pragma pack(pop)

        void SetThreadName( const char* threadName )
        {
            // If theres no debugger, don't do this (it can cause problems otherwise)
            if (!::IsDebuggerPresent())
                return;

            THREADNAME_INFO info;
            info.dwType = 0x1000;
            info.szName = threadName;
            info.dwThreadID = (DWORD)-1;
            info.dwFlags = 0;

            __try
            {
                RaiseException( MS_VC_EXCEPTION, 0, sizeof(info)/sizeof(ULONG_PTR), (ULONG_PTR*)&info );
            }
            __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }
#else
        void SetThreadName( const char* threadName )
        {
            // TODO
        }
#endif

        ThreadNameSetter::ThreadNameSetter()
        {

        }

        void ThreadNameSetter::setThreadName( QThread *thread, const char* threadName )
        {
            if (!thread->isRunning())
                return;

            ThreadNameSetter *setter = new ThreadNameSetter;
            setter->moveToThread(thread);
            QMetaObject::invokeMethod(setter, "_setName", Qt::QueuedConnection, Q_ARG(const char*, threadName));
        }

        void ThreadNameSetter::setCurrentThreadName ( const char* threadName )
        {
            SetThreadName(threadName);
        }

        void ThreadNameSetter::_setName( const char* threadName )
        {
            SetThreadName(threadName);

            this->deleteLater();
        }
        //////////////////////////////////////////////////////////////////////////

        const QJsonValue & JsonValueValidator::validate(const QJsonValue &val)
        {
#if ORIGIN_DEBUG
            ORIGIN_ASSERT(!val.isUndefined() && !val.isNull());
#endif
            //TODO:: ADD TELEMETRY HERE FOR FAILURES
            return val;
        }

        //////////////////////////////////////////////////////////////////////////

        
        void DebugService::createConsole()
        {
#ifdef ORIGIN_PC
            // from: http://asawicki.info/news_1326_redirecting_standard_io_to_windows_console.html
            // also: http://support.microsoft.com/kb/105305
            // originally: http://dslweb.nwnexus.com/~ast/dload/guicon.htm

            bool useFiles = (Origin::Services::readSetting(Origin::Services::SETTING_LogConsoleDevice) == "file");

            AllocConsole();

            if (useFiles) {
                FILE* file = fopen("stdout.txt", "w");
                setvbuf(file, NULL, _IONBF, 0);
                *stdout = *file;

                // redirect stderr to stdout.txt - useful for interleaving stdout and stderr
                //file = fopen("stderr.txt", "w");
                file = stdout;
                *stderr = *file;
            } else {
                // redirect unbuffered STDOUT to the console
                HANDLE stdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
                int osfHandle = _open_osfhandle(reinterpret_cast<long>(stdOutHandle), _O_TEXT);
                FILE* file = _fdopen(osfHandle, "w");
                setvbuf(file, NULL, _IONBF, 0);
                *stdout = *file;

                // redirect unbuffered STDIN to the console
                HANDLE stdInHandle = GetStdHandle(STD_INPUT_HANDLE);
                int osfInHandle = _open_osfhandle(reinterpret_cast<long>(stdInHandle), _O_TEXT);
                file = _fdopen(osfInHandle, "r" );
                setvbuf(file, NULL, _IONBF, 0);
                *stdin = *file;

                // redirect unbuffered STDERR to the console
                HANDLE stdErrHandle = GetStdHandle(STD_ERROR_HANDLE);
                int osfErrHandle = _open_osfhandle(reinterpret_cast<long>(stdErrHandle), _O_TEXT);
                file = _fdopen(osfErrHandle, "w" );
                setvbuf(file, NULL, _IONBF, 0);
                *stderr = *file;
            }

            // make cout, wcout, cin, wcin, wcerr, cerr, wclog and clog point to console as well
            std::ios::sync_with_stdio();
#endif
        }
    }
}
