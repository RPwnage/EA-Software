//*! *************************************************************************************************/
/*!
    \file debug.h


    \attention
       (c) Electronic Arts. All Rights Reserved.
***************************************************************************************************/

#ifndef BLAZE_DEBUG_H
#define BLAZE_DEBUG_H
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
#pragma once
#endif

#ifndef ENABLE_BLAZE_SDK_LOGGING
#ifdef EA_DEBUG
#define ENABLE_BLAZE_SDK_LOGGING (1)
#else
#define ENABLE_BLAZE_SDK_LOGGING (0)
#endif
#endif

#if ENABLE_BLAZE_SDK_LOGGING
#define BLAZE_SDK_DEBUG(...) Blaze::Debug::Log(false, __VA_ARGS__);
#define BLAZE_SDK_DEBUGF(...) Blaze::Debug::Logf(false, __VA_ARGS__);
// If possible, prefer BLAZE_SDK_DEBUGF, since it will only perform string operations if a loging function is in use.
#define BLAZE_SDK_DEBUGSB(sb) Blaze::Debug::LogSb(false, Blaze::StringBuilder() << sb);
#else
#define BLAZE_SDK_DEBUG(...)
#define BLAZE_SDK_DEBUGF(...)
#define BLAZE_SDK_DEBUGSB(sb)
#endif

/// Concatenates 2 macro arguments even if on of the arguments itself is a macro
#define BLAZE_CONCAT(X, Y)       BLAZE_CONCAT_IMPL1(X, Y)
/// @internal Used by BLAZE_CONCAT macro
#define BLAZE_CONCAT_IMPL1(X, Y) BLAZE_CONCAT_IMPL2(X,Y)
/// @internal Used by BLAZE_CONCAT macro
#define BLAZE_CONCAT_IMPL2(X, Y) X##Y

#ifndef ENABLE_BLAZE_SDK_PROFILING
#ifdef EA_DEBUG
#define ENABLE_BLAZE_SDK_PROFILING (1)
#else
#define ENABLE_BLAZE_SDK_PROFILING (0)
#endif
#endif

#if ENABLE_BLAZE_SDK_PROFILING
    #define BLAZE_SDK_SCOPE_TIMER(name) ScopeTimer BLAZE_CONCAT(scopeTimer, __LINE__ )(name)
#else
    #define BLAZE_SDK_SCOPE_TIMER(name)
#endif


#include "BlazeSDK/blazesdk.h"
#include "framework/util/shared/stringbuilder.h"

namespace Blaze
{

#if ENABLE_BLAZE_SDK_PROFILING
class BLAZESDK_API ScopeTimer
{
public:
    ScopeTimer(const char* name);
    ~ScopeTimer();
};
#endif

/*! ***************************************************************************/
/*!     
    \brief Provides logging and assert/verify support for Blaze SDK classes.
*******************************************************************************/
class BLAZESDK_API Debug
{
public:
    typedef void(*PerfStartFunc)(const char* name);
    typedef void(*PerfStopFunc)();

    /*! ***************************************************************************/
    /*!
        \brief Sets the functions to use for tracking performance.
        \param startHook    The function to use to start a timer.
        \param stopHook     The function to use to stop the innermost timer.
    *****************************************************************************/
    static void setPerfHooks(PerfStartFunc startHook, PerfStopFunc stopHook);

#if ENABLE_BLAZE_SDK_PROFILING
    /*! ***************************************************************************/
    /*!
        \brief Start a new timer
        \param name    Name of the timer
    *****************************************************************************/
    static void startTimer(const char* name);

    /*! ***************************************************************************/
    /*!
        \brief Stop the last started timer
    *****************************************************************************/
    static void stopTimer();
#endif
   
    typedef void (*LogFunction)(const char8_t *pText, void *data);

    /*! ***************************************************************************/
    /*!     
        \brief Sets the function to use to output debug messages.
        \param hook   The function to use to output debug messages.
        \param data   An optional data pointer to be passed back to the logging function.
    *****************************************************************************/
    static void setLoggingHook(LogFunction hook, void *data);   

    /*! ***************************************************************************/
    /*! \brief Gets the function currently set as the logging hook.
    *****************************************************************************/
    static LogFunction getLoggingHook();

