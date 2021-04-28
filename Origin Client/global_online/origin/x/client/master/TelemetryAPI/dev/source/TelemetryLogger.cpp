///////////////////////////////////////////////////////////////////////////////
// TelemetryLogger.cpp
//
// A singleton class to log telemetry events into a local file.
//
// Created by Origin DevSupport
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#include "TelemetryLogger.h"
#include "TelemetryConfig.h"
#include "TelemetryPersona.h"

#include "Utilities.h"
#include "version/version.h"

#include "EAAssert/eaassert.h"
#include "EAStdC/EADateTime.h"
#include "EAStdC/EARandomDistribution.h"
#include <EAIO/EAIniFile.h>
#include "EAIO/EAFileUtil.h"
#include "EAStdC/EASprintf.h"
#include "eathread/eathread_futex.h"
#include "eathread/eathread.h"


namespace OriginTelemetry
{
    // CONSTANTS
    
    // Max size of the obfuscated file is 4MB
    const uint32_t TELEMETRY_XML_ERROR_MAX_SIZE = 4000000;

    const eastl::string8 END_TAG = "</Telemetry_XML>";
    const eastl::string8 END_SESSION_TAG = "</Telemetry_Session>";
    const eastl::string8 END_PERSONA_TAG = "</Persona>";

#ifdef ORIGIN_PC
    const size_t LINEFEED_SIZE = sizeof("\n");
#else //On mac we need the linefeed size to be 1 for it to work.
    const size_t LINEFEED_SIZE = 1;
#endif

    const size_t END_TAG_SIZE = END_TAG.length() + LINEFEED_SIZE;
    const size_t END_SESSION_TAG_SIZE = END_SESSION_TAG.length() + LINEFEED_SIZE;
    const size_t END_PERSONA_TAG_SIZE = END_PERSONA_TAG.length() + LINEFEED_SIZE;


    TelemetryLogger::TelemetryLogger():
        m_lock(new EA::Thread::Futex()),
        m_isPersonalWritten(false)
    {
        EA::Thread::AutoFutex lock(getLock());

        NonQTOriginServices::Utilities::getIOLocation(TelemetryConfig::originLogPath(), m_ErrorLogFile);
        m_ErrorLogFile += TelemetryConfig::telemetryErrorFileName();

        // Create the error logger
        // TODO: Truncate error log file once it hits a certain size threshold
        // TODO tmobbs handle failure to create the error log.
        createFile(m_ErrorLogFile, ERROR_LOG);

        // If we're using the XML override, create the XML file
        if(TelemetryConfig::isTelemetryXMLEnabled())
        {
            m_isTelemetryXML = true;
            NonQTOriginServices::Utilities::getIOLocation(TelemetryConfig::telemetryDataPath(), m_XmlLogFile);
            m_XmlLogFile += TelemetryConfig::telemetryLogFileName();
            //TODO tmobbs handle failure to create the log file
            createFile(m_XmlLogFile, XML_LOG);
        }
    }

    TelemetryLogger::~TelemetryLogger()
    {
        TELEMETRY_DEBUG( "Telemetry Logger Destructor");

        //delete the lock.
        if(m_lock && m_lock->HasLock())
        {
            m_lock->Unlock();
        }
        if(m_lock)
        {
            delete m_lock;
            m_lock = NULL;
        }
    }

    EA::Thread::Futex& TelemetryLogger::getLock()
    {
        return *m_lock; 
    } 

    bool TelemetryLogger::clearExcessiveLogFile( const eastl::wstring &logFile ) const
    {
        // Check if the filesize has exceeded the maximum limit
        if(EA::IO::File::GetSize(logFile.c_str()) > TELEMETRY_XML_ERROR_MAX_SIZE)
        {
            // Clear the file
            FILE *clearFile = NonQTOriginServices::Utilities::openFile( logFile, L"w" );
            if (clearFile)
            {
                fclose(clearFile);
                return true;
            }  
        }
        return false;
    }

