//  LogService_win.h
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#ifndef LOGSERVICE_H
#define LOGSERVICE_H

#include <string>
#include <Foundation/Foundation.h>

class LogServicePlatform;

class LogService
{

public:

    /// \fn init
    /// \brief Initializes the log services.
    static void init();

    /// \fn release
    /// \brief Releases the log services.
    static void release();

    /// \fn releaseAndExitProcess
    /// \brief Release the log service and exit the process (exit (0))
    static void releaseAndExitProcess ();

private:

    LogService();
};

#define	LGR_FILE_SIZE		1024*1024 // 1MB
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

class Logger  
{

public:
    class LogEntryStreamer
    {
    public:
        LogEntryStreamer(LogMsgType msgType, const char* sFileName, const char* sFullFuncName, int fileLine);
        LogEntryStreamer();
        ~LogEntryStreamer();

        LogEntryStreamer &	operator<< ( bool t );
        LogEntryStreamer &	operator<< ( char t );
        LogEntryStreamer &	operator<< ( signed short i );
        LogEntryStreamer &	operator<< ( unsigned short i );
        LogEntryStreamer &	operator<< ( signed int i );
        LogEntryStreamer &	operator<< ( unsigned int i );
        LogEntryStreamer &	operator<< ( signed long l );
        LogEntryStreamer &	operator<< ( unsigned long l );
        LogEntryStreamer &	operator<< ( float f );
        LogEntryStreamer &	operator<< ( double f );
        LogEntryStreamer &	operator<< ( const char * s );
        LogEntryStreamer &  operator<< ( const std::string& );
        LogEntryStreamer &	operator<< ( const void * p );
    private:

        LogMsgType mMsgType;
        const char* mFileName;
        const char* mFullFuncName;
        int mFileLine;
        NSMutableString* mLogString;
    };

protected:
    class LogEntry
    {
    public:
        LogEntry(void);

        void Set(const int nIndex, const LogMsgType msgType, const char* sFullFuncName, const char* sLogMessage);

    public:
        LogMsgType  	mMsgType;
        time_t      	mTimestamp;
        std::string	msFullFuncName;
        std::string    msLogMessage;
        int			    mnIndex;
    };

public:
    static Logger* Instance();
    static LogEntryStreamer NewLogStatement(LogMsgType msgType, const char* sFileName, const char* sFullFuncName, int fileLine);

    void Destroy();

    Logger ( void );
    ~Logger( void );

    static void initializeMutex ();
    static void deleteMutex();

    bool Init(NSString* sLogDataFileName, bool logDebug);         // Takes the name of the file to view data and in which to Log the data (sans file extension).  If none provided, the log will not be saved to disk.
    bool Shutdown();
    bool Log(LogMsgType msgType, const char* sFullFuncName, const char* sLogMessage, const char* sSourceFile, int fileLine);
    bool Log(LogMsgType msgType, const char* sFullFuncName, NSString* sLogMessage, const char* sSourceFile, int fileLine);
    
protected:
    // write a raw string to the logfile (no formatting applied)
    void WriteRawStringToLogfile(NSString *sString);

    // format and write a rLogEntry to the html log file
    void WriteToLogfile(const LogEntry& rLogEntry);

    bool OpenLogFile(NSString* sLogDataFileName);

    // for a given msgType, return the text name for that message type
    NSString*GetMsgTypeFormattingInfo(const LogMsgType msgType) const;

    NSString* FormatLogEntryRaw (const LogEntry &rLogEntry);

protected:

    bool				mbInitted;
    
    NSFileHandle*      	mFile;  // File into which the log is written.
    NSString*		    mLogDataFileName;
    NSString*		    mLogDirectory;

    unsigned int		mnUnloggedEntryCnt_Disk;		// Buffered entries not yet committed to disk
    unsigned int		mnNextEntryIndex;

    int					mCurrEntryIndex;
    unsigned int    	mCurFileSize;

    bool				mLogDebug;
};

namespace LoggerFilter
{
    void DumpCommandLineToLog(const char* header, const wchar_t *cmdline);
};


#define ORIGINBS_LOG_ERROR   Logger::NewLogStatement(kLogError, __FILE__, __FUNCTION__, __LINE__)
#define ORIGINBS_LOG_WARNING Logger::NewLogStatement(kLogWarning, __FILE__, __FUNCTION__, __LINE__)
#define ORIGINBS_LOG_DEBUG   Logger::NewLogStatement(kLogDebug, __FILE__, __FUNCTION__, __LINE__)
// Log that the system performed some action
#define ORIGINBS_LOG_EVENT   Logger::NewLogStatement(kLogEvent, __FILE__, __FUNCTION__, __LINE__)
// Log that the user performed some action
#define ORIGINBS_LOG_ACTION  Logger::NewLogStatement(kLogAction, __FILE__, __FUNCTION__, __LINE__)

#endif // LOGSERVICE_H
