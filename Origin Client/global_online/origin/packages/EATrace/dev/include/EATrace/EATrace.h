/////////////////////////////////////////////////////////////////////////////
// EATrace.h
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
//
// This file implements basic debug tracing macros such as TRACE, ASSERT,
// and others that provided extended functionality. This file does not
// define a generic syslog-style logging interface; that's a task for a
// separate module. However, the system provided here maps to a syslog
// interface.
//
// Primary defines provided by this module include:
//    EA_TRACE*
//    EA_ASSERT*
//    EA_VERIFY*
//    EA_FAIL*
//    EA_LOG*
//
// The names selected were chosen because they are in line with what is
// most often used in the PC programming (particularly Microsoft) world.
// The Linux/GCC world has implemented similar facilities but hasn't
// really seemed to find a consistent naming scheme. Thus we stick with
// the Microsoft/PC-like conventions.
//
// Created by Paul Pedriana, Maxis
/////////////////////////////////////////////////////////////////////////////


#ifndef EATRACE_EATRACE_H
#define EATRACE_EATRACE_H


#include <EATrace/internal/Config.h>
#include <EATrace/EATraceBase.h>
#include <stdarg.h>

// If we are to use the EAAssert package for base assertion functionality...
#if UTF_USE_EAASSERT
    #include <EAAssert/eaassert.h>
#endif


#if defined(__SNC__)
    #pragma control %push diag
    #pragma diag_suppress=1787 // 2929504506 is not a valid value for the enumeration type "EA::COM::InterfaceId"
#endif


//------------------------------------------------------------------------------------------------------------
// Trace-based output
//
// Descriptions:
//    TRACE is your basic method of debugging output. It is a simple log write that completely goes away in
//    a release build.
//
//    WARN is the same as TRACE but only traces if an condition evaluates to true. It completely goes away
//    in a release build.
//
//    NOTIFY is the same as WARN but the condition expression is evaluated in a release build.
//
//
//------------------------------------------------------------------------------------------------------------------------------------------------
//| Macro                        | Debug build behaviour              | Release build behaviour               | Example usage
//|                              | Each line below applies to all.    | Each line below applies to all.       |
//|-----------------------------------------------------------------------------------------------------------------------------------------------
//| EA_TRACE                     | Always prints output.              | No output.                            | EA_TRACE("Hello world.");
//| EA_TRACE_FORMATTED           | No alert dialog.                   | No alert dialog.                      | EA_TRACE_FORMATTED(("Hello %s.", "world"));
//|                              | Output arguments evaluated.        | No output argument evaluation.        |
//|-----------------------------------------------------------------------------------------------------------------------------------------------
//| EA_WARN                      | May print output.                  | No output.                            | EA_WARN(result, "Hello world.");
//| EA_WARN_FORMATTED            | No alert dialog.                   | No alert dialog.                      | EA_WARN_FORMATTED(result, ("Hello %s.", "world"));
//|                              | Condition  evaluated.              | No condition evaluation.              |
//|                              | May evaluate output arguments.     | No output argument evaluation.        |
//|-----------------------------------------------------------------------------------------------------------------------------------------------
//| EA_NOTIFY                    | May print output.                  | No output.                            | EA_NOTIFY(result, "Hello world.");
//| EA_NOTIFY_FORMATTED          | No alert dialog.                   | No alert dialog.                      | EA_NOTIFY_FORMATTED(result, ("Hello %s.", "world"));
//|                              | Condition evaluated.               | Condition evaluated.                  |
//|                              | May evaluate output arguments.     | No output argument evaluation.        |
//------------------------------------------------------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------------------
// Alert-based output
//
// Descriptions:
//    ASSERT is your basic method of testing a condition and putting up an alert if the condition is false.
//    If the condition is false then a message is traced and then an alert is displayed to the user giving
//    the user an option to stop execution or continue. The ASSERT completely goes away in a release build.
//
//    VERIFY is the same as ASSERT but the condition expression is evaluated in a release build.
//
//    FAIL is the same as ASSERT except there is no condition to test; the condition is always true.
//    Thus, FAIL_MESSAGE("error") is the same as ASSERT_MESSAGE(false, "error")
//
//------------------------------------------------------------------------------------------------------------------------------------------------
//| Macro                        | Debug build behaviour              | Release build behaviour               | Example usage
//|                              | Each line below applies to all.    | Each line below applies to all.       |
//|-----------------------------------------------------------------------------------------------------------------------------------------------
//| EA_ASSERT                    | May print output.                  | No output.                            | EA_ASSERT(result);
//| EA_ASSERT_MESSAGE            | May display alert dialog.          | No alert dialog.                      | EA_ASSERT_MESSAGE(result, "Hello world");
//| EA_ASSERT_FORMATTED          | Condition evaluated.               | No condition evaluation.              | EA_ASSERT_FORMATTED(result, ("Hello %s.", "world"));
//|                              | May evaluate output arguments.     | No output argument evaluation.        |
//|-----------------------------------------------------------------------------------------------------------------------------------------------
//| EA_VERIFY                    | May print output.                  | No output.                            | EA_VERIFY(result);
//| EA_VERIFY_MESSAGE            | May display alert dialog.          | No alert dialog.                      | EA_VERIFY_MESSAGE(result, "Hello world");
//| EA_VERIFY_FORMATTED          | Condition evaluated.               | Condition evaluated.                  | EA_VERIFY_FORMATTED(result, ("Hello %s.", "world"));
//|                              | May evaluate output arguments.     | No output argument evaluation.        |
//|-----------------------------------------------------------------------------------------------------------------------------------------------
//| EA_FAIL_MESSAGE              | May display alert dialog.          | No alert dialog.                      | EA_FAIL_MESSAGE("Hello world");
//| EA_FAIL_FORMATTED            | May evaluate output arguments.     | No output argument evaluation.        | EA_FAIL_FORMATTED(("Hello %s.", "world"));
//------------------------------------------------------------------------------------------------------------------------------------------------