    /*! ***************************************************************************/
    /*! \brief Gets the data currently passed into the logging hook.
    *****************************************************************************/
    static void* getLoggingHookData();


    /*! ***************************************************************************/
    /*!     
        \brief Enable or disable log buffering.  When false (default), the LoggingHook
               callback is called for each individual log line. 
        \param enabled true to enable, false to disable.
    *****************************************************************************/
    static void setLogBufferingEnabled(bool enabled);

    /*! ***************************************************************************/
    /*! \brief Stop prepend timestamp in blaze log
    
        Blaze logging prepend timestamp to log line by default. You may call disableBlazeLogTimeStamp() 
        to disable it if your logger function logs time stamp. 

        You may also define BLAZE_SUPPRESS_LOG_TIMESTAMP to compile it out. 

        Recommended to leave it on unless your logging function is already logging time stamp.
    *****************************************************************************/
    static void disableBlazeLogTimeStamp() { mEnableBlazeLogTimeStamp = false; }

    /*! ***************************************************************************/
    /*! \brief Stop prepend timestamp in blaze log
    *****************************************************************************/
    static void enableBlazeLogTimeStamp() { mEnableBlazeLogTimeStamp = true; }

#if ENABLE_BLAZE_SDK_LOGGING
    /*! ***************************************************************************/
    /*!     
        \brief Logs a debug message.
        \param msg   The debug message to log.
        \param trace [DEPRECATED] True if this message should be additionally trace logged.
    *****************************************************************************/
    static void Log(bool trace, const char8_t *msg);

    
    /*! ***************************************************************************/
    /*!     
        \brief Logs a formatted debug message.
        \param msg   The format string to log.
        \param trace [DEPRECATED] True if this message should be additionally trace logged.
    *****************************************************************************/
    static void Logf(bool trace, const char8_t *msg, ...) ATTRIBUTE_PRINTF_CHECK(2,3);

    static void LogSb(bool trace, const StringBuilder &msg);

#endif

    typedef void (*AssertFunction)(bool assertValue, const char8_t *assertLiteral, const char8_t *file, size_t line, void* data);

    /*! ***************************************************************************/
    /*!     
    \brief Sets the function to use to output assert messages. If SetAssertHook is not used, 
            there is a default AssertFunction implementation that simply logs an assertation failure message. 
    \param hook   The function to use to output assert messages. Can be set to nullptr to suppress all blaze Asserts.
    \param data   An optional data pointer to be passed back to the assert function.
    *****************************************************************************/
    static void SetAssertHook(AssertFunction hook, void *data); 

    /*! ***************************************************************************/
    /*! \brief Gets the function currently set as the assert hook.
    *****************************************************************************/
    static AssertFunction getAssertHook() { return sAssertFunction; }

    /*! ***************************************************************************/
    /*! \brief Gets the data currently passed into the assert hook.
    *****************************************************************************/
    static void* getAssertHookData() { return sAssertData; }

    /*! ************************************************************************************************/
    /*! \brief Calls the current AssertHook function. The default AssertHook function simply logs the message.
                This should not be called directly, instead, use the BlazeAssert macro.

    \param[in] assertValue the value to assert on
    \param[in] assertLiteral the string-ified assertion value
    \param[in] file the file the assertion was thrown from
    \param[in] line the line the assertion was thrown from
    ***************************************************************************************************/
    static void Assert(bool assertValue, const char8_t *assertLiteral, const char8_t *file, size_t line);
    
    typedef void (*VerifyFunction)(bool verifyValue, const char8_t *verifyLiteral, const char8_t *file, size_t line, void* data);

    /*! ***************************************************************************/
    /*!     
    \brief Sets the function to use to output verify messages. If SetVerifyHook is not used, 
            there is a default VerifyFunction implementation that simply logs a verification failure message. 
    \param hook   The function to use to output verify messages. Can be set to nullptr to suppress all blaze verifys.
    \param data   An optional data pointer to be passed back to the verify function.
    *****************************************************************************/
    static void SetVerifyHook(VerifyFunction hook, void *data);