    LoggingErrorCode TelemetryLogger::createFile(eastl::wstring &logFile, LoggerType typeOfLog)
    {
        // see if we are creating a new LOG file or appending to the existing one
        bool isNewFile = true;
        if(EA::IO::File::Exists(logFile.c_str()))
        {
            isNewFile = false;
            // Only attempt to clear obfuscated file, and not XML
            if(typeOfLog == ERROR_LOG)
            {
                // If the file is cleared, treat it as a new file so that headers are appended
                isNewFile = clearExcessiveLogFile(logFile);
            }
        }

        // open the LOG file for writing 
        FILE *file = openFileHelper(typeOfLog, isNewFile);
        if(file == NULL)
        {
            logFile.erase();
            //TODO we should send a telemetry event about the failure here.
            return LOGGING_CANNOT_CREATE_FILE;
        }


        bool writeProblem = false;

        if(ERROR_LOG == typeOfLog)
        {
            // this is a tag character we use to detect a reset to the random generator that we 
            // use to obfuscate the file.
            if(fwrite(RANDOM_GENERATOR_RESET_CHARACTER.c_str(), 1, 1, file) != 1)
                writeProblem = true;
        }

        // if the LOG file is new (just created) we need to write the header
        if(isNewFile)
        {
            // write header
            if(internalWriteData("<?xml version=\"1.0\"?>", file, typeOfLog) == -1)
                writeProblem = true;
            if(internalWriteData("<Telemetry_XML>", file, typeOfLog) == -1)
                writeProblem = true;
        }
        else
        {
            //No end tags for the obfuscated file
            if(typeOfLog == XML_LOG)
                internalSeekFileBeforeClosingTag(file, END_TAG_SIZE);
        }

        if(internalWriteData("<Telemetry_Session>", file, typeOfLog) == -1)
            writeProblem = true;

        //No end tags for the obfuscated file
        if( typeOfLog == XML_LOG)
        {
            // write the header for the new session
            if(internalWriteData( END_SESSION_TAG, file, typeOfLog) == -1)
                writeProblem = true;

            // write the header for the new session
            if(internalWriteData( END_TAG, file, typeOfLog) == -1)
                writeProblem = true;
        }

        fflush(file);
        fclose(file);

        // return an error if there was a write error
        if(writeProblem)
        {
            //TODO tmobbs need to add handling if we can't write to file.
            return LOGGING_CANNOT_WRITE_TO_FILE;
        }

        return LOGGING_NO_ERROR;
    }

    void TelemetryLogger::logPersona(uint64_t nucleusId, uint32_t localeInt )
    {
        // prepare the header string
        eastl::string8 nucidAsString;
        nucidAsString.append_sprintf("%I64u", nucleusId);

        eastl::string8 locale;

        // Ensure that the persona's locale has been initialized
        if(localeInt != 0)
            locale.append_sprintf("%c%c%c%c", ((char) ((localeInt >> 24)&0xFF)), ((char) ((localeInt >> 16)&0xFF)), ((char) ((localeInt >> 8)&0xFF)), ((char) (localeInt&0xFF)));
        else
            // Initialize locale string as "xx"
            locale.append("xx");

         eastl::string8 userInfo = "";

        if(m_isPersonalWritten)
        {
            userInfo += END_PERSONA_TAG + "\n";
        }

        userInfo += "<Persona nucleusId=\"" + nucidAsString + "\" locale=\"" + locale + "\" machash=\"" + TelemetryConfig::uniqueMachineHash() + "\" >";

        if(!m_isPersonalWritten)
        {
            userInfo += "\n" + END_PERSONA_TAG;
        }

        writeData(userInfo, ERROR_LOG);
        writeData(userInfo, XML_LOG);

        m_isPersonalWritten = true;
    }

    
    void TelemetryLogger::logUnsentTelemetryEvent(const TelemetryEvent &ev)
    {
        // Do not write to the telemetry XML file if we're not using the override
        if(!m_isTelemetryXML )
            return;
        
        internalLogEvent("Unsent_Metric", ev, XML_LOG);
    }

    void TelemetryLogger::logTelemetryEvent(const TelemetryEvent &ev)
    {
        // Do not write to the telemetry XML file if we're not using the override
        if(!m_isTelemetryXML )
            return;
        
        //We don't log event to the error log.
        internalLogEvent("Metric", ev, XML_LOG);
    }

    void TelemetryLogger::logLostTelemetryEvent(const TelemetryEvent &ev)
    {
        internalLogEvent("Lost_Metric", ev, XML_LOG);
        internalLogEvent("Lost_Metric", ev, ERROR_LOG);
    }

