/////////////////////////////////////////////////////////////////////////////
// EALog.h
//
// EA Log -- defines interfaces for an extendable system log service.
// This service is designed to mesh easily with the standard assert/trace
// system. Log is an extension of EA Trace system.
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
//
// Created by Paul Pedriana, Maxis
/////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_EALOG_H
#define EATRACE_EALOG_H


#ifndef INCLUDED_eabase_H
    #include <EABase/eabase.h>
#endif
#ifndef EATRACE_EATRACE_H
    #include <EATrace/EATrace.h>
#endif



/// namespace EA
/// The standard Electronic Arts namespace.
namespace EA
{
    namespace Trace
    {
        /// class LogRecord
        class LogRecord;

        /// class ILogFilter
        ///
        /// The emphasis of the filtering system is to avoid spending any time
        /// in logging code unless those results are reported. Default implementation 
        /// is solely group/level. Other possible filters include a filter at the server level
        /// for thread name, a filename filter (f.e. *EAStdC*), and a function name filter 
        /// (f.e. specify wildcard on  the namespace EA::IO::*). These are typically part of
        /// an uber-filter that any reporter can use and configure.
        ///
        class ILogFilter : public IUnknown32 
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x2e9e0f1a;

            /// First filter against the helper (and status is cached).
            /// Once one of the reporter filters passes, 
            /// the record is created and passed to each reporter for secondary filtering.
            virtual bool            IsFiltered(const TraceHelper& inHelper) const = 0; 
            virtual bool            IsFiltered(const LogRecord& inRecord) const = 0; 

            /// Human-readable, scripted name
            virtual void            SetName(const char* name) = 0;
            virtual const char*     GetName() const = 0;

            /// Caller must call SetName() after this with a unique name
            virtual  ILogFilter*    Clone() = 0;
        };


        /// class ILogFormatter
        ///
        /// The formatter interface converts a log record into textual data.  
        /// Variants of the formatter can be used for socket, dbase, xml, html, 
        /// and text output. Some reporters may only accept their own pre-defined formatter.
        ///
        class ILogFormatter : public IUnknown32 
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x0e9f1ff0;

            // This returns a reference to a single internal string buffer shared 
            // between all formatters to avoid allocations because of the locks, 
            // there is no contention between the formatters for the buffer.
            virtual const char*     FormatRecord(const LogRecord& inRecord) = 0;

            /// Human-readable, scripted name.
            virtual void            SetName(const char* name) = 0;
            virtual const char*     GetName() const = 0;