    /*! ***************************************************************************/
    /*! \brief Gets the function currently set as the verify hook.
    *****************************************************************************/
    static VerifyFunction getVerifyHook() { return sVerifyFunction; }

    /*! ***************************************************************************/
    /*! \brief Gets the data currently passed into the verify hook.
    *****************************************************************************/
    static void* getVerifyHookData() { return sVerifyData; }

/*! ************************************************************************************************/
    /*! \brief Calls the current VerifyHook function. The default VerifyHook function simply logs the message.
                This should not be called directly, instead, use the BlazeVerify macro.

    \param[in] verifyValue the value to verify on
    \param[in] verifyLiteral the string-ified verify value
    \param[in] file the file the verify was thrown from
    \param[in] line the line the verify was thrown from
    ***************************************************************************************************/
    static void Verify(bool verifyValue, const char8_t *verifyLiteral, const char8_t *file, size_t line);


    static void logStart();
    static void logEnd();

    /*! ***************************************************************************/
    /*! \brief Release the memory allocated for log buffer.
    *****************************************************************************/
    static void clearLogBuffer();


    /*! ***************************************************************************/
    /*!     
        \brief [DEPRECATED] Sets the function to use to output trace messages.
        \param hook   The function to use to output trace messages.
        \param data   An optional data pointer to be passed back to the logging function.
    *****************************************************************************/
    static void setTraceHook(LogFunction hook, void *data) {}

    /*! ***************************************************************************/
    /*! \brief [DEPRECATED] Gets the function currently set as the trace hook.
    *****************************************************************************/
    static LogFunction getTraceHook() { return (LogFunction)nullptr; }

    /*! ***************************************************************************/
    /*! \brief [DEPRECATED]  Gets the data currently passed into the trace hook.
    *****************************************************************************/
    static void* getTraceHookData() { return nullptr; }

private:
    // private constructor and destructor
    // This class only provides a namespace for some static data and functions.  It should
    // never actually be instatiated by anything.
    Debug() { }
    ~Debug() { }

    /*! ************************************************************************************************/
    /*! \brief The default assert function simply logs an assertion failure message.

    \param[in] the value to assert on
    \param[in] assertLiteral the string-ified assertion value
    \param[in] the file the assertion was thrown from
    \param[in] the line the assertion was thrown from
    ***************************************************************************************************/
    static void DefaultAssertFunction(bool assertValue, const char8_t *assertLiteral, const char8_t *file, size_t line, void *data);

    static AssertFunction sAssertFunction;
    static void* sAssertData;
    static bool mEnableBlazeLogTimeStamp;

    /*! ************************************************************************************************/
    /*! \brief The default verify function simply logs a verify failure message.

    \param[in] the value to verify on
    \param[in] verifyLiteral the string-ified verify value
    \param[in] the file the verify was thrown from
    \param[in] the line the verify was thrown from
    ***************************************************************************************************/
    static void DefaultVerifyFunction(bool verifyValue, const char8_t *verifyLiteral,const char8_t *file, size_t line, void *data);

    static VerifyFunction sVerifyFunction;
    static void* sVerifyData;

    static bool sLogBufferingFeatureEnabled;

#if ENABLE_BLAZE_SDK_LOGGING
    static bool sLogBufferEnabled;
#endif
};

#ifdef EA_DEBUG
// NOTE: it's inefficient to always call the asserter function, but it prevents error C4127 on windows (if statement with constant value)
//  the assert function disables the warning, but I don't know of a way to disable or reenable it from inside a macro
#define BlazeAssert(x) { Blaze::Debug::Assert(x, #x, __FILE__, __LINE__); }
#define BlazeAssertMsg(x, msg) { Blaze::Debug::Assert(x, #x " --> " msg, __FILE__, __LINE__); }
#else
#define BlazeAssert(x)
#define BlazeAssertMsg(x, msg)
#endif // EA_DEBUG

#define BlazeVerify(x) {Blaze::Debug::Verify(x,#x, __FILE__,__LINE__);}
#define BlazeVerifyMsg(x, msg) {Blaze::Debug::Verify(x, #x " --> " msg, __FILE__,__LINE__);}

}       // namespace Blaze

#endif  // BLAZE_DEBUG