    void TelemetryLogger::internalLogEvent( const eastl::string8& tag, const TelemetryEvent& ev, LoggerType loggerType )
    {
        eastl::string8 timestamp;
        timestamp.append_sprintf("%I64u", ev.getTimeStampUTC());

        eastl::string8 line = "<" + tag + " TS=\"" + timestamp + "\" MGS=\"" + ev.getModuleStr() + "." + ev.getGroupStr() + "." + ev.getStringStr() 
            + "\" crit=\"" + ((ev.isCritical() == CRITICAL) ? "1":"0") + "\" count=\"" + eastl::string8().append_sprintf("%lu", ev.eventCount()) 
            + "\" " + ((ev.isCritical() == CRITICAL)? "ccount=\"":"ncount=\"") + eastl::string8().append_sprintf("%lu", ev.eventCriticalTypeCount()) 
            + "\"" + " ver_=\"" + EALS_VERSION_P_DELIMITED + "\" subs=\"" + ev.subscriberStatus() + "\"" + ev.getParams() + "/>";

        writeData(line, loggerType);
    }

    eastl::string8 buildMsg(const eastl::string8& msg, const eastl::string8& typeMsg)
    {
        EA::Thread::ThreadUniqueId tid;
        EAThreadGetUniqueId(tid);
        eastl::string8 myMsg;
        myMsg.append_sprintf("{%lu} [%s] ", tid, typeMsg.c_str());
        myMsg.append(msg);
        return myMsg;
    }

    void TelemetryLogger::logTelemetryWarning( const eastl::string8& msg )
    {
        internalLogMessage( "Warning", msg );
#ifdef ORIGIN_PC 
#ifdef DEBUG
        DebugTrace(buildMsg(msg,"warning"));
#endif
#endif

    }

#ifdef ORIGIN_PC 
#ifdef DEBUG
    void TelemetryLogger::logTelemetryDebug( const eastl::string8& msg )
    {
        //Put the thread id on debug messages.
        eastl::string8 msgWithTid = buildMsg(msg,"information");;
        internalLogMessage( "Debug", msgWithTid );
        DebugTrace(msgWithTid);
    }

    void TelemetryLogger::DebugTrace(const eastl::string8& line)
    {
        if (0 == TelemetryConfig::spamLevel())
            return;

        eastl::string8 s; 

        s += getTimeStampString() + " " + line + "\n"; 

        std::wstring ws(s.size(), L' '); // Overestimate number of code points.
        ws.resize(mbstowcs(&ws[0], s.c_str(), s.size())); // Shrink to fit.
        OutputDebugStringW( ws.c_str());  
    }
#endif 
#endif  

    void TelemetryLogger::logTelemetryError( const eastl::string8& msg )
    {
        internalLogMessage( "Error", msg );
#ifdef ORIGIN_PC 
#ifdef DEBUG
        DebugTrace(buildMsg(msg,"error"));
#endif
#endif
    }

    void TelemetryLogger::internalLogMessage( const eastl::string8& tag, const eastl::string8& msg )
    {
        eastl::string8 line = "<"+tag+" TS=\"" + getTimeStampString() + "\" msg=\"" + msg + "\" />"; 
        writeData(line, XML_LOG);
        writeData(line, ERROR_LOG);
    }

    const eastl::string8 TelemetryLogger::getTimeStampString()
    {
        auto my_time = (EA::StdC::GetTime() / UINT64_C(1000000000));

        // prepare the header string
        eastl::string8 timestamp;
        timestamp.append_sprintf("%I64u", my_time);

        return timestamp;
    }

