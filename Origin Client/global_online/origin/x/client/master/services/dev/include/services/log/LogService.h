//  LogService.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef LOGSERVICE_H
#define LOGSERVICE_H

#include <limits>
#include <QDateTime>
#include <QtCore>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Services
    {
        class ORIGIN_PLUGIN_API LogService : public QObject
        {
            Q_OBJECT

        public:

            /// \fn init
            /// \brief Initializes the log services.
            static void init(int argc, char* argv[]);

            /// \fn release
            /// \brief Releases the log services.
            static void release();

        private:

            LogService();
        };

#define	LGR_FILE_SIZE		4*1024*1024	// 4MB
#define	LGR_TRUNCATE_SIZE	.15

        enum LogMsgType
        {
            kLogUnused = 0,

            kLogError,
            kLogWarning,
            kLogDebug,
            kLogHelp,
            kLogEvent,
            kLogAction,
        };

        class ORIGIN_PLUGIN_API Logger : public QObject
        {
			Q_OBJECT

        public:
            class ORIGIN_PLUGIN_API LogEntryStreamer
            {
            public:
                LogEntryStreamer(LogMsgType msgType, const char* sFileName, const char* sFullFuncName, int fileLine);
                LogEntryStreamer();
                ~LogEntryStreamer();

                LogEntryStreamer &	operator<< ( QChar t );
                LogEntryStreamer &	operator<< ( bool t );
                LogEntryStreamer &	operator<< ( char t );
                LogEntryStreamer &	operator<< ( signed short i );
                LogEntryStreamer &	operator<< ( unsigned short i );
                LogEntryStreamer &	operator<< ( signed int i );
                LogEntryStreamer &	operator<< ( unsigned int i );
                LogEntryStreamer &	operator<< ( signed long l );
                LogEntryStreamer &	operator<< ( unsigned long l );
                LogEntryStreamer &	operator<< ( qint64 i );
                LogEntryStreamer &	operator<< ( quint64 i );
                LogEntryStreamer &	operator<< ( float f );
                LogEntryStreamer &	operator<< ( double f );
                LogEntryStreamer &	operator<< ( const char * s );
                LogEntryStreamer &	operator<< ( const wchar_t * s );
                LogEntryStreamer &	operator<< ( const QString & s );
                LogEntryStreamer &	operator<< ( const QStringRef & s );
                LogEntryStreamer &	operator<< ( const QLatin1String & s );
                LogEntryStreamer &	operator<< ( const QByteArray & b );
                LogEntryStreamer &  operator<< ( const std::string& );
                LogEntryStreamer &	operator<< ( const void * p );
            private:

                LogMsgType mMsgType;
                const char* mFileName;
                const char* mFullFuncName;
                int mFileLine;
                QString mLogString;
            };
        protected:
            class ORIGIN_PLUGIN_API LogEntry
            {
            public:
                LogEntry();

                static void CensorString(QString& str);

                void Set(const int nIndex, const LogMsgType msgType, const QString sFullFuncName, const QString sLogMessage);

            public:
                LogMsgType	mMsgType;
                QDateTime	mTimestamp;
                QString		msFullFuncName;
                QString		msLogMessage;
                int			mnIndex;

                static QStringList mCensoredStringList;
            };

		signals:
			void onNewLogEntry(LogMsgType msgType, QString sFullFuncName, QString sLogMessage, QString sSourceFile, int fileLine);

        public:
            static Logger* Instance();
            static LogEntryStreamer NewLogStatement(LogMsgType msgType, const char* sFileName, const char* sFullFuncName, int fileLine);

            void Destroy();

            Logger ( void );
            ~Logger( void );

            bool Init(const QString sLogDataFileName, bool logDebug);         // Takes the name of the file to view data and in which to Log the data (sans file extension).  If none provided, the log will not be saved to disk.
            bool Shutdown();
            bool Log(LogMsgType msgType, const QString sFullFuncName, const QString sLogMessage, const QString sSourceFile, int fileLine);
            void GetLogFile(QString& logFile, int sizeInKb);
            void SetLogDebug(bool logDebug);

        public:
            static QString FormatTimestamp(const QTime& systime);
            static void AddStringToCensorList(const QString& str);
            static QString CensorUrl(QString& str);

            /// \brief Removes PII from string (in place) for privacy.
            ///
            /// \note The LogService already performs this functionality. The
            ///       functionality is only provided here in the rare cases
            ///       when a string needs to be censored but not logged (such
            ///       as when sending telemetry).
            ///
            /// \sa LogEntry::CensorString
            static void CensorStringInPlace(QString& str) { LogEntry::CensorString(str); }

        protected:

            // write a raw string to the logfile (no formatting applied)
            void WriteRawStringToLogfile(const QString& sString);

            // format and write a rLogEntry to the html log file
            void WriteToLogfile(const LogEntry& rLogEntry);

            bool OpenLogFile(const QString& sLogDataFileName);

            // tries to find/open a logfile (log.txt, log1.txt, log2.txt, ...)
            bool FindAvailableLogFile(const QString& sRawFileName, const QString &fileExtension);

            void WriteLogfileHeader(); // using a template on disk, write out the beginning of the EAD html logfile

            // for a given msgType, get the formatting strings:
            //   pClassName is the css class name for the html entry  (ie: error)
            //   pName is the html text name of that message type   (ie: Error&nbsp;&nbsp;)
            void GetMsgTypeFormattingInfo(const LogMsgType msgType, QString &pClassName, QString &pName ) const;

            void FormatLogEntryForTextRow(const LogEntry &rLogEntry, QString& rsBlock) const;

        protected:

            // Thread-safe logging and logger destruction - note that mutex must
            // be static (or at least outside of Logger "instance" scope) in 
            // order to protect logger destruction while logging.
            static QMutex		sMutex;

            bool				mbInitted;

            QFile				mFile;  // File into which the log is written.
            QString				msLogDataFileName;
            QString				msLogDirectory;

            unsigned int		mnUnloggedEntryCnt_Disk;		// Buffered entries not yet committed to disk
            unsigned int		mnNextEntryIndex;

            bool				bKillLogThread;  // Voluntary thread shutdown signal
            volatile bool		mbLoggerThreadReady; // Logger thread is looping (may be halted but is still in loop)

            int					mCurrEntryIndex;
            int					mCurFileSize;

            bool				mLogDebug;
        };

        namespace LoggerFilter
        {
            ORIGIN_PLUGIN_API void DumpCommandLineToLog(const char* header, const QString& cmdline);
        }

        namespace LogUtil
        {
            /// \brief  Returns a "log safe" version of the given URL string.
            ///
            ///         The "log safe" version of the given URL string is essentially the same string but truncated
            ///         at the question-mark query string delimiter.
            ///
            /// \return The given URL string but truncated at the question-mark query string delimiter.
            ORIGIN_PLUGIN_API QString logSafeUrl(const QString& url);
        }

    }
}