            /// Caller must call SetName() after this with a unique name.
            virtual ILogFormatter*  Clone() = 0;
        };

    
        /// class ILogReporter
        ///
        /// The reporter is a data synch through which to report log record data.
        /// Unless external access is necessary, a reporter doesn't normally need to be thread-safe, 
        /// as this is taken care of by the log server.
        ///
        class ILogReporter : public IUnknown32 
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x23ab223d;

            /// IsFiltered 
            virtual bool IsFiltered(const TraceHelper& helper) = 0;
            virtual bool IsFiltered(const LogRecord& record) = 0;

            /// Report
            virtual tAlertResult Report(const LogRecord& record) = 0;

            /// GetName
            /// Gets the name of the reporter, which is a human-readable name describing the reporter.
            /// This is not to be confused with group names, which refer to the various groups which 
            /// the reporter might recognize.
            virtual const char* GetName() const = 0;

            /// SetName
            /// Sets the name of the reporter, which is a human-readable name describing the reporter.
            virtual void SetName(const char* pName) = 0;

            /// IsEnabled
            /// Check if reporter is enabled/disabled.
            virtual bool IsEnabled() const = 0;
            
            /// SetEnabled
            /// Enable/disable the reporter.
            virtual void SetEnabled(bool isEnabled) = 0;
            
            /// SetFlushOnWrite
            /// If enabled then buffered write calls (e.g. fprintf) are followed up 
            /// with a io buffer flush call (e.g. fflush). This is useful for platforms 
            /// with remote debugging where a device crash would otherwise cause
            /// the loss of debug output that was in the device's output buffer 
            /// when the crash occurred.
            virtual void SetFlushOnWrite(bool /*flushOnWrite*/) { }     // Inlined instead of pure-virtual because this function was 
            virtual bool GetFlushOnWrite() const { return false; }      // added after this interface was written and subclassed by users.

            /// SetFilter
            /// Access to the filter of the reporter.
            virtual void         SetFilter(ILogFilter* filter) = 0;
            virtual ILogFilter*  GetFilter() = 0;

            /// SetFormatter
            /// Access to the formatter of the reporter.
            virtual void            SetFormatter(ILogFormatter* formatter) = 0;
            virtual ILogFormatter*  GetFormatter() = 0;
        };



        /// class IServer
        ///
        /// Defines an interface for a Log server. 
        /// The server is both thread-safe and re-entrant. 
        ///
        /// Concrete implementations of IServer should normally inherit 
        /// from Service::IService and EATrace::ITracer.
        ///     
        class IServer : public IUnknown32 // Intentionally do not inherit from ITracer. Use GetTracer instead.
        {
        public:
            /// Interface ID (32 bit) for IServer
            static const InterfaceId kIID = (InterfaceId)0x23ab34a1;

            /// GetTracer
            /// We don't inherit from ITracer, as doing so would make class construction 
            /// more complicated. This is a classic problem and is trivially solved by having a 
            /// member accessor for ITracer instead. Thus a concrete Server could inherit 
            /// from a concrete tracer and IServer and have a clean implementation.
            virtual ITracer* AsTracer() = 0;

            /// Init
            /// Initialize the log system.
            virtual void Init() = 0;

            /// UpdateLogFilters
            /// Update the filtering on existing helpers due to a configuration change.
            virtual void UpdateLogFilters() = 0;

            /// SetAllEnabled
            /// Enable/disable all log/assert statements. Useful for, e.g., resetting ignored asserts.
            virtual void SetAllEnabled(bool enabled) = 0;

            // Explict configuration of the logging system.  These commands should be internalized
            //  as a scripting system is put in place to go through Configure() above.
            
            /// AddLogReporter
            /// The reporter will be AddRefd.
            virtual bool AddLogReporter(ILogReporter* pLogReporter, bool bAllowDuplicateName = false) = 0;

            /// RemoveLogReporter
            /// The reporter will be Released if found.
            virtual bool RemoveLogReporter(ILogReporter* pLogReporter) = 0;

            /// RemoveAllLogReporters
            /// All reporters will be released.
            virtual void RemoveAllLogReporters() = 0;
          
            /// EnumerateLogReporters
            /// Returns an array of all reporter pointers. The returned pointers are AddRefd for the caller
            /// in order to allow the operation to be thread-safe. The return value is the number of 
            /// pointers copied to the output array. If the output array is NULL, the return value is
            /// the size of pointers required by the output array at the time of the call.
            virtual size_t EnumerateLogReporters(ILogReporter* pLogReporterArray[], size_t nLogReporterArrayCount) = 0;

            /// GetLogReporter
            /// Get reporter by its name.
            /// Returns an AddRef'd reporter.
            /// The index refers to the Nth log reporter by that name. If you have multiple log reporters 
            /// added with the same name, you could 
            virtual bool GetLogReporter(const char* logReporterName, ILogReporter** ppLogReporter, int reporterIndex = 0) const = 0;

            /// SetOutputLevel
            /// Pass in NULL logReporterName ptr to set for all reporters.
            /// Pass in NULL groupName to set for all groups.
            /// Pass in kLevelUndefined to remove a group/level.
            virtual bool SetOutputLevel(const char* logReporterName, const char* groupName, tLevel nLevel, int reporterIndex = 0) = 0;

            /// Set default filter, used for added log reporters that don't have their own.
            virtual void SetDefaultFilter(ILogFilter* filter) = 0;

            /// Set default formatter, used for added log reporters that don't have their own.
            virtual void SetDefaultFormatter(ILogFormatter* formatter) = 0;
        };


        /// GetServer
        /// This function returns the current server object to use for debug logging.
        /// By default this is simply an instance of class Server. But you can use the 
        /// SetServer function to override this with a subclass of Server.
        ///
        /// Note: This function is not internally thread-safe and if you are using GetServer 
        /// from multiple threads then you must call SetServer on application startup
        /// before other threads are created (preferred), or else put a mutex lock around
        /// all usage of GetServer, SetServer, GetTracer, SetTracer.
        IServer* GetServer();


        /// SetServer
        /// Sets the server object used by the system. You might want to override
        /// the default server with a version that takes advantage of your system.
        ///
        /// Note: This function is not thread-safe as described above for GetServer.
        void SetServer(IServer* pServer);

    } // namespace Trace

} // namespace EA


#endif // EATRACE_EALOG_H