/// namespace EA
/// Standard Electronic Arts Framework namespace.
namespace EA
{
    /// namespace Trace
    /// Classes and functions (but not macros) that are specific to the EA Trace library
    /// are confined to the Trace namespace.
    namespace Trace
    {
        // Forward declarations
        class ITracer;

        /// Shutsdown EATrace and frees any resources that are allocated.
        void Shutdown();

        /// enum Level
        /// Level refers to the severity level of the current tracing statement.
        enum Level
        {
            kLevelUndefined =   0,   /// Level unspecified, typically overridden by the trace type.
            kLevelAll       =   1,   /// Report all messages.
            kLevelMin       =   2,   /// Minimum possible level; least serious.
            kLevelPrivate   =  10,   /// Refers to private (local-author-only) debug level, expected to be off by default.
            kLevelDebug     =  25,   /// Refers to your standard debug level.
            kLevelInfo      =  50,   /// Refers to information output only.
            kLevelWarn      = 100,   /// Refers to something that is amiss but may not be serious.
            kLevelError     = 150,   /// Refers to an error which much be resolved.
            kLevelFatal     = 200,   /// Refers to a serious error, such as a crash.
            kLevelMax       = 250,   /// Maximum possible level; most serious.
            kLevelNone      = 251,   /// Report no messages.
        };

        typedef int tLevel; /// tLevel can be any level between min/max or undefined, but many tracers/loggers may only support discrete levels


        /// enum OutputType
        /// OutputType defines what output a statement performs.
        enum OutputType
        {
            kOutputTypeNone   = 0x00,
            kOutputTypeText   = 0x01, /// Display text.
            kOutputTypeAlert  = 0x02  /// Display an interactive dialog.
        };
        typedef int tOutputType; // mask of OutputType


        /// enum AlertResult
        /// A trace statement that has an OutputType value of kOutputTypeAlert will
        /// return one or more of these flags, which indicate what the result of the alert
        /// notification was. Often the alert will be an assertion failure dialog box which
        /// gives the user an option to break, continue, or ignore. These result flags tell
        /// you what the user's choice was.
        enum AlertResult
        {
            kAlertResultNone    = 0x00,
            kAlertResultBreak   = 0x01,  /// Stop execution.
            kAlertResultDisable = 0x02,  /// Disable the statement in the future.
            kAlertResultAbort   = 0x04   /// Unilaterally exit the application. Some platforms effectively require this, since they don't have the ability to pause.
        };

