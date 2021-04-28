//  LogService.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "services/log/LogService.h"
#include "services/debug/DebugService.h"
#include "TelemetryAPIDLL.h"
#include <EATrace/EATrace.h>
#include <EAStdC/EAString.h>
#include "version/version.h"

#include <limits>
#include <QDateTime>
#include <QTextDocument>
#include <string>

#if defined(Q_OS_WIN)
#include <Windows.h>
#include <AccCtrl.h>
#include <AclAPI.h>
#include <Sddl.h>
#include <shlobj.h>
#include <shobjidl.h>
#else
#include <sys/types.h>
#include <sys/stat.h>
#endif

namespace
{
    // local prototypes
    bool grantEveryoneAccessToFile(const std::wstring& sFileName);
    QString GetUserAccount();
}

namespace Origin
{
    namespace Services
    {
        void LogService::init(int argc, char* argv[])
        {
            // Compute log filename
            const QString gsEADMLogFolder("/Logs/Client_Log");
#ifdef ORIGIN_PC
            
            TCHAR szPath[MAX_PATH];
            if(! SUCCEEDED(SHGetFolderPath(NULL, CSIDL_COMMON_APPDATA|CSIDL_FLAG_CREATE, NULL, 0, szPath))) return;
            
            QString strLogFolder(QString::fromWCharArray(szPath));
            strLogFolder.append(QDir::separator());
            strLogFolder.append(QCoreApplication::applicationName());
#else
            QString strLogFolder;
            QStringList paths = QStandardPaths::standardLocations(QStandardPaths::DataLocation);
            for (QStringList::const_iterator iter = paths.constBegin(); iter != paths.constEnd(); ++iter)
            {
                if (!(*iter).isEmpty())
                {
                    strLogFolder = *iter;
                    break;
                }
            }

            // This shouldn't be happening, but just to be safe...
            if (strLogFolder.isEmpty())
                strLogFolder = QCoreApplication::applicationDirPath();
            
            strLogFolder += QDir::separator();
#endif
            strLogFolder += gsEADMLogFolder;
            
            
            // Enable full logging until we initialize the SettingsManager, at which point we can check the actual
            // Logging level to use
#ifdef ORIGIN_DEBUG
            bool logDebug = true;
#else
            bool logDebug = false;
#endif
            
            //  Initiate logging
            Origin::Services::Logger::Instance()->Init(strLogFolder, logDebug);
            
            //  Output the client version
            ORIGIN_LOG_EVENT << "Version: " << EALS_VERSION_P_DELIMITED;
            
            //  Output the command-line parameters
            QString cmdLine;
            for (int i = 1; i < argc; i++)
            {
                cmdLine += argv[i];
                cmdLine += " ";
            }
            Origin::Services::LoggerFilter::DumpCommandLineToLog("Cmdline Param", cmdLine);
        }

        void LogService::release()
        {
        }

        /*
        Not used yet
        void ReleaseAssert(bool value, const char* condition)
        {
            if (value == false)
            {
                ORIGIN_LOG_ERROR << "Assert = " << condition;
            }
        }
        */

        ///////////////////////////////////////////////////////////////////////////////
        // GLOBAL FUNCTION
        ///////////////////////////////////////////////////////////////////////////////

        // Filters out token values and persona values from the command-line string
        //
        namespace LoggerFilter
        {
            void DumpCommandLineToLog(const char* header, const QString& cmdline)
            {
                ORIGIN_LOG_DEBUG << "debug header=\"" << header << "\" cmdLine=\"" << cmdline << "\"";

                // Tokenize the string
                QStringList lineParsed = cmdline.split(QRegExp(" "));
                QString logLine;

                // Iterate through the string list
                for (int i = 0; i < lineParsed.size(); i++)
                {
                    // Filter out token/persona
                    if ((lineParsed[i].contains(QString("token"), Qt::CaseInsensitive) == true) ||
                        (lineParsed[i].contains(QString("persona"), Qt::CaseInsensitive) == true))
                    {
                        // Don't log
                    }
                    else
                    {
                        // Safe to save to log
                        logLine.append(QString("%1: %2 ").arg(header).arg(lineParsed[i]));
                    }
                }

                ORIGIN_LOG_EVENT << logLine;
            }
        };