#define ORIGIN_LOG_ERROR   Origin::Services::Logger::NewLogStatement(Origin::Services::kLogError, __FILE__, __FUNCTION__, __LINE__)
#define ORIGIN_LOG_WARNING Origin::Services::Logger::NewLogStatement(Origin::Services::kLogWarning, __FILE__, __FUNCTION__, __LINE__)
#define ORIGIN_LOG_DEBUG   Origin::Services::Logger::NewLogStatement(Origin::Services::kLogDebug, __FILE__, __FUNCTION__, __LINE__)
// Log that the system performed some action
#define ORIGIN_LOG_EVENT   Origin::Services::Logger::NewLogStatement(Origin::Services::kLogEvent, __FILE__, __FUNCTION__, __LINE__)
// Log that the user performed some action
#define ORIGIN_LOG_ACTION  Origin::Services::Logger::NewLogStatement(Origin::Services::kLogAction, __FILE__, __FUNCTION__, __LINE__)

#define ORIGIN_LOG_ERROR_IF(condition)      if (condition) Origin::Services::Logger::NewLogStatement(Origin::Services::kLogError, __FILE__, __FUNCTION__, __LINE__)
#define ORIGIN_LOG_WARNING_IF(condition)    if (condition) Origin::Services::Logger::NewLogStatement(Origin::Services::kLogWarning, __FILE__, __FUNCTION__, __LINE__)
#define ORIGIN_LOG_DEBUG_IF(condition)      if (condition) Origin::Services::Logger::NewLogStatement(Origin::Services::kLogDebug, __FILE__, __FUNCTION__, __LINE__)
#define ORIGIN_LOG_EVENT_IF(condition)      if (condition) Origin::Services::Logger::NewLogStatement(Origin::Services::kLogEvent, __FILE__, __FUNCTION__, __LINE__)
#define ORIGIN_LOG_ACTION_IF(condition)     if (condition) Origin::Services::Logger::NewLogStatement(Origin::Services::kLogAction, __FILE__, __FUNCTION__, __LINE__)

#endif // LOGSERVICE_H