        typedef int tAlertResult; /// tAlertResult is a mask of AlertResult


        /// enum TraceType
        /// Defines the type of message generated.  Filtering may act on this info.
        enum TraceType
        {
            kTraceTypeAssert,
            kTraceTypeVerify,
            kTraceTypeTrace,
            kTraceTypeFail,
            kTraceTypeLog
        };


        /// class ReportingLocation
        ///
        /// This transports all the information about a reporting location.
        ///
        class ReportingLocation
        {
        public:
            ReportingLocation()
                : mFile(""), mLine(0), mFunction("") { }

            ReportingLocation(const char8_t* file, int line, const char8_t* func)
                : mFile(file), mLine(line), mFunction(func) { }

            ReportingLocation& operator=(const ReportingLocation& x) { mFile = x.mFile; mLine = x.mLine; mFunction = x.mFunction; return *this; }

            const char8_t* GetFilename() const { return mFile; }
            int            GetLine()     const { return mLine; }
            const char8_t* GetFunction() const { return mFunction; }

        protected:
            const char8_t* mFile;
            int            mLine;
            const char8_t* mFunction;
        };

        // For now reporting location is a raw string containing file(line): function
        // #define EA_TRACE_REPORTING_LOCATION EA_PREPROCESSOR_LOCATION_FUNC
        // This does not work with edit and continue in 7.1, so the class is used instead.
        // Disabled when tracing and logging are disabled, so file information is not available.
        #if EA_TRACE_ENABLED || EA_LOG_ENABLED
            #define EA_TRACE_REPORTING_LOCATION \
                EA::Trace::ReportingLocation(__FILE__, __LINE__, EA_CURRENT_FUNCTION)
        #else
            #define EA_TRACE_REPORTING_LOCATION \
                EA::Trace::ReportingLocation()
        #endif



        /// class TraceHelper
        ///
        /// A default implemation of the trace helper interface.
        ///
        class TraceHelper
        {
        public:
            TraceHelper(TraceType traceType, const char8_t* groupName, tLevel level, const ReportingLocation& reportingLocation);
            virtual ~TraceHelper();

            TraceHelper& operator=(const TraceHelper&) { return *this; } // By design we don't support copying TraceHelpers. They are always temporary objects.

            /// This is only to be used for setting dynamic group name and/or level
            /// filtering is re-evaluated (or should check just be disabled).
            /// the group name will be aliased so make sure it lives as long as the helper.
            void SetGroupNameAndLevel(const char8_t* groupName, tLevel level);

            bool IsTracing();

            tAlertResult Trace(const char8_t* text);
            tAlertResult TraceFormatted(const char8_t* format, ...);
            tAlertResult TraceFormattedV(const char8_t* pFormat, va_list argList);

        public: // TraceHelper
            virtual void UpdateTracer(ITracer* tracer);

            virtual TraceType                GetTraceType() const         { return mTraceType; }
            virtual tOutputType              GetOutputType() const        { return mOutputType; }
            virtual tLevel                   GetLevel() const             { return mLevel; }
            virtual const char8_t*           GetGroupName() const         { return mGroupName; }
            virtual const ReportingLocation* GetReportingLocation() const { return &mReportingLocation; }
            virtual void                     SetEnabled(bool bEnabled)    { mIsEnabled = bEnabled; }

            // Global runtime switch for entire trace/log system
            static bool                      GetTracingEnabled();               // The implementations of these two functions are intentionally not inline, 
            static void                      SetTracingEnabled(bool bEnabled);  // for reasons explained at their definition in their .cpp file.

        protected:
            // 16-bytes (assuming enum/bool packing), plus string data for group name and preprocessor location

            // Track state about the helper itself and its behavior
            bool                       mIsEnabled;  // this is controlled by user input - could also just set tracer to NULL
            bool                       mIsFiltered; // whether an active back end is reporting this message, also false if no tracer
            bool                       mIsFilterEvaluated; // mark a helper needs to re-eval filtering
            TraceType                  mTraceType;
            tOutputType                mOutputType;

