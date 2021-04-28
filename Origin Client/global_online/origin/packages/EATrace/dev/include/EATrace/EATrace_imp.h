/////////////////////////////////////////////////////////////////////////////
// EATrace_Imp.h
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
//
// This file is the default implemenation of a trace system and helpers.
// This avoid polluting the trace header with unnecessary dependencies, 
// since EATrace.h is such a widely used header.
//
// Created by Paul Pedriana, Maxis
/////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_EATRACE_IMP_H
#define EATRACE_EATRACE_IMP_H


#include <EATrace/internal/Config.h>
#ifndef INCLUDED_eabase_H
    #include <EABase/eabase.h>
#endif
#ifndef EASTDC_EASTRING_H
    #include <EAStdC/EAString.h>
#endif
#ifndef EATRACE_EATRACE_H
    #include <EATrace/EATrace.h>
#endif
#ifndef EATRACE_EATRACEZONEOBJECT_H
    #include <EATrace/EATraceZoneObject.h>
#endif
#ifndef EASTDC_EAHASHSTRING_H
    #include <EAStdC/EAHashString.h>
#endif
#ifndef EA_ALLOCATOR_ICOREALLOCATOR_INTERFACE_H
    #include <coreallocator/icoreallocator_interface.h>
#endif
#ifndef EASTL_CORE_ALLOCATOR_ADAPTER_H
    #include <EASTL/core_allocator_adapter.h>
#endif

#include <eathread/eathread_atomic.h>
#include <eathread/eathread_mutex.h>

#include <EASTL/fixed_string.h>
#include <EASTL/hash_map.h>
#include <EASTL/vector.h>
#include <EASTL/fixed_vector.h>
#include <stdarg.h>


namespace EA
{
    /// namespace Trace
    /// Classes and functions (but not macros) that are specific to the EA Trace library
    /// are confined to the Trace namespace.
    namespace Trace
    {
        /// DisplayTraceDialogFunc 
        ///
        /// Defines the type of function used to display a trace dialog box, as for
        /// an assertion failure for example. The pContextParameter is a user-defined
        /// value which can be used in any way useful to the user, such as storing
        /// the address of a C++ class object.
        ///
        typedef tAlertResult (*DisplayTraceDialogFunc)(const char* pTitle, const char* pText, void* pContext);

        /// SetDisplayTraceDialogFunc 
        ///
        /// Defines the default function to use for dialog-based display traces
        /// such as assertion failures. By default the DisplayTraceDialog function
        /// is used. 
        ///
        void SetDisplayTraceDialogFunc(DisplayTraceDialogFunc pDisplayTraceDialogFunc, void* pContext);

        /// GetDisplayTraceDialogFunc 
        ///
        /// Returns the current trace dialog function, which is the same as GetDefaultDisplayTraceDialogFunc
        /// unless the user set a custom one with SetDisplayTraceDialogFunc.
        ///
        DisplayTraceDialogFunc GetDisplayTraceDialogFunc(void*& pContext);

        /// GetDefaultDisplayTraceDialogFunc 
        ///
        /// Returns the internal default trace dialog function.
        ///
        DisplayTraceDialogFunc GetDefaultDisplayTraceDialogFunc(void*& pContext);


        /// DisplayTraceDialog
        ///
        /// This is the default function used to implement a display trace dialog;
        /// you can install an alternative via the SetDisplayTraceDialogFunc function.
        /// Displays a message box on the debugging host machine. This message box
        /// is an assertion failed-style message box, though for our uses we allow
        /// it to be used for things beyond assertion failures and thus don't call
        /// these dialog boxes "assertion" dialog boxes.
        ///
        /// If the display of trace dialogs are disabled (via SetTraceDialogEnabled), 
        /// this function always returns kAlertResultNone.
        /// 
        tAlertResult DisplayTraceDialog(const char* pTitle, const char* pText, void* pContext = NULL);

        /// GetTraceDialogEnabled
        ///
        /// Returns true if display of trace dialogs (e.g. via DisplayTraceDialog) 
        /// is enabled. 
        ///
        bool GetTraceDialogEnabled();

        /// SetTraceDialogEnabled
        /// 
        /// Enables or disables the display of trace dialogs. If dialogs are disabled, 
        /// then DisplayTraceDialog will always return kAlertResultNone. Alternative dialog 
        /// implementations should ideally use GetTraceDialogEnabled to tell if they 
        /// should display and act similarly.
        ///
        void SetTraceDialogEnabled(bool bEnabled);

        /// SetTraceDialogCallback
        ///
        /// Sets the callback function that will be invoked on the tracing thread
        /// while the trace dialog thread is awaiting user input. This function is 
        /// an 'idle' function that the user can use to do any routine application 
        /// processing.
        ///
        typedef void (*TraceDialogCallbackFunction)(void *pContext);
        void SetTraceDialogCallback(TraceDialogCallbackFunction pFunction, void *pContext = NULL);



