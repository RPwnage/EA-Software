//  DebugService.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef DEBUGSERVICE_H
#define DEBUGSERVICE_H

#include "EATrace/EATrace.h"
#include <QObject>
#include <QMetaObject>
#include <QThread>
#include <QMutex>
#include <QMap>
#include <QElapsedTimer>
#include <QJsonValue>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API DebugLogToggles
        {
        public:
            static bool entitlementLoggingEnabled;
            static bool catalogUpdateLoggingEnabled;
            static bool dirtyBitsLoggingEnabled;
        };

        class ORIGIN_PLUGIN_API DebugService : public QObject
        {
            Q_OBJECT

        public:

            // structs
            class ORIGIN_PLUGIN_API OriginTimerEntry
            {
            public:
                OriginTimerEntry() : mLastTimestamp(0), mMarkCount(0), mTotalTime(0), mPeakTime(0) {}

                qint64  mLastTimestamp;     // The last time at which this timer was started
                qint64  mMarkCount;         // Number of times this timer was "marked" (i.e. finished)
                qint64  mTotalTime;         // Total amount of time spent timing this
                qint64  mPeakTime;          // Highest amount of time in a single time
            };

            /// \brief Initializes the debug services.
            static void init();

            /// \brief Releases the debug services.
            static void release();

            /// \brief Disables the handling of debug messages.
            static void disableDebugMessages();

            /// \brief Enables the handling of debug messages.
            static void enableDebugMessages();

            /// \brief Starts a generic timer.  Can be used for restart as well.
            static void startTimer(const QString& name);

            /// \brief Gets the elapsed time of a generic timer, and makes a note in the timer map.
            static qint64 markTimer(const QString& name);

            /// \brief Gets the global elapsed time.
            static qint64 getGlobalTimerElapsed();

            //static void assert(bool condition);
            //static void assert_message(bool condition);

            /// \brief Creates a console to which standard IO is directed
            static void createConsole();

        private:

            DebugService();

            static void ReportOriginTimers();

            static QMutex mOriginTimerMutex;
            static QMap<QString, OriginTimerEntry> mOriginTimeMap;;
            static QElapsedTimer mGlobalTimer;

        };

        class ORIGIN_PLUGIN_API ThreadNameSetter : public QObject
        {
            Q_OBJECT
        public:
            static void setThreadName( QThread *thread, const char* threadName );

            static void setCurrentThreadName ( const char* threadName );
        protected:
            Q_INVOKABLE void _setName( const char* threadName );
        
            ThreadNameSetter(); // Not publicly constructable
            
        };

        class ORIGIN_PLUGIN_API JsonValueValidator : public QObject
        {
            Q_OBJECT
        public:
            static const QJsonValue &validate(const QJsonValue &val);
        };
    }
}

#if defined(_DEBUG)

#define ORIGIN_DEBUG 1
#define ORIGIN_ASSERT(condition) EA_ASSERT(condition)
#define ORIGIN_ASSERT_MESSAGE(condition, message) EA_ASSERT_MESSAGE(condition, message)

#else

#define ORIGIN_RELEASE 1
#define ORIGIN_ASSERT(condition)
#define ORIGIN_ASSERT_MESSAGE(condition, message)

#endif

#define ORIGIN_UNUSED(vble) (void) vble;

#define ORIGIN_VERIFY(condition) { bool c = condition; EA_ASSERT(c); Q_UNUSED(c); }
#define ORIGIN_VERIFY_CONNECT(a,b,c,d) {bool wasConnected = QObject::connect(a,b,c,d); ORIGIN_ASSERT(wasConnected); Q_UNUSED(wasConnected); }
#define ORIGIN_VERIFY_DISCONNECT(a,b,c,d) {QObject::disconnect(a,b,c,d);} 
#define ORIGIN_VERIFY_CONNECT_EX(a,b,c,d,e) {bool wasConnected = QObject::connect(a,b,c,d,e); ORIGIN_ASSERT(wasConnected); Q_UNUSED(wasConnected); }
#define ORIGIN_VERIFY_DISCONNECT_EX(a,b,c,d,e) {QObject::disconnect(a,b,c,d,e);} 

// Generic timing across the application
#define TIME_BEGIN(name) Origin::Services::DebugService::startTimer(name);
#define TIME_END(name) Origin::Services::DebugService::markTimer(name);

#define ORIGIN_CHECK(condition) { bool c = condition; EA_ASSERT(c); Q_UNUSED(c); }

#endif // DEBUGSERVICE_H