            // Filtering info; these are re-examined during reconfig as well
            tLevel                     mLevel;
            const char8_t*             mGroupName; // alias for now, TODO: could copy into fixed sized string
            ReportingLocation          mReportingLocation; // no way to pool this, so this should be unique address

            // Track info about the tracer
            ITracer*                   mTracer;

        public: // for fast macro access only
            static bool                sTracingEnabled;
        }; // class TraceHelper


        ///////////////////////////////////////////////////////////////////////
        // EACOM-like functionality
        ///////////////////////////////////////////////////////////////////////

        enum InterfaceId
        {
            kIIDMin     = 0,           /// Defined as 0 just as a placeholder.
            kIIDFactory = 1,           /// This is a special InterfaceId that means "return a pointer to your factory".
            kIIDMax     = 0xffffffff   /// Defined as 0xffffffff to ensure the compiler reserves at least 32 bits of space.
        };
        typedef uint32_t InterfaceIdInt;   /// This identifies the type of built-in integer that corresponds to InterfaceId. It is useful for portably casting interface ids back and forth between integers. Consider for example that some day a interface id might be a different size than now.

        class IRefCount
        {
        public:
            static const InterfaceId kIID = InterfaceId(0xae9cb0fa);

            virtual int AddRef() = 0;
            virtual int Release() = 0;

            virtual ~IRefCount() { }
        };

        class IUnknown32 : public IRefCount
        {
        public:
            static const InterfaceId kIID = InterfaceId(0xee3f516e);

            virtual void* AsInterface(InterfaceId iid) = 0;
        };


    } // namespace Trace

} // namespace EA




/// Note that all formatted macros with args before the formatting string require
///   pFormatAndArguments to be surrounded by an extra set of ().
///
/// EA_ASSERT_FORMATTED(false, ("%d", intValue));
/// EA_TRACE_FORMATTED("%d", intValue);
///
/// Be careful about use of the format string, passing EA_ASSERT_FORMATTED(false, ("Value at 50%")) will throw off the varargs processor.
///  Safer to use EA_ASSERT_FORMATTED(false, ("%s", "Value at 50%")), but switch to the EA_ASSERT_MESSAGE for single strings.
///
/// Do not call the helper macros directly.  These are used the implement the trace macros
///  and ensure the correct code location and debug break occurs.  They are also stripped
///  out of the code in release builds.
///
#if defined(__GNUC__) && (__GNUC__ >= 3)
    #define EvaluateTraceExpression(expression) __builtin_expect(expression, 1)
#else
    // EvaluateTraceExpression avoids the "warning C4127: conditional expression is constant" from VC++
    #define EvaluateTraceExpression(expression) ((expression) && EATRACE_TRUE)
#endif


// If using an earlier version of EABase then we need to define EA_ANALYSIS_ASSUME ourselves.
#if !defined(EA_ANALYSIS_ASSUME)
    #if defined(_MSC_VER) && (_MSC_VER >= 1300) && !defined(EA_PLATFORM_AIRPLAY)
        #define EA_ANALYSIS_ASSUME(x) __analysis_assume(x)
    #else
        #define EA_ANALYSIS_ASSUME(x)
    #endif
#endif


// ------------------------------------------------------------------------
// EA_DISABLE_VC_WARNING / EA_RESTORE_VC_WARNING
// 
// Disable and re-enable warning(s) within code.
// This is simply a wrapper for VC++ #pragma warning(disable: nnnn) for the
// purpose of making code easier to read due to avoiding nested compiler ifdefs
// directly in code.
//
// Example usage:
//     EA_DISABLE_VC_WARNING(4127 3244)
//     <code>
//     EA_RESTORE_VC_WARNING()
//
#ifndef EA_DISABLE_VC_WARNING
    #if defined(_MSC_VER)
        #define EA_DISABLE_VC_WARNING(w)  \
            __pragma(warning(push))       \
            __pragma(warning(disable:w))
    #else
        #define EA_DISABLE_VC_WARNING(w)
    #endif
#endif