        namespace LogUtil
        {
            QString logSafeUrl(const QString& url)
            {
                QString logSafeUrl(url);

                int delimiter = url.lastIndexOf('?');
                if (delimiter > 0)
                {
                    // Strip off the token
                    logSafeUrl = url.left(delimiter);
                }

                return logSafeUrl;
            }
        }

        ///////////////////////////////////////////////////////////////////////////////
        // LOGGER
        ///////////////////////////////////////////////////////////////////////////////

        // Singleton static instance - note that this isn't a static member variable,
        // just static (internal) linkage.
        static Logger* sLogger = NULL;

        // Static mutex member variable definition
        QMutex Logger::sMutex(QMutex::Recursive);

        QStringList Logger::LogEntry::mCensoredStringList;

        Logger* Logger::Instance()
        {
            // Obtain mutex lock to prevent another thread calling Destroy while 
            // we're executing.
            QMutexLocker locker(&sMutex);

            if (sLogger == NULL)
                sLogger = new Logger();
            return sLogger;
        }

        void Logger::Destroy()
        {
            // Ensure that we don't destroy the logger while we're logging - note
            // that it's still possible, however, for another tread to get the Logger
            // instance pointer and have another thread call destroy.
            QMutexLocker locker(&sMutex);

            // Since Destroy is a non-static member function, the following should
            // hold true:
            ORIGIN_ASSERT(this == sLogger);

            delete sLogger;
            sLogger = NULL;
        }

        Logger::Logger( void )
        {
            mbInitted = false;
            mCurrEntryIndex = 0;
        }

        Logger::~Logger( void )
        {
            if ( mbInitted ) { Shutdown(); }
        }

        Logger::LogEntryStreamer::LogEntryStreamer(LogMsgType msgType, const char* sFileName, const char* sFullFuncName, int fileLine)
            : mMsgType(msgType), mFileName(sFileName), mFullFuncName(sFullFuncName), mFileLine(fileLine)
        {
        }

        Logger::LogEntryStreamer::LogEntryStreamer() {}

