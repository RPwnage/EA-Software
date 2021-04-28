//  LogService.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "LogService.h"

#include <algorithm>
#include <string>
#include <time.h>
#include <sstream>

using namespace std;

static pthread_mutex_t     mMutex;

NSString* localTimeStr()
{
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );

    NSString* str = [NSString stringWithUTF8String:(asctime(timeinfo))];

    //asctime returns a string with carriage return in it, which we don't want so strip it out
    return [str stringByTrimmingCharactersInSet:[NSCharacterSet newlineCharacterSet]];
}

void LogService::init()
{
    pthread_mutex_init(&mMutex, NULL);
}

void LogService::release()
{
    Logger::Instance()->Destroy();
    pthread_mutex_destroy(&mMutex);
}

void LogService::releaseAndExitProcess ()
{
    ORIGINBS_LOG_EVENT << "releaseAndExitProcess called";
    release ();
    exit (0);
}

///////////////////////////////////////////////////////////////////////////////
//	GLOBAL FUNCTION
///////////////////////////////////////////////////////////////////////////////



///////////////////////////////////////////////////////////////////////////////
//	LOGGER
///////////////////////////////////////////////////////////////////////////////
// Singleton static instance - note that this isn't a static member variable,
// just static (internal) linkage.
static Logger* sLogger = NULL;

Logger* Logger::Instance()
{
    // Obtain mutex lock to prevent another thread calling Destroy while 
    // we're executing.
    pthread_mutex_lock(&mMutex);

    if (sLogger == NULL)
        sLogger = new Logger();

    pthread_mutex_unlock(&mMutex);
    return sLogger;
}

void Logger::Destroy()
{
    // Ensure that we don't destroy the logger while we're logging - note
    // that it's still possible, however, for another tread to get the Logger
    // instance pointer and have another thread call destroy.
    pthread_mutex_lock(&mMutex);

    delete sLogger;
    sLogger = NULL;
    pthread_mutex_unlock(&mMutex);
}

Logger::Logger( void )
{
    mbInitted = false;
    mCurrEntryIndex = 0;
}

Logger::~Logger( void )
{
    if ( mbInitted ) 
    { 
        Shutdown(); 
    }
}


