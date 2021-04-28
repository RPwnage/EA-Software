///////////////////////////////////////////////////////////////////////////////
// TelemetryLogger.h
//
// A singleton class to log telemetry events into a local file.
//
// Created by Origin DevSupport
// Copyright (c) 2015 Electronic Arts. All rights reserved.
///////////////////////////////////////////////////////////////////////////////


#ifndef ORIGINTELEMETRY_TELEMETRYLOGGER_H
#define ORIGINTELEMETRY_TELEMETRYLOGGER_H

#include "EASTL/string.h"

//Forward Declarations
namespace EA
{
    namespace Thread
    {
        class Thread;
        class Futex;
    }
}

#ifdef ORIGIN_PC
#ifdef DEBUG
    #define TELEMETRY_DEBUG(debugstr) OriginTelemetry::TelemetryLogger::GetTelemetryLogger().logTelemetryDebug(debugstr)
#else
    #define TELEMETRY_DEBUG(debugstr)
#endif
#else
#define TELEMETRY_DEBUG(debugstr)
#endif // ORIGIN_PC

namespace OriginTelemetry
{

enum LoggingErrorCode
{
    LOGGING_NO_ERROR = 0,
    LOGGING_BAD_FILENAME,
    LOGGING_CANNOT_WRITE_TO_FILE,
    LOGGING_CANNOT_CREATE_FILE,
    LOGGING_CANNOT_OPEN_FILE
};

    class TelemetryEvent;

    class TelemetryLogger
    {
    public: 
        
        /// destructor.
        ~TelemetryLogger();

        /// function that returns a pointer to logger instance
        static TelemetryLogger& GetTelemetryLogger()
        {
            static TelemetryLogger instance;
            return instance;
        }

        /// function that writes the telemetry event to the log file
        /// \param ev telemetry event to be written to the log file
        void logTelemetryEvent(const TelemetryEvent &ev);

        /// function that writes the unsent telemetry event to the log file
        /// \param ev telemetry event to be written to the log file
        void logUnsentTelemetryEvent(const TelemetryEvent &ev);

        /// function that writes the lost telemetry event to the log file
        /// \param ev telemetry event to be written to the log file
        void logLostTelemetryEvent(const TelemetryEvent &ev);

        /// Log a telemetry persona change
        void logPersona(uint64_t nuculeusId, uint32_t localeInt );

        /// Log a telemetry Warning Message
        void logTelemetryWarning( const eastl::string8& msg );
        /// Log a telemetry Error Message
        void logTelemetryError( const eastl::string8& msg );

#ifdef ORIGIN_PC 
#ifdef DEBUG
        /// Log a telemetry Debug Message
        void logTelemetryDebug( const eastl::string8& msg );
#endif
#endif

    private:

        enum LoggerType
        {
            ERROR_LOG = 0,
            XML_LOG
        };

        /// private default constructor.
        TelemetryLogger();

        ///private (disabled) copy constructor.
        TelemetryLogger( const TelemetryLogger &eventToCopy);

        /// private (disabled) assignment operator.
        TelemetryLogger& operator= (const TelemetryLogger &anotherLogger);

        /// private (disabled) dereference  operator.
        TelemetryLogger& operator&(const TelemetryLogger&);

        void internalLogEvent( const eastl::string8& tag, const TelemetryEvent& ev, LoggerType loggerType );

        /// auxiliary function for writing strings to the LOG file
        /// the function will scramble the data before writing to file if m_isScrambleData is set
        /// it will maintain the closing telemetr ytag in the LOG file
        /// \param str string to be written
        void writeData(const eastl::string8& str, LoggerType typeOfLog);

        ///Internal helper to actually write the string to the file.
        int32_t internalWriteData(const eastl::string8& str, FILE* file, LoggerType typeOfLog);

        /// Helper to seek the file before the closing tags
        void seekFileBeforeClosingTag( FILE* file);

        /// Internal helper for seekFileBeforeClosingTag that let's you specify the size to seek.
        void internalSeekFileBeforeClosingTag( FILE* file, size_t sizeOfClosingTag);

        /// Create a new logging file.
        LoggingErrorCode createFile(eastl::wstring &logFile, LoggerType typeOfLog);

        /// Private helper for opening the correct log file.
        FILE* openFileHelper( LoggerType typeOfLog, bool isNewFile = false );

        ///Helper for logging warnings, errors, and debug messages.
        void internalLogMessage( const eastl::string8& tag, const eastl::string8& msg );

        /// Clear log file when it reaches a certain threshold
        bool clearExcessiveLogFile( const eastl::wstring &logFile ) const;

        ///Helper to get the current timestamp as string.
        const eastl::string8 getTimeStampString();

        /// Get the futuex for the current object
        EA::Thread::Futex &getLock();

        /// \brief lock
        EA::Thread::Futex *m_lock;

        void obfuscateString( eastl::string8& strToScramble );

#ifdef ORIGIN_PC 
#ifdef DEBUG
        void DebugTrace(const eastl::string8& s);
#endif // _DEBUG
#endif 

        /// bool that indicates if we should be writing all events to a telemetryXML file
        bool m_isTelemetryXML;

        // LOG file name
        eastl::wstring m_ErrorLogFile;
        eastl::wstring m_XmlLogFile;

        bool m_isPersonalWritten;

    };

} //namespace OriginTelemetry


#endif // ORIGINTELEMETRY_TELEMETRYLOGGER_H