/////////////////////////////////////////////////////////////////////////////
// EALog_Imp.h
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
//
// Defines an implementation of the Log interface.
//
// Created by Paul Pedriana, Maxis
/////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_EALOG_IMP_H
#define EATRACE_EALOG_IMP_H


#ifndef EA_ALLOCATOR_ICOREALLOCATOR_INTERFACE_H
    #include <coreallocator/icoreallocator_interface.h>
#endif
#ifndef EASTL_CORE_ALLOCATOR_ADAPTER_H
    #include <EASTL/core_allocator_adapter.h>
#endif
#ifndef EATRACE_EALOG_H
    #include <EATrace/EALog.h>
#endif
#ifndef EATRACE_EATRACE_IMP_H
    #include <EATrace/EATrace_imp.h>
#endif
#ifndef EATRACE_EATRACEZONEOBJECT_H
    #include <EATrace/EATraceZoneObject.h>
#endif
#ifndef EATRACE_REFCOUNT_H
    #include <EATrace/internal/RefCount.h>
#endif
#ifndef EASTDC_EASTRING_H
    #include <EAStdC/EAString.h>
#endif
#ifndef EAIO_EAFILESTREAM_H
    #include <EAIO/EAFileStream.h>
#endif

#include <eathread/eathread.h>
#include <eathread/eathread_mutex.h>
#include <eathread/eathread_atomic.h>

#include <EASTL/fixed_vector.h>
#include <EASTL/functional.h>
#include <EASTL/map.h>
#include <EASTL/string.h>
#include <EASTL/fixed_string.h>

#include <stdio.h>


/// namespace EA
/// The standard Electronic Arts namespace.
namespace EA
{
    namespace IO
    {
        class IStream;
        class FileStream;
    }

    namespace Trace
    {
        /// class LogRecord 
        ///
        /// This is a lightweight record, a more elaborate trace record 
        /// could contain a frame number, reporting timestamp, thread 
        /// object (for name/id), stack trace, chained exception, etc.
        ///
        class LogRecord
        {
        public:
            LogRecord()
              : mnRefCount(0), mRecordNumber(-1), mTraceHelper(NULL), mMessageTextAlias(NULL), mLevelNameAlias(NULL)
            { }

            virtual ~LogRecord()
            { }

            void SetTraceHelper(const TraceHelper& helper)
            { 
                mTraceHelper = &helper; 
            }

            const TraceHelper* GetTraceHelper() const
            {
                return mTraceHelper;
            }

            void SetRecordInfo(const int32_t recordNumber, const char8_t* messageText, const char8_t* levelName)
            { 
                mRecordNumber      = recordNumber;
                mMessageTextAlias  = messageText;
                mLevelNameAlias    = levelName;
            }

        public: // LogRecord
            virtual int32_t GetRecordNumber() const
            { return mRecordNumber; }

            virtual const char8_t* GetLevelName() const
            { return mLevelNameAlias; }

            virtual const char8_t* GetMessageText() const
            { return mMessageTextAlias; }
                 
        public:
            int                 mnRefCount;
            int32_t             mRecordNumber;
            const TraceHelper*  mTraceHelper;
            const char8_t*      mMessageTextAlias; 
            const char8_t*      mLevelNameAlias;
        };



        /// LogFormatterSimple
        ///
        /// Implements a log formatter that has basic functionality.
        ///
        class LogFormatterSimple : public ILogFormatter, public Trace::ZoneObject
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x6e9f4237;

            LogFormatterSimple(Allocator::ICoreAllocator* pAllocator = NULL);
            LogFormatterSimple(const char* name, Allocator::ICoreAllocator* pAllocator = NULL);
            virtual ~LogFormatterSimple();

            void SetAllocator(Allocator::ICoreAllocator* pAllocator) { mpCoreAllocator = pAllocator; }
            Allocator::ICoreAllocator* GetAllocator() const          { return mpCoreAllocator; }

        public: // ILogFormatter
            virtual const char*     FormatRecord(const LogRecord& inRecord);
            virtual void            SetName(const char* name);
            virtual const char*     GetName() const;
            virtual ILogFormatter*  Clone();

        public: // IUnknown32
            virtual void* AsInterface(InterfaceId iid);
            virtual int   AddRef();
            virtual int   Release();

        protected:
            Allocator::ICoreAllocator*              mpCoreAllocator;
            int                                     mnRefCount;
            eastl::fixed_string<char, 2048, true>   mName;
            eastl::fixed_string<char, 2048, true>   mLine;
        };


