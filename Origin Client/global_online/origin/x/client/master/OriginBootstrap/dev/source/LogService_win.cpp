//  LogService.cpp
//  Copyright 2012 Electronic Arts Inc. All rights reserved.

#include "LogService_win.h"
#include <Windows.h>

#include <algorithm>
#include <string>
#include <time.h>
#include <sstream>

using namespace std;

std::wstring stringToWString(const std::string& s)
{
    std::wstring temp(s.length(),L' ');
    std::copy(s.begin(), s.end(), temp.begin());
    return temp;
}

std::string wstringToString (const std::wstring& ws)
{
    std::string temp(ws.length(), ' ');
    std::copy(ws.begin(), ws.end(), temp.begin());
    return temp;
}

std::wstring localTimeStr()
{
    time_t rawtime;
    struct tm * timeinfo;

    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
	char mbstr[100];
	strftime(mbstr, sizeof(mbstr), "%c", timeinfo);
    wstring sTextLocalTime = stringToWString (mbstr);

    //asctime returns a string with carriage return in it, which we don't want so strip it out
    string::size_type pos = sTextLocalTime.find_first_of(L"\n", 0);
    if (pos != string::npos)
        sTextLocalTime = sTextLocalTime.substr (0, pos);
    return sTextLocalTime;
}

void LogService::init()
{
    LogServicePlatform::initializeMutex();
}

void LogService::release()
{
    Logger::Instance()->Destroy();
    LogServicePlatform::deleteMutex();
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

//	Filters out token values and persona values from the command-line string
//
namespace LoggerFilter
{
    template <typename T>
    void lower(T& str)
    {
	    std::transform(str.begin(), str.end(), str.begin(), tolower);
    }

    bool contains(const wchar_t* const commandLine, const wchar_t* const token)
    {
	    std::wstring cl(commandLine);
	    lower<std::wstring>(cl);

	    std::wstring tk(token);
	    lower<std::wstring>(tk);

	    const wchar_t* findStr = wcsstr(cl.c_str(), tk.c_str());
	    return NULL != findStr;
    }

    void DumpCommandLineToLog(const char* header, const wchar_t *cmdline)
    {
        ORIGINBS_LOG_DEBUG << "debug header=\"" << header << "\" cmdLine=\"" << cmdline << "\"";

        wstring wCmdLineStr (cmdline);
        wstring logLine;
    	
	    // skip any spaces at the beginning
        string::size_type lastPos = wCmdLineStr.find_first_not_of(L" ", 0);
    	
	    // find first token
        string::size_type pos = wCmdLineStr.find_first_of(L" ", lastPos);

    	while (wstring::npos != pos || wstring::npos != lastPos)
        {
        	// found a token, add it to the vector, but skip any token or personal info
            wstring tok = wCmdLineStr.substr (lastPos, pos - lastPos);
            if (!contains (tok.c_str(), L"token") &&  !contains (tok.c_str(), L"persona"))
            {
                logLine.append (tok);
            }
		
        	// skip any spaces
        	lastPos = wCmdLineStr.find_first_not_of(L" ", pos);
		
        	// find next token
        	pos = wCmdLineStr.find_first_of(L" ", lastPos);
    	}

        ORIGINBS_LOG_EVENT << logLine;
    }
}; //namespace LoggerFilter


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
    LogServicePlatform::lockMutex ();

    if (sLogger == NULL)
        sLogger = new Logger();

    LogServicePlatform::unlockMutex ();
    return sLogger;
}

void Logger::Destroy()
{
    // Ensure that we don't destroy the logger while we're logging - note
    // that it's still possible, however, for another tread to get the Logger
    // instance pointer and have another thread call destroy.
    LogServicePlatform::lockMutex ();

    delete sLogger;
    sLogger = NULL;
    LogServicePlatform::unlockMutex ();
}

Logger::Logger( void )
{
    mbInitted = false;
	mLogDebug = false;

    mCurrEntryIndex			= 0;
	mnUnloggedEntryCnt_Disk = 0;		// Buffered entries not yet committed to disk
	mnNextEntryIndex		= 0;
	mCurFileSize			= 0;
}

Logger::~Logger( void )
{
    if ( mbInitted ) 
    { 
        Shutdown(); 
    }
}