// opens a log in the sFileName logFile.
bool Logger::Init(NSString* sLogDataFileName, bool logDebug)
{
    bool bResult = false;

    if ( !mbInitted )
    {
        BOOL isDirectory;
        //Make sure the folder exists or is created
        NSString* logDirectory = [sLogDataFileName stringByDeletingLastPathComponent];
        
        if (![[NSFileManager defaultManager] fileExistsAtPath:logDirectory isDirectory:&isDirectory])
        {
            [[NSFileManager defaultManager] createDirectoryAtPath:logDirectory withIntermediateDirectories:YES attributes:nil error:nil];
        }
        else if(!isDirectory)
        {
            NSLog(@"ERROR: Log file directory is blocked by file with same name at path [%@]", logDirectory);
            return false;
        }

        // Start log data file
        bResult = mbInitted = OpenLogFile(sLogDataFileName);

        // Log the start message.
        NSString* sTextFilename = [NSString stringWithFormat:@"LogFile: %@", sLogDataFileName];
        Log(kLogEvent, __FUNCTION__, [sTextFilename copy], __FILE__, __LINE__);
        Log(kLogEvent, __FUNCTION__, [localTimeStr() copy], __FILE__, __LINE__);

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
    if (mFile != nil)
    {
        // Close file
        [mFile closeFile];
    }

    mbInitted = false; 
    return true;
}

// tries to open a logfile
// sLogDataFileName should be base name without file extension
bool Logger::OpenLogFile(NSString *sLogDataFileName)
{
    if (!sLogDataFileName ||[sLogDataFileName length] == 0)
        return false;

    // The file overwrites any previous log
    mLogDataFileName = [NSString stringWithFormat:@"%@.txt", [sLogDataFileName stringByExpandingTildeInPath]];

    if([[NSFileManager defaultManager] fileExistsAtPath:mLogDataFileName])
    {
        mFile = [[NSFileHandle fileHandleForUpdatingAtPath:mLogDataFileName] retain];
    }
    else // Create the file
    {
        [[NSFileManager defaultManager] createFileAtPath:mLogDataFileName contents:nil attributes:nil];
        mFile = [[NSFileHandle fileHandleForWritingAtPath:mLogDataFileName] retain];
    }
    
    if (mFile == nil)
    {
        NSLog(@"Failed to open log file at %@", mLogDataFileName);
        return false;
    }

    // Ensure access to standard users too if running as admin
    // LogServicePlatform::grantEveryoneAccessToFile(mLogDataFileName);

    // get length of file:
    [mFile seekToEndOfFile];
    mCurFileSize = [mFile offsetInFile];

    return true;
}
 
Logger::LogEntryStreamer Logger::NewLogStatement(LogMsgType msgType, const char* sFileName, const char* sFullFuncName, int fileLine)
{
    // we need a static fail safe instance, otherwise is goes out of scope after the return and crashes!!!
    static Logger::LogEntryStreamer failSafeInstance = Logger::LogEntryStreamer();
    if (!sLogger)
        return failSafeInstance;
    return Logger::LogEntryStreamer(msgType, sFileName, sFullFuncName, fileLine);
}

bool Logger::Log(LogMsgType msgType, const char* sFullFuncName, NSString* sLogMessage, const char* sSourceFile, int fileLine)
{
    return Log(msgType, sFullFuncName, [sLogMessage UTF8String], sSourceFile, fileLine);
}

bool Logger::Log(LogMsgType msgType, const char* sFullFuncName, const char* sLogMessage, const char* sSourceFile, int fileLine)
{
    // Obtain a lock here to prevent "this" from getting destroyed while 
    // we're logging.
    pthread_mutex_lock(&mMutex);

    if ((msgType == kLogDebug || msgType == kLogHelp) && !mLogDebug)
    {
        pthread_mutex_unlock(&mMutex);
        return mbInitted;
    }

    // Log file output
    if ( mbInitted )
    {
        //Create the log entry and then write it
        LogEntry entry;
        entry.Set(mCurrEntryIndex, msgType, sFullFuncName, sLogMessage);

        mCurrEntryIndex++;

        WriteToLogfile(entry);

        if (mCurFileSize > LGR_FILE_SIZE)
        {
            NSLog(@"Log file has reached max size, shrinking it.");
            
            //Open up a stream, go to the beginning of the file and read it all into a string
            [mFile synchronizeFile];
            [mFile closeFile];
            [mFile release];
            
            NSStringEncoding encoding;
            NSError* error = nil;
            NSString* data = [NSString stringWithContentsOfFile:mLogDataFileName usedEncoding:&encoding error:&error];

            if (data)
            {
                //when file is written out, it has carriage return (0xD) and line feed (0xA)
                //but when it is read in, it removes carriage returns so that actual # of bytes read in is less than
                //what the file size was
                unsigned int length = [data length];    //update with actual bytes read in

                //Want to cut off the front x% from the log and update it
                mCurFileSize = (mCurFileSize * static_cast <unsigned int>((1.f - LGR_TRUNCATE_SIZE) * 100.0f)) / 100;

                //offset into the buffer 
                unsigned int startNdx = static_cast<unsigned int>(length) - mCurFileSize;

                //cut off any leading junk
                unsigned int i;
                for (i = startNdx; i < mCurFileSize; i++)
                {
                    if ([data characterAtIndex:i] == '\n')
                        break;
                }

                //update the starting index into the buffer
                if (i < mCurFileSize)
                {
                    mCurFileSize -= (i - startNdx);
                    startNdx = i;
                }

                //Write truncated log back to file
                [[data substringFromIndex:startNdx] writeToFile:mLogDataFileName atomically:YES encoding:encoding error:&error];
                
                // Ensure access to standard users too if running as admin
                // LogServicePlatform::grantEveryoneAccessToFile(mLogDataFileName);
                
                //Reopen file and continue
                mFile = [[NSFileHandle fileHandleForUpdatingAtPath:mLogDataFileName] retain];
                
                if (mFile == nil)
                {
                    NSLog(@"Failed to open log file at %@", mLogDataFileName);
                    return false;
                }

                // get length of file:
                [mFile seekToEndOfFile];
                mCurFileSize = [mFile offsetInFile];
            }
        }
    }
    
    pthread_mutex_unlock(&mMutex);
    return mbInitted;
}

// format and write a rLogEntry to the html log file
void Logger::WriteToLogfile(const LogEntry &rLogEntry)
{
    if( mFile )
    {
        NSString* rawLogEntry = FormatLogEntryRaw (rLogEntry);
        WriteRawStringToLogfile(rawLogEntry);
        [rawLogEntry release];
    }
}

NSString* Logger::FormatLogEntryRaw (const LogEntry &rLogEntry)
{

    NSMutableString* formatString = [[NSMutableString alloc] init];

    if (rLogEntry.mnIndex == 0)
    {
        [formatString appendString:@"\n*************************\nOrigin Bootstrapper\n*************************\n\n"];
    }

    //output the index
    [formatString appendFormat:@"%d:\t", rLogEntry.mnIndex];

    //output the timestamp
    struct tm * timeInfo;
    timeInfo = localtime (&rLogEntry.mTimestamp);
    char tmpStr [256];
    strftime (tmpStr, 256, "[%b %d %H:%M:%S] ", timeInfo);
    [formatString appendFormat:@"%s", tmpStr];
    
    //output the type
    [formatString appendFormat:@"%7@\t", GetMsgTypeFormattingInfo(rLogEntry.mMsgType)];

    //finally output the message
    [formatString appendFormat:@"%s\n", (rLogEntry.msLogMessage.c_str())];

    return formatString;
}

// write a raw string to the logfile - no formatting is applied
void Logger::WriteRawStringToLogfile(NSString* sString)
{
    if( mFile )
    {
        NSData* data = [sString dataUsingEncoding:NSUTF8StringEncoding];
        [mFile writeData:data];

        mCurFileSize = static_cast <unsigned int> ([mFile offsetInFile]);
    }
}

// for a msgType, get the formatting string
NSString* Logger::GetMsgTypeFormattingInfo(const LogMsgType msgType) const
{
    switch( msgType )
    {
    case kLogUnused:
        return @"UNUSED ";
        break;
    case kLogDebug:
        return @"Debug  ";
        break;
    case kLogWarning:
        return @"Warning";
        break;
    case kLogError:
        return @"Error  ";
        break;
    case kLogHelp:
        return @"Help   ";
        break;
    case kLogEvent:
        return @"Event  ";
        break;
    case kLogAction:
        return @"Action ";
        break;
    default:
        return @"";
    }
}

///
/// LOGENTRYSTREAMER
///
Logger::LogEntryStreamer::LogEntryStreamer(LogMsgType msgType, const char* sFileName, const char* sFullFuncName, int fileLine)
    : mMsgType(msgType), mFileName(sFileName), mFullFuncName(sFullFuncName), mFileLine(fileLine)
{
}

Logger::LogEntryStreamer::LogEntryStreamer() {}

Logger::LogEntryStreamer::~LogEntryStreamer()
{
    //this could be called for the destruction of the static one that's declared in NewLogStatement (so that it won't clear the stack when it goes out of scope)
    //but by the time the desctructor for that is called, we would have already called LogService::release which would get rid of sLogger as well as deleted the
    //critical section
    if (sLogger)
    {
        // We obtain a mutex lock here to ensure the pointer returned by the
        // instance() method is still valid when executing the Log method.
        pthread_mutex_lock(&mMutex);

        Logger::Instance()->Log(mMsgType, mFullFuncName, mLogString, mFileName, mFileLine);
        
        pthread_mutex_unlock(&mMutex);        
    }
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( bool t )
{
    [mLogString appendString:(t ? @"true" : @"false")];
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( char c )
{
    [mLogString appendFormat:@"%c", c];
    return *this;
}


Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( signed short i )
{
    [mLogString appendFormat:@"%hd", i];
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( unsigned short i )
{
    [mLogString appendFormat:@"%hu", i];
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( signed int i )
{
    [mLogString appendFormat:@"%d", i];
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( unsigned int i )
{
    [mLogString appendFormat:@"%u", i];
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( signed long l )
{
    [mLogString appendFormat:@"%ld", l];
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( unsigned long l )
{
    [mLogString appendFormat:@"%lu", l];
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( float f )
{
    [mLogString appendFormat:@"%f", f];
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( double f )
{
    [mLogString appendFormat:@"%f", f];
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( const char * s )
{
    [mLogString appendFormat:@"%s", s];
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( const std::string& str)
{
    [mLogString appendFormat:@"%s", str.c_str()];
    return *this;
}

///
/// LOGENTRY
///

Logger::LogEntry::LogEntry()
    :mMsgType( kLogUnused )
{
}

void Logger::LogEntry::Set(const int nIndex, const LogMsgType msgType, const char* sFullFuncName, const char* sLogMessage)
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
//        CensorString(msLogMessage);
    }
#endif

    // Time stamp
    time (&mTimestamp);
}