        /// LogFormatterPrefixed
        ///
        /// Implements a log formatter that prefixes a log record's text with its group name.
        ///
        class LogFormatterPrefixed : public LogFormatterSimple
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x046968e1;

            LogFormatterPrefixed(const char* pName, const char8_t* pLineEnd = NULL);
            virtual const char* FormatRecord(const LogRecord& inRecord);

        private:
            eastl::fixed_string<char8_t, 2048, true> mLine;
            const char8_t*                           mpLineEnd;
        };


        /// LogFormatterFancy
        ///
        /// Implements a log formatter that has extended functionality.
        ///
        class LogFormatterFancy : public ILogFormatter, public Trace::ZoneObject
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x0119adc5;

            LogFormatterFancy(Allocator::ICoreAllocator* pAllocator = NULL);
            LogFormatterFancy(const char* name, Allocator::ICoreAllocator* pAllocator = NULL);
            virtual ~LogFormatterFancy();

            void SetAllocator(Allocator::ICoreAllocator* pAllocator)  { mpCoreAllocator = pAllocator; }
            Allocator::ICoreAllocator* GetAllocator() const           { return mpCoreAllocator; }

        public: // ILogFormatter
            virtual const char*     FormatRecord(const LogRecord& inRecord);
            virtual void            SetName(const char* name);
            virtual const char*     GetName() const;
            virtual ILogFormatter*  Clone();

        public: // IUnknown32
            virtual void* AsInterface(InterfaceId iid);
            virtual int   AddRef();
            virtual int   Release();

        public: // LogFormatterFancy
            enum
            {
                kFlagTimeStamp      = 0x0001,   /// Report the current time.
                kFlagDeltaTimeStamp = 0x0002,   /// Report the time as a delta from last time instead of an absolute.
                kFlagIncludeMs      = 0x0004,   /// Report the time as milliseconds instead of seconds.
                kFlagDate           = 0x0008,   /// Report the current date.
                kFlagGroup          = 0x0010,   /// Report the log group.
                kFlagLevel          = 0x0020    /// Report the log level.
            };

            void     SetFlags(uint16_t flags);
            uint16_t GetFlags() const;
            void     SetFileInfoLevel(tLevel level);    ///< Set level above which file location info should be displayed
            tLevel   GetFileInfoLevel() const;          ///< Get level above which file location info should be displayed

        protected:
            Allocator::ICoreAllocator*              mpCoreAllocator;
            int                                     mnRefCount;
            eastl::fixed_string<char, 512, true>    mName;
            uint16_t                                mnFlags;
            tLevel                                  mnFileInfoLevel;
            uint64_t                                mInitialTime;
            eastl::fixed_string<char, 2048, true>   mLine;
        };



        /// LogFilterGroupLevels
        ///
        /// Implements a log filter based on group and level information.
        ///
        class LogFilterGroupLevels : public ILogFilter, public Trace::ZoneObject
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x2e9e25fe;

            typedef eastl::fixed_vector<const char*, 32, true> GroupList;

            LogFilterGroupLevels(Allocator::ICoreAllocator* pCoreAllocator = NULL);
            LogFilterGroupLevels(const char* name, Allocator::ICoreAllocator* pCoreAllocator = NULL);
            virtual ~LogFilterGroupLevels();

            /// SetAllocator
            /// Sets the memory allocator to use with this class.
            /// This allocator is used to allocate GroupLevelMap elements.
            /// This function must be called before any entries are added.
            void SetAllocator(Allocator::ICoreAllocator* pAllocator);
            Allocator::ICoreAllocator* GetAllocator() const { return mpCoreAllocator; }

            void   AddGroupLevel(const char* pGroup, tLevel nLevel);
            bool   RemoveGroupLevel(const char* pGroup);
            tLevel GetGroupLevel(const char* pGroup) const;
            void   GetGroupList(GroupList* pGroupList) const;
            void   Reset();

        public: // ILogFilter
            virtual bool IsFiltered(const TraceHelper& helper) const;
            virtual bool IsFiltered(const LogRecord& record) const;

            virtual const char* GetName() const;
            virtual void        SetName(const char* pName);
            
            virtual ILogFilter* Clone();

        public: // IUnknown32
            virtual void* AsInterface(InterfaceId iid);
            virtual int   AddRef();
            virtual int   Release();

        protected:
            typedef const char* tKeyType;

            struct KeyLess : public eastl::binary_function<tKeyType, tKeyType, bool>
            {
                result_type operator()(first_argument_type lhs, second_argument_type rhs) const
                   { return StdC::Stricmp(lhs, rhs) < 0; } // case insensitive compare
            };

            // tKeyType is a const char* to avoid string creation during filter lookup.
            // Means that stricmp function cannot just test length of string. 
            typedef eastl::map<tKeyType, tLevel, KeyLess, Allocator::EASTLICoreAllocator> GroupLevelMap;

            Allocator::ICoreAllocator*          mpCoreAllocator;
            int                                 mnRefCount;
            eastl::fixed_string<char, 16, true> mName;
            int                                 mGlobalLevel;
            GroupLevelMap                       mGroupLevelMap;
        };




        /// class LogReporter
        ///
        /// This class doesn't need to be thread-safe, as the Server implementation 
        /// implements thread safety around usage of this class.
        ///
        class LogReporter : public ILogReporter, public Trace::ZoneObject
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x046968f7;

            LogReporter();
            LogReporter(const char* pName);
            virtual ~LogReporter();

        public: // ILogReporter
            // this is where the work is done
            virtual bool IsFiltered(const TraceHelper& helper);
            virtual bool IsFiltered(const LogRecord& record);

            virtual tAlertResult Report(const LogRecord& record) = 0;

            virtual const char* GetName() const;
            virtual void        SetName(const char* pName);

            virtual bool IsEnabled() const;
            virtual void SetEnabled(bool isEnabled);

            virtual void SetFlushOnWrite(bool flushOnWrite);
            virtual bool GetFlushOnWrite() const;

            // filter
            virtual void         SetFilter(ILogFilter* filter);
            virtual ILogFilter*  GetFilter();

            // formatter
            virtual void            SetFormatter(ILogFormatter* formatter);
            virtual ILogFormatter*  GetFormatter();

        public:    // IUnknown32 functionality
            virtual void* AsInterface(InterfaceId iid);
            virtual int   AddRef();
            virtual int   Release();

        protected:
            EA::Trace::AutoRefCount<ILogFilter>     mFilter;
            EA::Trace::AutoRefCount<ILogFormatter>  mFormatter;

            // reporter data
            bool                                mIsEnabled;
            bool                                mbFlushOnWrite;
            int                                 mnRefCount;
            eastl::fixed_string<char, 16, true> mName;
        };



        /// LogReporterDebugger
        ///
        /// Reporter that sends output to the debugger's output (a.k.a. TTY) display.
        /// This is equivalent to the OutputDebugString function that is provided by 
        /// Microsoft with their platforms and equivalent to printf(stderr) on platforms 
        /// such as PlayStation and Unix.
        ///
        class LogReporterDebugger : public LogReporter 
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x046968fd;

            LogReporterDebugger(const char8_t* name)
                : LogReporter(name) 
            { }

            virtual bool IsFiltered(const TraceHelper& helper)
            {
                return ((helper.GetOutputType() & kOutputTypeText) == 0) || LogReporter::IsFiltered(helper);
            }

            virtual bool IsFiltered(const LogRecord& record)
            {
                return ((record.GetTraceHelper()->GetOutputType() & kOutputTypeText) == 0) || LogReporter::IsFiltered(record);
            }

            virtual tAlertResult Report(const LogRecord& record);

        public: // IUnknown32
            void* AsInterface(InterfaceId iid)
            {
                if (iid == LogReporterDebugger::kIID) 
                    return this;
                return LogReporter::AsInterface(iid);
            }
        };



        /// LogReporterStdio
        ///
        /// Reporter that sends output to a C FILE destination which defaults to stdout.
        /// Alternatively it might be useful to set it to output to stderr.
        ///
        class LogReporterStdio: public LogReporter 
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x04696907;

            LogReporterStdio(FILE* pFILE = stdout) 
                : mpFILE(pFILE)
            { }

            virtual bool IsFiltered(const TraceHelper& helper)
            {
                return ((helper.GetOutputType() & kOutputTypeText) == 0) || LogReporter::IsFiltered(helper);
            }

            virtual bool IsFiltered(const LogRecord& record)
            {
                return ((record.GetTraceHelper()->GetOutputType() & kOutputTypeText) == 0) || LogReporter::IsFiltered(record);
            }

            virtual tAlertResult Report(const LogRecord& record);

        public: // IUnknown32
            void* AsInterface(InterfaceId iid)
            {
                if (iid == LogReporterStdio::kIID) 
                    return this;
                return LogReporter::AsInterface(iid);
            }

        protected:
            FILE* mpFILE;
        };



        /// LogReporterDialog
        ///
        /// Reporter that sends output to a dialog.
        /// Normally this is used to display assertion failures with kOutputTypeAlert.
        /// The Report function displays the text for the user and gives the user a
        /// choice to break, continue, or ignore. The user is expected to provide 
        /// a response via an input device such as a keyboard or controller.
        /// On platforms which don't support such dialog functionality, this LogReporter
        /// displays nothing on the screen and acts as if the user chose to break.
        ///
        /// Note: As of this writing, this LogReporter only displays a dialog on Windows.
        /// However, Xbox 360 and PS3 versions are being investigated.
        ///
        class LogReporterDialog : public LogReporter 
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x0469690f;
            
            LogReporterDialog(const char* name, tOutputType outputTypes = kOutputTypeAlert)
                : LogReporter(name)
                , mOutputTypes(outputTypes)
            {}

            virtual bool IsFiltered(const TraceHelper& helper)
            {
                return ((helper.GetOutputType() & mOutputTypes) == 0) || LogReporter::IsFiltered(helper);
            }

            virtual bool IsFiltered(const LogRecord& record)
            {
                 return ((record.GetTraceHelper()->GetOutputType() & mOutputTypes) == 0) || LogReporter::IsFiltered(record);
            }

            virtual tAlertResult Report(const LogRecord& record);

        public: // IUnknown32
            void* AsInterface(InterfaceId iid)
            {
                if (iid == LogReporterDialog::kIID) 
                    return this;
                return LogReporter::AsInterface(iid);
            }

        protected:
            tOutputType mOutputTypes;
        };



        /// LogReporterStream
        ///
        /// Reporter that sends output to an arbitrary IStream.
        /// An IStream is a superset of a file and can be used to send output to an arbitrary 
        /// destination that can act like an output stream. An example of such a destination
        /// might be a network destination over TCP/IP.
        ///
        class LogReporterStream : public LogReporter
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x0469691d;

            LogReporterStream(const char8_t* name, IO::IStream* pStream);
           ~LogReporterStream();

            virtual bool IsFiltered(const TraceHelper& helper)
            {
                return ((helper.GetOutputType() & kOutputTypeText) == 0) || LogReporter::IsFiltered(helper);
            }

            virtual bool IsFiltered(const LogRecord& record)
            {
                return ((record.GetTraceHelper()->GetOutputType() & kOutputTypeText) == 0) || LogReporter::IsFiltered(record);
            }

            virtual tAlertResult Report(const LogRecord& record);

        public: // IUnknown32
            void* AsInterface(InterfaceId iid)
            {
                if (iid == LogReporterStream::kIID) 
                    return this;
                return LogReporter::AsInterface(iid);
            }

        public: 
            IO::IStream* mpStream;
        };



        /// LogReporterFile
        ///
        /// Reporter that sends output to a file specified by a path.
        ///
        class LogReporterFile : public LogReporter
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x04696926;

            LogReporterFile(const char8_t* name, const char8_t*  path, Allocator::ICoreAllocator* pAllocator = NULL, bool openImmediately = false);
            LogReporterFile(const char8_t* name, const char16_t* path, Allocator::ICoreAllocator* pAllocator = NULL, bool openImmediately = false);
            LogReporterFile(const char8_t* name, const char32_t* path, Allocator::ICoreAllocator* pAllocator = NULL, bool openImmediately = false);
            #if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
                LogReporterFile(const char8_t* name, const wchar_t* path, Allocator::ICoreAllocator* pAllocator = NULL, bool openImmediately = false);
            #endif

           ~LogReporterFile();

            void SetAllocator(Allocator::ICoreAllocator* )  {  }              // Currently not needed.
            Allocator::ICoreAllocator* GetAllocator() const { return NULL; }  // Currently not needed.

            bool IsFiltered(const TraceHelper& helper)
            {
                return ((helper.GetOutputType() & kOutputTypeText) == 0) || LogReporter::IsFiltered(helper);
            }

            bool IsFiltered(const LogRecord& record)
            {
                return ((record.GetTraceHelper()->GetOutputType() & kOutputTypeText) == 0) || LogReporter::IsFiltered(record);
            }

            bool Open(int nAccessFlags = IO::kAccessFlagWrite, int nCreationDisposition = IO::kCDCreateAlways, int nSharing = IO::FileStream::kShareRead);

            tAlertResult Report(const LogRecord& record);

        public: // IUnknown32
            void* AsInterface(InterfaceId iid)
            {
                if (iid == LogReporterFile::kIID) 
                    return this;
                return LogReporter::AsInterface(iid);
            }

        protected:

            IO::FileStream mFileStream;
            bool           mAttemptedOpen;
        };



        /// class Server
        ///
        /// Really a log manager.  Maintains lists of filters, formatters, and reporters. 
        /// This default implementation only maintains reporters.
        class Server : public IServer, public ITracer, public Trace::ZoneObject
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x0469692a;

            /// This constructs the default implemenation of the log service.
            /// Callers must call Configure() to set a default filter and formatter, 
            /// or else supply one with each active reporter that is added to the services.
            Server(Allocator::ICoreAllocator* pAllocator = NULL);
            Server(const Server&);
            virtual ~Server();
            Server& operator=(const Server&);

            // Allocator access.
            void SetAllocator(Allocator::ICoreAllocator* pAllocator) { mpCoreAllocator = pAllocator; }
            Allocator::ICoreAllocator* GetAllocator() const          { return mpCoreAllocator; }
 
            // Log reporter list access. 
            typedef EA::Trace::AutoRefCount<ILogReporter> LogReporterARC;
            typedef eastl::vector<LogReporterARC, Allocator::EASTLICoreAllocator> LogReporters;

            LogReporters& GetLogReporters() { return mLogReporters; }

        public:  // IServer functionality
            virtual ITracer* AsTracer();
          
            virtual void   Init();
            virtual void   UpdateLogFilters();
            virtual void   SetAllEnabled(bool enabled);

            virtual bool   GetLogReporter(const char* reporterName, ILogReporter** ppLogReporter, int reporterIndex = 0) const;
            virtual bool   SetOutputLevel(const char* reporterName, const char* pGroup, tLevel nLevel, int reporterIndex = 0);
            
            virtual bool   AddLogReporter(ILogReporter* pLogReporter, bool bAllowDuplicateName = false);
            virtual bool   RemoveLogReporter(ILogReporter* pLogReporter);
            virtual void   RemoveAllLogReporters();
            virtual size_t EnumerateLogReporters(ILogReporter* pLogReporterArray[], size_t nLogReporterArrayCount);
            
            virtual void   SetDefaultFilter(ILogFilter* filter);
            virtual void   SetDefaultFormatter(ILogFormatter* formatter);

        public:    // ITracer functionality
            virtual tAlertResult Trace(const TraceHelper& helper, const char* pText);
            virtual tAlertResult TraceV(const TraceHelper& helper, const char* pFormat, va_list arglist);
            virtual bool         IsFiltered(const TraceHelper& helper) const;

        public:    // IUnknown32 functionality
            virtual void* AsInterface(InterfaceId iid);
            virtual int   AddRef();
            virtual int   Release();

        protected:
            const char* GetLevelName(tLevel level) const;

        protected:
            Allocator::ICoreAllocator*             mpCoreAllocator;
            bool                                   mbIsReporting;          // True if currently in the process of executing a backout output. Used for re-entrancy protection.
            char*                                  mpHeapBuffer;           // Scratch heap for tracing overly large output. We could use an STL vector here, but our usage is very simple.
            int                                    mnHeapBufferSize;       // Size of above scratch heap.
            int                                    mLogRecordCounter;      // A counter for each log record created, can track re-ording issues this way (especially with deferred list)
            LogReporters                           mLogReporters;          // Array of ILogReporter*
            Trace::AutoRefCount<ILogFilter>        mDefaultFilter;         // Copied to reporters if they do not have a default filter.
            Trace::AutoRefCount<ILogFormatter>     mDefaultFormatter;      // Copied to reporters if they do not have a default formatter.
            Thread::AtomicInt32                    mnRefCount;             // Thread-safe reference count integer.
            mutable EA::Thread::Mutex              mMutex;                 // For general thread safety.
        };

        ITracer* CreateDefaultTracer(Allocator::ICoreAllocator* pAllocator);

    } // namespace Trace

} // namespace EA



#endif // EATRACE_EALOG_IMP_H