// opens a log in the sFileName logFile.
bool Logger::Init(const wchar_t * sLogDataFileName, bool logDebug)
{
    bool bResult = false;

    if ( !mbInitted )
    {
        //Make sure the folder is created
        wstring filename (sLogDataFileName);

        int nLastIndex = filename.find_last_of('\\');
        wstring sLogDirectory = filename.substr (0, nLastIndex);

        //check and see if directory exists
        if (!LogServicePlatform::pathExists (sLogDirectory.c_str()))
        {
            LogServicePlatform::createPath (sLogDirectory.c_str()); // Create (if it doesn't exist) the full path to our log directory
        }

        // Start log data file
        bResult = mbInitted = OpenLogFile(sLogDataFileName);
            

        // Log the start message.
        wstring sTextFilename (L"LogFile: ");
        sTextFilename.append (sLogDataFileName);
        Log(kLogEvent, __FUNCTION__, sTextFilename.c_str(), __FILE__, __LINE__);
        Log(kLogEvent, __FUNCTION__, localTimeStr().c_str(), __FILE__, __LINE__);

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
    if (mFile.is_open())
    {
        // Close file
        mFile.close();
    }

    mbInitted = false; 
    return true;
}

// tries to open a logfile
// sLogDataFileName should be base name without file extension
bool Logger::OpenLogFile(const wchar_t *sLogDataFileName)
{
    if (!sLogDataFileName ||sLogDataFileName [0] == L'\0')
        return false;

    const wchar_t *fileExtension = L".txt";

    // The file overwrites any previous log
    mLogDataFileName = sLogDataFileName;
    mLogDataFileName.append(fileExtension);

    mFile.open (mLogDataFileName.c_str(), ios_base::in | ios_base::out | ios_base::app | ios_base::ate);
    if (mFile.fail ())
    {
        return false;
    }

    // Ensure access to standard users too if running as admin
    LogServicePlatform::grantEveryoneAccessToFile(mLogDataFileName);

    // get length of file:
    mFile.seekg (0, ios::end);
    mCurFileSize = static_cast <unsigned int>(mFile.tellg());

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

bool Logger::Log(LogMsgType msgType, const char* sFullFuncName, const wchar_t* sLogMessage, const char* sSourceFile, int fileLine)
{
    // Obtain a lock here to prevent "this" from getting destroyed while 
    // we're logging.
    LogServicePlatform::lockMutex ();

    if ((msgType == kLogDebug || msgType == kLogHelp) && !mLogDebug)
    {
        LogServicePlatform::unlockMutex ();
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
            //Open up a stream, go to the beginning of the file and read it all into a string
            mFile.flush();

            //get the filesize
            mFile.seekg(0,std::ios::end);
            std::streampos length = mFile.tellg();
            mFile.seekg(0,std::ios::beg);

            char *buffer = (char *)malloc (static_cast<size_t>(length)+1);
            if (buffer)
            {
                mFile.read (&buffer[0], length);
                //when file is written out, it has carriage return (0xD) and line feed (0xA)
                //but when it is read in, it removes carriage returns so that actual # of bytes read in is less than
                //what the file size was
                length = mFile.gcount();    //update with actual bytes read in

                //clear the eof bit, otherwise all subsequent operations return invalid values
                ios_base::iostate state = mFile.rdstate();
                if (state & ofstream::eofbit)
                {
                    mFile.clear ();
                    mFile.rdstate();
                }

                //Want to cut off the front x% from the log and update it
                mCurFileSize = (mCurFileSize * static_cast <unsigned int>((1.f - LGR_TRUNCATE_SIZE) * 100.0f)) / 100;

                //offset into the buffer 
                unsigned int startNdx = static_cast<unsigned int>(length) - mCurFileSize;

                //cut off any leading junk
                unsigned int i;
                for (i = startNdx; i < mCurFileSize; i++)
                {
                    if (buffer [i] == '\n')
                        break;
                }

                //update the starting index into the buffer
                if (i < mCurFileSize)
                {
                    mCurFileSize -= (i - startNdx);
                    startNdx = i;
                }

                //close the file first so...
                mFile.close();

                //we can open the file as trunc so it will clear the file
                mFile.open (mLogDataFileName.c_str(), ios_base::in | ios_base::out | ios_base::trunc);
                mFile.write (&buffer [startNdx], mCurFileSize);
                mFile.flush();

                mFile.seekp (0, ios::end);
                mFile.seekg (0, ios::end);
                mCurFileSize = static_cast <unsigned int> (mFile.tellp());
                free (buffer);
            }
        }
    }
    LogServicePlatform::unlockMutex ();
    return mbInitted;
}

// format and write a rLogEntry to the html log file
void Logger::WriteToLogfile(const LogEntry &rLogEntry)
{
    if( mFile.is_open() )
    {
        string formattedRaw;
        FormatLogEntryRaw (rLogEntry, formattedRaw);

#ifdef DEBUG
        std::string debugStr = formattedRaw + std::string("\n");
        OutputDebugStringA(debugStr.c_str());
#endif

        WriteRawStringToLogfile(formattedRaw.c_str());
    }
}

void Logger::FormatLogEntryRaw (const LogEntry &rLogEntry, std::string& formattedRaw)
{
    wchar_t tmpStr [256];
    wstring tmpWstr;

    if (rLogEntry.mnIndex == 0)
    {
        tmpWstr.append (L"\n*************************\nOrigin Bootstrapper\n*************************\n\n");
    }

    //output the index
    swprintf (tmpStr, L"%d:\t", rLogEntry.mnIndex);
    tmpWstr.append (tmpStr);

    //output the timestamp
    struct tm * timeInfo;
    timeInfo = localtime (&rLogEntry.mTimestamp);
    wcsftime (tmpStr, 256, L"[%b %d %H:%M:%S] ", timeInfo);
    tmpWstr.append (tmpStr);

    //output the type
    tmpWstr.append (GetMsgTypeFormattingInfo(rLogEntry.mMsgType));
    tmpWstr.append (L"\t");

    //finally output the message
    tmpWstr.append (rLogEntry.msLogMessage);

    formattedRaw = wstringToString (tmpWstr);
}


// write a raw string to the logfile - no formatting is applied
void Logger::WriteRawStringToLogfile(const char *sString)
{
    if( mFile.is_open() )
    {
        //wen need to convert it to char  string

        mFile << sString << endl;

        mCurFileSize = static_cast <unsigned int> (mFile.tellp());
    }
}

// for a msgType, get the formatting string
wchar_t *Logger::GetMsgTypeFormattingInfo(const LogMsgType msgType) const
{
    switch( msgType )
    {
    case kLogUnused:
        return L"UNUSED";
        break;
    case kLogDebug:
        return L"Debug";
        break;
    case kLogWarning:
        return L"Warning";
        break;
    case kLogError:
        return L"Error";
        break;
    case kLogHelp:
        return L"Help";
        break;
    case kLogEvent:
        return L"Event";
        break;
    case kLogAction:
        return L"Action";
        break;
    default:
        return L"";
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
        LogServicePlatform::lockMutex ();

        Logger::Instance()->Log(mMsgType, mFullFuncName, mLogString.c_str(), mFileName, mFileLine);

        LogServicePlatform::unlockMutex ();
    }
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( bool t )
{
    mLogString.append(t ? L"true" : L"false");
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( char c )
{
    wchar_t charStr [32];
    swprintf (charStr, L"%c", c);
    mLogString.append (charStr);
    return *this;
}


Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( signed short i )
{
    wchar_t numStr [32];
    swprintf (numStr, L"%hd", i);
    mLogString.append (numStr);
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( unsigned short i )
{
    wchar_t numStr [32];
    swprintf (numStr, L"%hu", i);
    mLogString.append(numStr);
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( signed int i )
{
    wchar_t numStr [32];
    swprintf (numStr, L"%d", i);
    mLogString.append(numStr);
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( unsigned int i )
{
    wchar_t numStr [32];
    swprintf (numStr, L"%u", i);
    mLogString.append(numStr);
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( signed long l )
{
    wchar_t numStr [32];
    swprintf (numStr, L"%ld", l);
    mLogString.append(numStr);
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( unsigned long l )
{
    wchar_t numStr [32];
    swprintf (numStr, L"%lu", l);
    mLogString.append(numStr);
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( float f )
{
    wchar_t numStr [32];
    swprintf (numStr, L"%f", f);
    mLogString.append(numStr);
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( double f )
{
    wchar_t numStr [32];
    swprintf (numStr, L"%f", f);
    mLogString.append(numStr);
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( const char * s )
{
    wstring toWstring = stringToWString (string(s));
    mLogString.append(toWstring);
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( const wchar_t * s )
{
    mLogString.append(s);
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( const std::string& str)
{
    wstring toWstring = stringToWString (str);
    mLogString.append(toWstring);
    return *this;
}

Logger::LogEntryStreamer &	Logger::LogEntryStreamer::operator<< ( const std::wstring& wstr)
{
    mLogString.append(wstr);
    return *this;
}


///
/// LOGENTRY
///

Logger::LogEntry::LogEntry()
    :mMsgType( kLogUnused ), mnIndex(0)
{
}

void Logger::LogEntry::Set(const int nIndex, const LogMsgType msgType, const char* sFullFuncName, const wchar_t* sLogMessage)
{
    if (sFullFuncName && sLogMessage)
    {
        // Chronological index
        mnIndex = nIndex;

        // Entry type
        mMsgType = msgType;

        // Function where log entry was called
        string strFuncName (sFullFuncName);
        msFullFuncName = stringToWString (strFuncName);

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
}