        /// class ITracer
        class ITracer : public IUnknown32
        {
        public:
            /// IID_ITracer
            /// Interface ID (32 bit) for ITracer
            static const InterfaceId kIID = (InterfaceId)0x6c7ca8e4;

            /// Trace
            /// This is the core tracing routine. By default it writes lines to the stderr stream.
            virtual tAlertResult Trace(const TraceHelper& helper, const char* pText) = 0;

            /// TraceV
            /// By default, this function formats the string and calls Trace.
            virtual tAlertResult TraceV(const TraceHelper& helper, const char* pFormat, va_list arglist) = 0;
            #define TraceVaList TraceV // TraceVaList is deprecated.

            /// IsFiltered
            /// Returns true if all reporters have filtered this helper.
            virtual bool IsFiltered(const TraceHelper& helper) const = 0;
            
            /// SetFlushOnWrite
            /// If enabled then buffered write calls (e.g. fprintf) are followed up 
            /// with a io buffer flush call (e.g. fflush). This is useful for platforms 
            /// with remote debugging where a device crash would otherwise cause
            /// the loss of debug output that was in the device's output buffer 
            /// when the crash occurred.
            virtual void SetFlushOnWrite(bool /*flushOnWrite*/) { }     // Inlined instead of pure-virtual because this function was 
            virtual bool GetFlushOnWrite() const { return false; }      // added after this interface was written and subclassed by users.

        }; // class ITracer
      
        ITracer* CreateDefaultTracer(Allocator::ICoreAllocator* pAllocator);
    

        /// class ITraceHelperTable
        class ITraceHelperTable : public IUnknown32
        {
        public:
            static const InterfaceId kIID = (InterfaceId)0x6e8d0051;

            // Notify the helpers of a tracer change.
            virtual void UpdateTracer() = 0;
            virtual void UpdateTracer(ITracer* tracer) = 0;

            // Enable or disable all trace helpers. Useful to reset ignored asserts for instance.
            virtual void SetAllEnabled(bool enabled) = 0;

            // All helpers are registered into this table.
            virtual void RegisterHelper(TraceHelper& helper) = 0;
            virtual void UnregisterHelper(TraceHelper& helper) = 0;

            /// Configuration code can reserve a helper object before registration occurs.
            /// This is also used to support callback apis.  These create a heap-based TraceHelper.
            virtual TraceHelper* ReserveHelper(TraceType traceType, const char8_t* groupName, tLevel level, const ReportingLocation& reportingLocation) = 0;
        };

        ITraceHelperTable* CreateDefaultTraceHelperTable(Allocator::ICoreAllocator* pAllocator);

        
        /// class TraceHelperTable
        /// This table exists to handle tracer registrations.  Keep in mind that a tracer
        ///  may not be established before the helper is created, so this serves as the holder.
        class TraceHelperTable : public ITraceHelperTable, public Trace::ZoneObject
        { 
        public:
            TraceHelperTable(Allocator::ICoreAllocator* pAllocator = NULL);
            TraceHelperTable(const TraceHelperTable& x);

            virtual ~TraceHelperTable();

            TraceHelperTable& operator=(const TraceHelperTable& x) { mTraceHelperTable = x.mTraceHelperTable; return *this; }

            void SetAllocator(Allocator::ICoreAllocator* pAllocator);
            Allocator::ICoreAllocator* GetAllocator() const;

            virtual int   AddRef();
            virtual int   Release();
            virtual void* AsInterface(InterfaceId iid);

            // This is called by the tracer to update the tracer and filtering. 
            // Updates all existing tracers.
            virtual void UpdateTracer();
            virtual void UpdateTracer(ITracer* tracer);

            virtual void SetAllEnabled(bool enabled);

            // each individual helper registers with the table
            virtual void RegisterHelper(TraceHelper& helper);
            virtual void UnregisterHelper(TraceHelper& helper);

            /// Configuration code can reserve a helper object before registration occurs.
            /// This is also used to support callback apis.  These create a heap-based TraceHelper.
            virtual TraceHelper* ReserveHelper(TraceType traceType, const char8_t* groupName, tLevel level, const ReportingLocation& reportingLocation);
        
        protected:
            ITracer* mTracer;

            // table containing ptrs to trace helpers in exe and dll's
            typedef eastl::vector<TraceHelper*, Allocator::EASTLICoreAllocator> tTraceHelperTable;
            tTraceHelperTable mTraceHelperTable;

            EA::Thread::Mutex       mMutex;
            EA::Thread::AtomicInt32 mnRefCount; /// Thread-safe reference count integer.
        };



        /// class TempTraceHelperMap
        /// Some api's send all log/trace traffic through a callback.   The helper table may broadcast messages
        /// so that filtering can be performed once at registration, or on the helper table at log/trace reconfig.
        /// This map helps maintain the registered trace helpers, so that helper reservation happens once.
        /// The caller creates the map, and should destroy it before unloading a DLL.
        class TempTraceHelperMap
        {
        public:
            TempTraceHelperMap(Allocator::ICoreAllocator* pAllocator = NULL);
           ~TempTraceHelperMap();