    void TelemetryLogger::obfuscateString( eastl::string8& strToScramble )
    {
        size_t length = strToScramble.length();
        if (0 == length)
        {
            return;
        }
        static EA::StdC::RandomLinearCongruential random(INITIAL_SEED);

        const char* const str = strToScramble.c_str();
        // scramble data

        // storage for the scrambled data
        eastl::string8 scrambled;
        // Serbian proverb: "What one jerk messes up, ten intelligent people cannot untangle."
        // TODO consider using an actual encryption algorithm with a public private key.
        char8_t current = str[length-1] << 4;
        for(unsigned int i=0; i<length; i++)
        {
            current = (current & 0xF0) | ((str[i] >> 4) & 0x0F);
            char8_t final = current ^ (int8_t) EA::StdC::Random256(random);
            // see if escaping is needed
            switch(final)
            {
                // we use '\0' to mark the end of string
                // since this is not an array of printable characters,
                // it is possible that one of the bytes will have the value of '\0'
                // if that is the case, substitute it with 'D''N'
                // to avoid false end of string
            case END_OF_STRING:
                scrambled += ESCAPE_CHAR;
                final = END_OF_STRING_ESC;
                break;

                // also be carful to avoid a false "end of file"
            case EOF:
                scrambled += ESCAPE_CHAR;
                final = END_OF_FILE_ESC;
                break;

                // escape false "end of line"
            case END_OF_LINE:
                scrambled += ESCAPE_CHAR;
                final = END_OF_LINE_ESC;
                break;

                // escape "reset random generator" character
            case RESET_GENERATOR_ESC:
                scrambled += ESCAPE_CHAR;
                final = RESET_GENERATOR_ESC;
                break;

                // finally, we need to escape the escape character
            case ESCAPE_CHAR:
                scrambled += ESCAPE_CHAR;
                break;

            default:
                // no escaping is needed
                break;
            }

            scrambled += final;
            current = str[i] << 4;
        }

        strToScramble = scrambled;
    }


    FILE* TelemetryLogger::openFileHelper( LoggerType typeOfLog, bool isNewFile )
    {
        eastl::wstring filename = m_ErrorLogFile;
        eastl::wstring modePrefix = L"a";
        eastl::wstring modeSuffix = L"b";


        if( typeOfLog == XML_LOG)
        {
            filename = m_XmlLogFile;
            modePrefix = L"r";
            modeSuffix = L"t";

            if( isNewFile)
                modePrefix = L"w";
        }

        eastl::wstring fileMode = modePrefix + L"+" + modeSuffix;
        return NonQTOriginServices::Utilities::openFile( filename, fileMode );
    }

    void TelemetryLogger::writeData(const eastl::string8& str, LoggerType typeOfLog)
    {
        EA::Thread::AutoFutex lock(getLock());
        if( XML_LOG == typeOfLog && !m_isTelemetryXML)
            return ;

        FILE *file = (FILE *)NULL;
        int retValue = -1;

        file = openFileHelper(typeOfLog);

        // make sure the file is opened
        if(file == NULL)
        {
            EA_FAIL_MSG("TelemetryLogger::writeData failed. Unable to open Telemetry log file");
            return;
        }

        //Don't write out closing tags for the obfuscated file.
        if( typeOfLog == XML_LOG)
        {
            seekFileBeforeClosingTag(file);

            retValue = internalWriteData( str, file, typeOfLog);

            if( m_isPersonalWritten )
            {
                internalWriteData( END_PERSONA_TAG, file, typeOfLog);
            }
            internalWriteData( END_SESSION_TAG, file, typeOfLog);
            internalWriteData( END_TAG, file, typeOfLog);
        }
        else
        {
            retValue = internalWriteData( str, file, typeOfLog);
        }

        //TODO tmobbs handle failures  here.
        EA_ASSERT_MSG( retValue > 0, "Failed to write to telemtery log file");

        // close the file
        fflush(file);
        fclose(file);


    }


    void TelemetryLogger::seekFileBeforeClosingTag(FILE * file)
    {
        size_t sizeToSeek = END_TAG_SIZE + END_SESSION_TAG_SIZE;
        if( m_isPersonalWritten )
        {
            sizeToSeek += END_PERSONA_TAG_SIZE;
        }
        internalSeekFileBeforeClosingTag(file, sizeToSeek);
    }

    void TelemetryLogger::internalSeekFileBeforeClosingTag( FILE* file, size_t sizeOfClosingTag)
    {
        // set the cursor at the bottom of the file but before the closing XML tag
        fseek(file, -((int32_t)sizeOfClosingTag), SEEK_END);
    }

    int32_t TelemetryLogger::internalWriteData(const eastl::string8& str, FILE* file, LoggerType typeOfLog)
    {
        eastl::string8 stringToWrite = str;

        // write to file
        if(typeOfLog == ERROR_LOG)
        {
            obfuscateString(stringToWrite);
        }

        stringToWrite.append("\n");
        if(ERROR_LOG == typeOfLog)
            stringToWrite.append("\0");

        return fwrite(stringToWrite.c_str(), 1, stringToWrite.length(), file);
    }

} //namespace OriginTelemetry