        Logger::LogEntryStreamer::~LogEntryStreamer()
        {
            // We obtain a mutex lock here to ensure the pointer returned by the
            // instance() method is still valid when executing the Log method.
            QMutexLocker locker(&sMutex);

            Logger::Instance()->Log(mMsgType,mFullFuncName, mLogString, mFileName, mFileLine);
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( QChar t )
        {
            mLogString = mLogString.append(t);
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( bool t )
        {
            mLogString = mLogString.append(t ? "true" : "false");
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( char t )
        {
            mLogString = mLogString.append(QChar(t));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( signed short i )
        {
            mLogString = mLogString.append(QString::number(i));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( unsigned short i )
        {
            mLogString = mLogString.append(QString::number(i));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( signed int i )
        {
            mLogString = mLogString.append(QString::number(i));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( unsigned int i )
        {
            mLogString = mLogString.append(QString::number(i));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( signed long l )
        {
            mLogString = mLogString.append(QString::number(l));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( unsigned long l )
        {
            mLogString = mLogString.append(QString::number(l));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( qint64 i )
        {
            mLogString = mLogString.append(QString::number(i));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( quint64 i )
        {
            mLogString = mLogString.append(QString::number(i));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( float f )
        {
            mLogString = mLogString.append(QString::number(f));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( double f )
        {
            mLogString = mLogString.append(QString::number(f));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( const char * s )
        {
            mLogString = mLogString.append(s);
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( const wchar_t * s )
        {
            mLogString = mLogString.append(QString::fromWCharArray(s));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( const QString & s )
        {
            mLogString = mLogString.append(s);
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( const QStringRef & s )
        {
            mLogString = mLogString.append(s);
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( const QLatin1String & s )
        {
            mLogString = mLogString.append(s);
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( const QByteArray & b )
        {
            mLogString = mLogString.append(QString::fromUtf8(b));
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( const std::string& b )
        {
            mLogString = mLogString.append(b.c_str());
            return *this;
        }

        Logger::LogEntryStreamer& Logger::LogEntryStreamer::operator<< ( const void * p )
        {
            mLogString = mLogString.append(QString("0x%1").arg((unsigned long)p, 8, 16, QChar('0')));
            return *this;
        }

        // opens a log in the sFileName logFile.
        //   if successfully opened, file is truncated
        //   if we couldn't open it, we create logFile_x.log
        //   (the most likely cause of this is another instance of the EAD preventing the file from being opened for shared writing)
        bool Logger::Init(const QString sLogDataFileName, bool logDebug)
        {
            bool bResult = false;

            if ( !mbInitted )
            {
                //Make sure the folder is created
                int nLastIndex = sLogDataFileName.lastIndexOf('/');
                QString sLogDirectory = sLogDataFileName.left( nLastIndex );
                QDir logDirectory(sLogDirectory);
                if (!logDirectory.exists())
                {
                    //Create it since it's apparently not there
                    logDirectory.mkpath( sLogDirectory );
                }

                // Start log data file
                OpenLogFile(sLogDataFileName);
            
                // Mark that we're initialized.
                bResult = mbInitted = true;

                //QTime timestamp;
                //timestamp = QTime::currentTime();
                
                AddStringToCensorList(GetUserAccount());

                // Log the start message.
                QString sTextFilename;
                QTextStream(&sTextFilename) << "Logfile: " << sLogDataFileName;
                LogEntry::CensorString(sTextFilename);
                Log(kLogEvent, __FUNCTION__, sTextFilename, __FILE__, __LINE__);

                QString sTextLocalTime;
                QTextStream(&sTextLocalTime) << "Local time: " << QDateTime::currentDateTime().toString();
                Log(kLogEvent, __FUNCTION__, sTextLocalTime, __FILE__, __LINE__);

                mLogDebug = logDebug;
            }

            return bResult;
        }

        bool Logger::Shutdown()
        {
            if (mbInitted == false)
                return true;
            mbInitted = false; 

            // If we're logging to a file, close it now.
            if (mFile.isOpen())
            {
                // Close file
                mFile.close();
            }

            mbInitted = false; 

            return true;
        }

        // tries to open a logfile for exclusive writing
        //   rawFileName should be a base logfile name w/o a file extension ie: CoreLog
        bool Logger::OpenLogFile(const QString& sLogDataFileName)
        {
            ORIGIN_ASSERT(!sLogDataFileName.isEmpty());
            const QString fileExtension = ".txt";

            // The file overwrites any previous log
            msLogDataFileName = sLogDataFileName;
            msLogDataFileName.append(fileExtension);

            mFile.setFileName(msLogDataFileName);

            // Try to explicitly set permissions as writable
            mFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

            if (!mFile.open(QIODevice::ReadWrite | QIODevice::Append))
            {
                // Ensure access to standard users too if running as admin
                wchar_t fileName[2048];
                
                EA::StdC::Strlcpy(&fileName[0], reinterpret_cast<const char16_t*>(msLogDataFileName.utf16()), 2048);
                grantEveryoneAccessToFile(fileName);

                // we probably couldn't open the file because another EAD instance has it open for exclusive writes...
                // try to open a logfile w/ a similar name...
                if( !FindAvailableLogFile(sLogDataFileName, fileExtension) )
                {
                    // compose & log an error saying so (if we're a debug build, we could see the error in the log window)
                    QString errorText;
                    QTextStream(&errorText) << "Couldn't open log file \"" << msLogDataFileName << "\" for writing!";

                    msLogDataFileName = "";
                    return false;
                }
            }

            mCurFileSize = mFile.size();

            return true;
        }
 

        // try to open a similarly named logfile
        //  (tries 4 times: eaddesktop1.log, eaddesktop2.log, ...)
        bool Logger::FindAvailableLogFile(const QString& sRawFileName, const QString& fileExtension)
        {
            // find an available logfile with a similar name to the one provided...
            for( int i=1; i<5; ++i )
            {
                msLogDataFileName.sprintf("%s%d%s", qPrintable(sRawFileName), i, qPrintable(fileExtension));

                mFile.setFileName(msLogDataFileName);

                // Force permission to writable in case of read-only file
                mFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

                if (mFile.open(QIODevice::ReadWrite | QIODevice::Append))
                {
                    GetTelemetryInterface()->Metric_APP_ADDITIONAL_CLIENT_LOG(msLogDataFileName.toUtf8().data());
                    return true;
                }
            }

            // we still can't open the log (the logs dir probably doesn't exist)
            return false;
        }

        // write a raw string to the logfile - no formatting is applied
        void Logger::WriteRawStringToLogfile(const QString& sString)
        {
            ORIGIN_ASSERT(mFile.isOpen());

            if( mFile.isOpen() )
            {
                //Create a stream to write to the file
                QTextStream stream( &mFile );
                stream.setCodec("UTF-8");
                stream << sString;

                mCurFileSize += sString.length();
            }
        }

        Logger::LogEntryStreamer Logger::NewLogStatement(LogMsgType msgType, const char* sFileName, const char* sFullFuncName, int fileLine)
        {
            // we need a static fail safe instance, otherwise is goes out of scope after the return and crashes!!!
            static Logger::LogEntryStreamer failSafeInstance = Logger::LogEntryStreamer();
            if (!sLogger)
                return failSafeInstance;
            return Logger::LogEntryStreamer(msgType, sFileName, sFullFuncName, fileLine);
        }

        bool Logger::Log(LogMsgType msgType, const QString sFullFuncName, const QString sLogMessage, const QString sSourceFile, int fileLine)
        {
            // Obtain a lock here to prevent "this" from getting destroyed while 
            // we're logging.
            QMutexLocker locker(&sMutex);

            // Emit the signal for anybody who may be listening (a CLI, perhaps?)
            emit (onNewLogEntry(msgType, sFullFuncName, sLogMessage, sSourceFile, fileLine));

            if ((msgType == kLogDebug || msgType == kLogHelp) && !mLogDebug)
                return mbInitted;

            // Log file output
            if ( mbInitted )
            {
                //Create the log entry and then write it
                LogEntry entry;
                entry.Set(mCurrEntryIndex, msgType, sFullFuncName, sLogMessage);

                mCurrEntryIndex++;

                // format message string
                QString sFormattedMsg;
                QString sThreadId;
                sThreadId.sprintf("[%lld]", (qint64)QThread::currentThreadId());
                sFormattedMsg = sThreadId + QDateTime::currentDateTimeUtc().toString() + " " + sLogMessage;

                // Output to console
                EA_TRACE_FORMATTED(("%s\n", sFormattedMsg.toLatin1().constData()));

                WriteToLogfile(entry);

                if (mCurFileSize > LGR_FILE_SIZE)
                {
                    //Open up a stream, go to the beginning of the file and read it all into a string
                    QTextStream stream( &mFile );

                    //Set the codec manually so unicode symbols in the logs do not get garbled 
                    stream.setAutoDetectUnicode(false);
                    stream.setCodec(QTextCodec::codecForName("UTF-8"));

                    stream.seek(0);
                    QString currentLog = stream.readAll();

                    //Want to cut off the front x% from the log and update it
                    mCurFileSize *= (1.f - LGR_TRUNCATE_SIZE);

                    // This will be our new log
                    QString newLog = currentLog.right( mCurFileSize );
            
                    // Need to cut off any junk that we half removed...
                    int lineEnd = newLog.indexOf('\n'); 
                    mCurFileSize -= lineEnd;
                    newLog = newLog.right(mCurFileSize );

                    // Empty the file
                    mFile.resize(0);

                    // Store this temporarily since it is correct and will be modified during our write call
                    int dTempSize = mCurFileSize;

                    WriteRawStringToLogfile(newLog);

                    mCurFileSize = dTempSize;
                }

            }
            return mbInitted;
        }

        QString Logger::FormatTimestamp(const QTime& systime)
        {
            // Format the timestamp
            QString sTimestamp;
            sTimestamp.sprintf("[%02d:%02d:%02d:%03d]", systime.hour(), systime.minute(), systime.second(), systime.msec());
            return sTimestamp;
        }

        void Logger::AddStringToCensorList(const QString& str)
        {
            // We only need to sensor strings that are longer than 1 letter.  
            // Also reduces log confusion 
            if (str.size() > 1)
            {
                int strLen = str.size();
                int i;
                for (i = 0; i < LogEntry::mCensoredStringList.size(); ++i)
                {
                    int currentLen = LogEntry::mCensoredStringList.at(i).size();
                    if (strLen > currentLen)
                    {
                        break;
                    }
                    else if (strLen == currentLen && str.compare(LogEntry::mCensoredStringList.at(i), Qt::CaseInsensitive) == 0)
                    {
                        return;
                    }
                }

                LogEntry::mCensoredStringList.insert(i, str);
            }
        }

        // format and write a rLogEntry to the log file
        void Logger::WriteToLogfile(const LogEntry &rLogEntry)
        {
            if( mFile.isOpen() )
            {
                QString sRaw;
                FormatLogEntryForTextRow(rLogEntry, sRaw);
                WriteRawStringToLogfile(sRaw);
            }
        }

        // given a rLogEntry, spit out a string that we can drop into either the log window or the log file
        void Logger::FormatLogEntryForTextRow(const LogEntry &rLogEntry, QString& rsBlock) const
        {
            // logType type col
            QString sLogTypeCssName, sLogTypeDisplayName;
            GetMsgTypeFormattingInfo(rLogEntry.mMsgType, sLogTypeCssName, sLogTypeDisplayName);

            // compose the text row. Use QTextStream instead of sprintf since it works seamlessly with unicode
            QTextStream outStream(&rsBlock);
            outStream.setCodec("UTF-8");
            outStream.setFieldAlignment(QTextStream::AlignLeft);
            QString marker;
            if((rLogEntry.mMsgType == kLogWarning) || (rLogEntry.mMsgType == kLogError))
            {
                marker = "**";
            }

            outStream.setFieldWidth(2);
            outStream << marker;
            outStream.setFieldWidth(7);
            outStream << rLogEntry.mnIndex;
            outStream.setFieldWidth(20);
            outStream << rLogEntry.mTimestamp.toString("MMM d hh:mm:ss.zzz");
            outStream.setFieldWidth(10);
            outStream << sLogTypeDisplayName;
            outStream.setFieldWidth(75);
            outStream << rLogEntry.msFullFuncName;
            outStream.setFieldWidth(1);
            outStream << " ";
            outStream.setFieldWidth(10);
            outStream << QString::number((qint64)QThread::currentThreadId());
            outStream.setFieldWidth(10);
            outStream << rLogEntry.msLogMessage;
            outStream.setFieldWidth(1);
            outStream << "\r\n";
        }

        // for a msgType, get the formatting strings:
        //   pClassName is the css class name for the html entry  (ie: error)
        //   pName is the html text name of that message type   (ie: Error&nbsp;&nbsp;)
        void Logger::GetMsgTypeFormattingInfo(const LogMsgType msgType, QString &pClassName, QString &pName) const
        {
            switch( msgType )
            {
            case kLogUnused:
                pClassName = "UNUSED";
                pName = "UNUSED";
                break;
            case kLogDebug:
                pClassName = "Dbg";
                pName = "Debug";
                break;
            case kLogWarning:
                pClassName = "Wrn";
                pName = "Warning";
                break;
            case kLogError:
                pClassName = "Err";
                pName = "Error";
                break;
            case kLogHelp:
                pClassName = "Hlp";
                pName = "Help";
                break;
            case kLogEvent:
                pClassName = "Evt";
                pName = "Event";
                break;
            case kLogAction:
                pClassName = "Act";
                pName = "Action";
                break;
            default:
                pClassName = "";
                pName = "";
                break;
            }
        }


        Logger::LogEntry::LogEntry() :
        mMsgType( kLogUnused )
        {}

        void Logger::LogEntry::Set(const int nIndex, const LogMsgType msgType, const QString sFullFuncName, const QString sLogMessage)
        {
            // Chronological index
            mnIndex = nIndex;

            // Entry type
            mMsgType = msgType;

            // Function where log entry was called
            msFullFuncName = sFullFuncName;

            // format message string
            msLogMessage = sLogMessage;

        #ifndef EA_DEBUG
            if (msgType != kLogDebug)
            {
                CensorString(msLogMessage);
            }
        #endif

            mTimestamp = QDateTime::currentDateTimeUtc();
        }

        void Logger::LogEntry::CensorString(QString& str)
        {
            foreach(QString s, mCensoredStringList)
            {
                if (str.indexOf(s, 0, Qt::CaseInsensitive) != -1)
                {
                    QString censored(s.size(), 'X');
                    str.replace(s, censored, Qt::CaseInsensitive);
                }
            }
        }

        QString Logger::CensorUrl(QString& str)
        {
            QString retString = str;
            int idx;
            QList<char> endCharList;
            endCharList << '.' << '/' << '\0' << '\n' << '\r';
            foreach(QString s, LogEntry::mCensoredStringList)
            {
                if(((idx = str.indexOf(s, 0, Qt::CaseInsensitive)) != -1) && (idx > 0) && (str[idx-1] == '/'))
                {
                    int i = idx + s.length();
                    if(endCharList.contains(str[i].toLatin1()))
                    {
                        // take the censored string out
                        QString censored = s + str[i];
                        retString.remove(censored);
                    }
                }
            }
            return retString;
        }

        void Logger::GetLogFile(QString& logFile, int sizeInKb)
        {
            //Open up a stream, go to the beginning of the file and read it all into a string
            QTextStream stream( &mFile );
            stream.seek(0);
            logFile = stream.readAll();
            
            if (logFile.length() > (sizeInKb * 1024))
            {
                //Cut it down to size
                logFile = logFile.right( sizeInKb * 1024 );
            }
        }
        
        void Logger::SetLogDebug(bool logDebug)
        {
            mLogDebug = logDebug;
        }
    }
}

namespace
{

#if defined(Q_OS_WIN)
    EXPLICIT_ACCESS eaForWellKnownGroup(PSID groupSID)
    {
        EXPLICIT_ACCESS ea={0};
        ea.grfAccessMode = GRANT_ACCESS;
        ea.grfAccessPermissions = GENERIC_ALL;
        ea.grfInheritance = CONTAINER_INHERIT_ACE|OBJECT_INHERIT_ACE;
        ea.Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea.Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea.Trustee.ptstrName = (LPTSTR)groupSID;

        return ea;
    }
#endif

    bool grantEveryoneAccessToFile(const std::wstring& sFileName)
    {
        bool ret = false;

    #if defined(Q_OS_WIN)
        HANDLE hDir = CreateFile(sFileName.c_str(), READ_CONTROL|WRITE_DAC,0,NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
        if (hDir == INVALID_HANDLE_VALUE) 
        {
            return false;
        }

        ACL* pOldDACL=NULL;
        PSECURITY_DESCRIPTOR pSD = NULL;
        GetSecurityInfo(hDir,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,&pOldDACL,NULL,&pSD);

        PSID usersSID;
        PSID everyoneSID;

        static const LPWSTR UsersGroup = L"S-1-5-32-545";
        static const LPWSTR EveryoneGroup = L"S-1-1-0";

        if ((ConvertStringSidToSid(UsersGroup, &usersSID)) &&
            ConvertStringSidToSid(EveryoneGroup, &everyoneSID))
        {
            EXPLICIT_ACCESS newAccess[2];

            newAccess[0] = eaForWellKnownGroup(usersSID);
            newAccess[1] = eaForWellKnownGroup(everyoneSID);

            ACL* pNewDACL = NULL;

            if (SetEntriesInAcl(2, newAccess,pOldDACL,&pNewDACL) == ERROR_SUCCESS)
            {
                ret = (SetSecurityInfo(hDir,SE_FILE_OBJECT,DACL_SECURITY_INFORMATION,NULL,NULL,pNewDACL,NULL) == ERROR_SUCCESS);
            }

            LocalFree(pNewDACL);
        }

        LocalFree(pSD);

        CloseHandle(hDir);

    #else
        QString qfName = QString::fromStdWString(sFileName);
        // chmod 666 filename
        if (0 == chmod(qPrintable(qfName), S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH))
        {
            ret = true;
        }
        else
        {
            ret = false;
        }
    #endif

        return ret;
    }

    QString GetUserAccount()
    {
        QString userName;
#ifdef WIN32
        LPTSTR lpszSystemInfo;
        DWORD cchBuff = 256;
        const int MaxUserNameLength = 256; // Maximum user name length
        TCHAR tchBuffer[MaxUserNameLength + 1];
        lpszSystemInfo = tchBuffer;
        GetUserName(lpszSystemInfo, &cchBuff);
        userName = QString::fromUtf16(lpszSystemInfo);
#else
        userName = getenv("USER");
#endif
        return  userName;
    }
}