            void SetAllocator(Allocator::ICoreAllocator* pAllocator);
            Allocator::ICoreAllocator* GetAllocator() const;

            /// Once release is called, the helper table is permanently disabled from further additions.
            void ReleaseHelpers();

            /// This reserves a helper, and returns it to the caller, to pass to the trace/log manager.
            TraceHelper* ReserveHelper(TraceType traceType, const char8_t* groupName, tLevel level, const ReportingLocation& reportingLocation);

        private:
            // buried to avoid having to transfer ptr ownership
            TempTraceHelperMap(const TempTraceHelperMap&) { }
            TempTraceHelperMap& operator=(const TempTraceHelperMap&) { return *this; } 

        protected:
            EA::Thread::Mutex mMutex;

            /// Code requires a valid filename (and line num) that are permanent enough to alias the file/function.
            struct ReportingLocationHash
            {
                // hash
                size_t operator()(const EA::Trace::ReportingLocation& key) const
                { return (size_t)(EA::StdC::FNV1_String8(key.GetFilename()) * key.GetLine()); }

                // comparator
                bool operator()(const EA::Trace::ReportingLocation& key1, const EA::Trace::ReportingLocation& key2) const
                { return (EA::StdC::Strcmp(key1.GetFilename(), key2.GetFilename()) == 0) && (key1.GetLine() == key2.GetLine()); }
            };

            typedef eastl::hash_map<ReportingLocation, TraceHelper*, ReportingLocationHash, ReportingLocationHash, Allocator::EASTLICoreAllocator> tFileToHelperMap;
            tFileToHelperMap mFileToHelperMap;
            bool             mIsEnabled;
        };


        /// class Tracer
        /// Default basic implementation of ITracer
        /// This implementation is not thread-safe with respect to reference counting.
        /// This implementation is not guaranteed to be thread-safe with respect to tracing.
        class Tracer : public ITracer, public Trace::ZoneObject
        {
        public:
            /// IID_Tracer
            /// Interface ID (32 bit) for Tracer
            static const InterfaceId kIID = (InterfaceId)0x0c7ca925;

            // sizeo of scratch buffer used for Vsnprintf calls
            enum { kTraceBufferSize = 2048 };

            Tracer();
            Tracer(const Tracer& x);
            virtual ~Tracer();

            Tracer& operator=(const Tracer& x);

            virtual int          AddRef();
            virtual int          Release();
            virtual void *       AsInterface(InterfaceId iid);
            virtual void         FinalRelease();
            
            virtual tAlertResult Trace(const TraceHelper& helper, const char* pText);
            virtual tAlertResult TraceV(const TraceHelper& helper, const char* pFormat, va_list argList);
            virtual bool         IsFiltered(const TraceHelper& helper) const;
            virtual void         SetFlushOnWrite(bool flushOnWrite);
            virtual bool         GetFlushOnWrite() const;

        protected:
            char                    mTraceBuffer[kTraceBufferSize]; /// Used by Vsnprintf
            bool                    mbIsReporting;
            bool                    mbFlushOnWrite;
            EA::Thread::AtomicInt32 mnRefCount; /// Thread-safe reference count integer.
            EA::Thread::Mutex       mMutex;
        }; // class Tracer



        /// GetTracer
        /// This function returns the current tracer object to use for debug tracing.
        /// By default this is simply an instance of class Tracer. But you can use the 
        /// SetTracer function to override this with a subclass of Tracer.
        ///
        /// Note: This function is not internally thread-safe and if you are using GetTracer 
        /// from multiple threads then you must call SetServer on application startup
        /// before other threads are created (preferred), or else put a mutex lock around
        /// all usage of GetServer, SetServer, GetTracer, SetTracer.
        ITracer* GetTracer();

        /// SetTracer
        /// Sets the tracer object used by the system. You would normally want to override
        /// the default tracer with a version that takes advantage of your system. 
        /// 
        /// This function must be called on startup before any tracing occurs, otherwise
        /// there will be no output.
        ///
        /// Note: This function is not thread-safe as described above for GetTracer.
        ///
        /// Example usage:
        ///     #include <EATrace/EATrace.h>
        ///     
        ///     int main(int, char**){
        ///         EA::Trace::Tracer tracer;       // Just use the built-in default version,.
        ///         EA::Trace::SetTracer(&tracer);
        ///     
        ///         EA_TRACE( "Hello world");
        ///         . . .
        ///     }
        void SetTracer(ITracer* pTracer);




        /// GetTraceHelperTable
        /// Return the table containing registered tracer helpers.
        /// This can be used to notify the helper table about changes in 
        ///  the filtering or tracer.
        ITraceHelperTable* GetTraceHelperTable();

        /// SetTraceHelperTable
        /// Update the default trace helper table.  This default table is 
        ///  typically relied upon, but this call is here for completeness.
        void SetTraceHelperTable(ITraceHelperTable* table);

    } // namespace Trace

} // namespace EA


#endif // EATRACE_EATRACE_IMP_H