#ifndef EA_RESTORE_VC_WARNING
    #if defined(_MSC_VER)
        #define EA_RESTORE_VC_WARNING()   \
            __pragma(warning(pop))
    #else
        #define EA_RESTORE_VC_WARNING()
    #endif
#endif


#define EA_TRACE_HELPER_MESSAGE_GL(traceType, expression, group, level, message) \
    EA_DISABLE_VC_WARNING(4127) \
    do{ \
        EA_ANALYSIS_ASSUME(expression); \
        if (EvaluateTraceExpression(!(expression)) && EA::Trace::TraceHelper::GetTracingEnabled())  \
        { \
            static EA::Trace::TraceHelper  sTraceHelper(traceType, group, level, EA_TRACE_REPORTING_LOCATION); \
            if (sTraceHelper.IsTracing() && ((sTraceHelper.Trace(message) & EA::Trace::kAlertResultBreak) != 0)) \
                { EA_DEBUG_BREAK(); } \
        } \
    } while(0) \
    EA_RESTORE_VC_WARNING()

#define EA_TRACE_HELPER_FORMATTED_GL(traceType, expression, group, level, pFormatAndArguments) \
    EA_DISABLE_VC_WARNING(4127) \
    do{ \
        EA_ANALYSIS_ASSUME(expression); \
        if (EvaluateTraceExpression(!(expression)) && EA::Trace::TraceHelper::GetTracingEnabled())  \
        { \
            static EA::Trace::TraceHelper  sTraceHelper(traceType, group, level, EA_TRACE_REPORTING_LOCATION); \
            if (sTraceHelper.IsTracing() && ((sTraceHelper.TraceFormatted pFormatAndArguments & EA::Trace::kAlertResultBreak) != 0)) \
                { EA_DEBUG_BREAK(); } \
        } \
    } while(0) \
    EA_RESTORE_VC_WARNING()

#define EA_TRACE_HELPER(traceType, expression) \
    EA_TRACE_HELPER_MESSAGE_GL(traceType, expression, NULL, EA::Trace::kLevelUndefined, #expression "\n")

#define EA_TRACE_HELPER_MESSAGE(traceType, expression, message) \
    EA_TRACE_HELPER_MESSAGE_GL(traceType, expression, NULL, EA::Trace::kLevelUndefined, message)

#define EA_TRACE_HELPER_FORMATTED(traceType, expression, pFormatAndArguments) \
    EA_TRACE_HELPER_FORMATTED_GL(traceType, expression, NULL, EA::Trace::kLevelUndefined, pFormatAndArguments)



//------------------------------------
#if EA_TRACE_ENABLED

    #ifndef EA_ASSERT_BREAK
        #define EA_ASSERT_BREAK(expression) (EvaluateTraceExpression(!(expression)) ? EA_DEBUG_BREAK() : 0)
    #endif

    #ifndef EA_ASSERT
        #define EA_ASSERT(expression) EA_TRACE_HELPER(EA::Trace::kTraceTypeAssert, expression)
    #endif

    #ifndef EA_ASSERT_MESSAGE
        #define EA_ASSERT_MESSAGE(expression, message) EA_TRACE_HELPER_MESSAGE(EA::Trace::kTraceTypeAssert, expression, message)
    #endif

    #ifndef EA_ASSERT_FORMATTED
        #define EA_ASSERT_FORMATTED(expression, pFormatAndArguments) EA_TRACE_HELPER_FORMATTED(EA::Trace::kTraceTypeAssert, expression, pFormatAndArguments)
    #endif

    #ifndef EA_VERIFY
        #define EA_VERIFY(expression) EA_TRACE_HELPER(EA::Trace::kTraceTypeVerify, expression)
    #endif

    #ifndef EA_VERIFY_MESSAGE
        #define EA_VERIFY_MESSAGE(expression, message) EA_TRACE_HELPER_MESSAGE(EA::Trace::kTraceTypeVerify, expression, message)
    #endif

    #ifndef EA_VERIFY_FORMATTED
        #define EA_VERIFY_FORMATTED(expression, pFormatAndArguments) EA_TRACE_HELPER_FORMATTED(EA::Trace::kTraceTypeVerify, expression, pFormatAndArguments)
    #endif

    #ifndef EA_TRACE
        #define EA_TRACE EA_TRACE_MESSAGE
    #endif

    #ifndef EA_TRACE_MESSAGE
        #define EA_TRACE_MESSAGE(message) EA_TRACE_HELPER_MESSAGE(EA::Trace::kTraceTypeTrace, EATRACE_FALSE, message)
    #endif

    #ifndef EA_TRACE_FORMATTED
        #define EA_TRACE_FORMATTED(pFormatAndArguments) EA_TRACE_HELPER_FORMATTED(EA::Trace::kTraceTypeTrace, EATRACE_FALSE, pFormatAndArguments)
    #endif

    #ifndef EA_FAIL_MESSAGE
        #define EA_FAIL_MESSAGE(message) EA_TRACE_HELPER_MESSAGE(EA::Trace::kTraceTypeFail, EATRACE_FALSE, message)
    #endif

    #ifndef EA_FAIL_FORMATTED
        #define EA_FAIL_FORMATTED(pFormatAndArguments) EA_TRACE_HELPER_FORMATTED(EA::Trace::kTraceTypeFail, EATRACE_FALSE, pFormatAndArguments)
    #endif

#else

    #ifndef EA_ASSERT_BREAK
        #define EA_ASSERT_BREAK(expression)
    #endif

    #ifndef EA_ASSERT
        #define EA_ASSERT(expression)
    #endif

    #ifndef EA_ASSERT_MESSAGE
        #define EA_ASSERT_MESSAGE(expression, message)
    #endif

    #ifndef EA_ASSERT_FORMATTED
        #define EA_ASSERT_FORMATTED(expression, pFormatAndArguments)
    #endif

    #ifndef EA_VERIFY
        #define EA_VERIFY(expression) (expression)
    #endif

    #ifndef EA_VERIFY_MESSAGE
        #define EA_VERIFY_MESSAGE(expression, message) (expression)
    #endif

    #ifndef EA_VERIFY_FORMATTED
        #define EA_VERIFY_FORMATTED(expression, pFormatAndArguments) (expression)
    #endif

    #ifndef EA_TRACE
        #define EA_TRACE EA_TRACE_MESSAGE
    #endif

    #ifndef EA_TRACE_MESSAGE
        #define EA_TRACE_MESSAGE(message)
    #endif

    #ifndef EA_TRACE_FORMATTED
        #define EA_TRACE_FORMATTED(pFormatAndArguments)
    #endif

    #ifndef EA_FAIL_MESSAGE
        #define EA_FAIL_MESSAGE(message)
    #endif

    #ifndef EA_FAIL_FORMATTED
        #define EA_FAIL_FORMATTED(pFormatAndArguments)
    #endif

#endif // EA_TRACE_ENABLED



/////////////////////////////////////////////////////////////////////////
// Shortcuts
/////////////////////////////////////////////////////////////////////////

#ifndef EA_ASSERT_M
    #define EA_ASSERT_M     EA_ASSERT_MESSAGE
#endif

#ifndef EA_ASSERT_MSG
    #define EA_ASSERT_MSG   EA_ASSERT_MESSAGE
#endif

#ifndef EA_ASSERT_F
    #define EA_ASSERT_F     EA_ASSERT_FORMATTED
#endif

#ifndef EA_VERIFY_M
    #define EA_VERIFY_M     EA_VERIFY_MESSAGE
#endif

#ifndef EA_VERIFY_F
    #define EA_VERIFY_F     EA_VERIFY_FORMATTED
#endif

#ifndef EA_TRACE_M
    #define EA_TRACE_M      EA_TRACE_MESSAGE
#endif

#ifndef EA_TRACE_F
    #define EA_TRACE_F      EA_TRACE_FORMATTED
#endif

#ifndef EA_FAIL
    #define EA_FAIL()       EA_FAIL_MESSAGE("Unspecified error")
#endif

#ifndef EA_FAIL_M
    #define EA_FAIL_M       EA_FAIL_MESSAGE
#endif

#ifndef EA_FAIL_MSG
    #define EA_FAIL_MSG     EA_FAIL_MESSAGE
#endif

#ifndef EA_FAIL_F
    #define EA_FAIL_F       EA_FAIL_FORMATTED
#endif



/////////////////////////////////////////////////////////////////////////
// Logging
//
// Example usage:
//     EA_LOG("Sim", Trace::kLevelError, ("Sim found %d mistakes", mistakeCount));
//     EA_LOGE("Sim", ("Sim found %d mistakes", mistakeCount));
//     EA_LOGI("Sim", ("Sim finished search of %s", "toaster"));
//
// A limitation of the log macro system is that the group (a.k.a. channel)
// and level must be constant. The values passed to an individual EA_LOG
// cannot change at runtime. The result of having variable values is that
// all log calls will work as if they were using the first-seen group and
// level values. This limitation is present because it allows the log
// system to be very fast at filtering. See the HTML documentation for
// resolutions to this problem.
//
// Neither of the following will work as expected:
//     EA_LOG(stringArray[index], Trace::kLevelError, ("Sim found %d mistakes", mistakeCount));
//     EA_LOG("Sim",              levelArray[index],  ("Sim found %d mistakes", mistakeCount));
//
/////////////////////////////////////////////////////////////////////////

#if EA_LOG_ENABLED
    #define EA_LOG(group, level, pFormatAndArguments)           EA_TRACE_HELPER_FORMATTED_GL(EA::Trace::kTraceTypeLog, EATRACE_FALSE, group, level,                  pFormatAndArguments)
    #define EA_LOGE(group, pFormatAndArguments)                 EA_TRACE_HELPER_FORMATTED_GL(EA::Trace::kTraceTypeLog, EATRACE_FALSE, group, EA::Trace::kLevelError, pFormatAndArguments)
    #define EA_LOGW(group, pFormatAndArguments)                 EA_TRACE_HELPER_FORMATTED_GL(EA::Trace::kTraceTypeLog, EATRACE_FALSE, group, EA::Trace::kLevelWarn,  pFormatAndArguments)
    #define EA_LOGI(group, pFormatAndArguments)                 EA_TRACE_HELPER_FORMATTED_GL(EA::Trace::kTraceTypeLog, EATRACE_FALSE, group, EA::Trace::kLevelInfo,  pFormatAndArguments)
    #define EA_LOGD(group, pFormatAndArguments)                 EA_TRACE_HELPER_FORMATTED_GL(EA::Trace::kTraceTypeLog, EATRACE_FALSE, group, EA::Trace::kLevelDebug, pFormatAndArguments)
    #define EA_LOGP(group, pFormatAndArguments)                 EA_TRACE_HELPER_FORMATTED_GL(EA::Trace::kTraceTypeLog, EATRACE_FALSE, group, EA::Trace::kLevelPrivate, pFormatAndArguments)

    // EATrace compatibility:
    #define EA_LOG_MESSAGE(group, level, message)               EA_TRACE_HELPER_MESSAGE_GL  (EA::Trace::kTraceTypeLog, EATRACE_FALSE, group, level, message)
    #define EA_LOG_FORMATTED(group, level, pFormatAndArguments) EA_TRACE_HELPER_FORMATTED_GL(EA::Trace::kTraceTypeLog, EATRACE_FALSE, group, level, pFormatAndArguments)
#else
    #define EA_LOG(group, level, pFormatAndArguments)
    #define EA_LOGE(group, pFormatAndArguments)
    #define EA_LOGW(group, pFormatAndArguments)
    #define EA_LOGI(group, pFormatAndArguments)
    #define EA_LOGD(group, pFormatAndArguments)
    #define EA_LOGP(group, pFormatAndArguments)

    // EATrace compatibility:
    #define EA_LOG_MESSAGE(group, level, pMessage)
    #define EA_LOG_FORMATTED(group, level, pFormatAndArguments)
#endif

// Abbreviations
#define EA_LOG_M EA_LOG_MESSAGE
#define EA_LOG_F EA_LOG_FORMATTED


#if defined(__SNC__)
    #pragma control %pop diag
#endif


#endif // Header include guard...





