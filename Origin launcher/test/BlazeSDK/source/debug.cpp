/*! ***************************************************************************/
/*!
    \file 

    Source file for debug routines.

    \attention
        (c) Electronic Arts. All Rights Reserved.

*******************************************************************************/

#include "BlazeSDK/internal/internal.h"

#include <stdio.h>
#include <stdarg.h>

#include "BlazeSDK/debug.h"
#include "BlazeSDK/blaze_eastl/string.h"
#include "DirtySDK/platform.h"

#if defined(EA_PLATFORM_MICROSOFT)
#pragma warning(push,0)
#include <windows.h> // for OutputDebugStringA
#pragma warning(pop)
#endif

#if ENABLE_BLAZE_SDK_LOGGING
#ifndef BLAZE_SUPPRESS_LOG_TIMESTAMP
#include "EATDF/time.h"
#endif
#endif

#include "EAAssert/eaassert.h"

namespace Blaze
{

//
// Constants
//

#if ENABLE_BLAZE_SDK_LOGGING
#ifndef BLAZE_SDK_LOGGER_ALLOC_SIZE
#define BLAZE_SDK_LOGGER_ALLOC_SIZE 1024
#endif
static const int DEFAULT_LOGGER_ALLOC_SIZE = BLAZE_SDK_LOGGER_ALLOC_SIZE;
#endif


bool Debug::mEnableBlazeLogTimeStamp = true;

//
// Methods
//

#if ENABLE_BLAZE_SDK_PROFILING
ScopeTimer::ScopeTimer(const char* name)
{
    Debug::startTimer(name);
}

ScopeTimer::~ScopeTimer()
{
    Debug::stopTimer();
}
#endif

static void DefaultPerfStartFunc(const char* name) {}
static void DefaultPerfStopFunc() {}

static Debug::PerfStartFunc sPerfStartFunction = DefaultPerfStartFunc;
static Debug::PerfStopFunc sPerfStopFunction = DefaultPerfStopFunc;

void Debug::setPerfHooks(PerfStartFunc startHook, PerfStopFunc stopHook)
{
    sPerfStartFunction = startHook;
    sPerfStopFunction = stopHook;
}

#if ENABLE_BLAZE_SDK_PROFILING
void Debug::startTimer(const char* name)
{
    sPerfStartFunction(name);
}

void Debug::stopTimer()
{
    sPerfStopFunction();
}
#endif

static void DefaultLogFunction(const char *msg, void *data)
{
    printf("%s", msg);
#if defined(EA_PLATFORM_MICROSOFT)
    OutputDebugStringA(msg);
#endif
}
static Debug::LogFunction sLogFunction = DefaultLogFunction;
static void *sLogData = nullptr;

#if ENABLE_BLAZE_SDK_LOGGING
bool Debug::sLogBufferEnabled = false;
static Blaze::string *sLogBuffer = nullptr;
#endif

void Debug::setLoggingHook(LogFunction hook, void *data)
{
    sLogFunction = hook;
    sLogData = data;
}

bool Debug::sLogBufferingFeatureEnabled = false;
void Debug::setLogBufferingEnabled(bool enabled)
{
    sLogBufferingFeatureEnabled = enabled;
}

void Debug::clearLogBuffer()
{
#if ENABLE_BLAZE_SDK_LOGGING
    BLAZE_DELETE(MEM_GROUP_DEFAULT_TEMP, sLogBuffer);
    sLogBuffer = nullptr;
#endif
}

#if ENABLE_BLAZE_SDK_LOGGING
void Debug::logStart()
{
    if (sLogBufferingFeatureEnabled)
        sLogBufferEnabled = true;
}

void Debug::logEnd()
{
    if (sLogBuffer == nullptr)
        sLogBuffer = BLAZE_NEW(MEM_GROUP_DEFAULT_TEMP, "Debug::LogBuffer") Blaze::string;

    if (sLogBufferingFeatureEnabled && sLogBuffer != nullptr)
    {
        sLogBufferEnabled = false;
        if (sLogFunction != nullptr)
            sLogFunction(sLogBuffer->c_str(), sLogData);
        sLogBuffer->clear();
    }
}

void Debug::Log(bool trace, const char *msg)
{
    if (sLogBuffer == nullptr)
        sLogBuffer = BLAZE_NEW(MEM_GROUP_DEFAULT_TEMP, "Debug::LogBuffer") Blaze::string;

    if (sLogFunction != nullptr)
    {
        if (sLogBufferEnabled && sLogBuffer != nullptr)
            sLogBuffer->append(msg);
        else 
            sLogFunction(msg, sLogData);
    }
}

Debug::LogFunction Debug::getLoggingHook()
{
    return sLogFunction;
}

void* Debug::getLoggingHookData()
{
    return sLogData;
}

void Debug::Logf(bool trace, const char *msg, ...)
{
    if (sLogFunction == nullptr)
        return;

    va_list args;
    char strText[DEFAULT_LOGGER_ALLOC_SIZE];

    int printtimeoffset = 0;
#ifndef BLAZE_SUPPRESS_LOG_TIMESTAMP
    if (mEnableBlazeLogTimeStamp)
    {
        struct tm tm;
        int32_t iMsec;

        ds_plattimetotimems(&tm, &iMsec);
        printtimeoffset = ds_snzprintf(strText, DEFAULT_LOGGER_ALLOC_SIZE, "%d/%02d/%02d-%02d:%02d:%02d.%03.3d ",
            tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, iMsec);
    }
#endif

    va_start(args, msg);
    int textLogLen = vsnprintf(strText+printtimeoffset, sizeof(strText)-printtimeoffset-1, msg, args);
    va_end(args);

    if (textLogLen > (int) sizeof(strText) - printtimeoffset - 1)
        textLogLen = sizeof(strText) - printtimeoffset - 1;

    //make sure strText has \n at the end.
    if (textLogLen > 0 && strText[printtimeoffset + textLogLen - 1] != '\n')
        strText[printtimeoffset + textLogLen - 1] = '\n';

    //make sure strText is null terminated
    strText[sizeof(strText) - 1] = 0;
    Log(trace, strText);
}

void Debug::LogSb(bool trace, const StringBuilder &msg)
{
    Logf(trace, "%s\n", msg.get());
}
#endif
/*! ************************************************************************************************/
/*! \brief The default assert function simply logs an assertion failure message.  Don't call directly;
        use the BlazeAssert macro.

    \param[in] the value to assert on
    \param[in] assertLiteral the string-ified assertion value
    \param[in] the file the assertion was thrown from
    \param[in] the line the assertion was thrown from
    \param[in] not used - optional data pointer 
***************************************************************************************************/
void Debug::DefaultAssertFunction(bool assertValue, const char8_t *assertLiteral, const char8_t *file, size_t line, void *data)
{
    if (!assertValue)
    {
        BLAZE_SDK_DEBUGF("ERR: BlazeAssert failed! BlazeAssert(%s)\n%s(%" PRIsize ")\n", assertLiteral, file, line);
        EA_DEBUG_BREAK();
    }   
}

Debug::AssertFunction Debug::sAssertFunction = Debug::DefaultAssertFunction;
void* Debug::sAssertData = nullptr;

void Debug::SetAssertHook(AssertFunction hook, void *data)
{
    sAssertFunction = hook;
    sAssertData = data;
}

void Debug::Assert(bool assertValue, const char8_t *assertLiteral, const char8_t *file, size_t line)
{
    if (sAssertFunction != nullptr)
        sAssertFunction(assertValue, assertLiteral, file, line, sAssertData);
}

/*! ************************************************************************************************/
/*! \brief The default verify function simply logs a verify failure message.  Don't call directly;
        use the BlazeVerify macro.

    \param[in] the value to verify on
    \param[in] verifyLiteral the string-ified verify value
    \param[in] the file the verify was thrown from
    \param[in] the line the verify was thrown from
    \param[in] optional data pointer 
***************************************************************************************************/
void Debug::DefaultVerifyFunction(bool verifyValue, const char8_t *verifyLiteral, const char8_t *file, size_t line, void *data)
{
    if (!verifyValue)
    {
        BLAZE_SDK_DEBUGF("ERR: BlazeVerify failed! BlazeVerify(%s) %s(%" PRIsize ")\n", verifyLiteral, file, line);
    }   
}

Debug::VerifyFunction Debug::sVerifyFunction = Debug::DefaultVerifyFunction;
void* Debug::sVerifyData = nullptr;

void Debug::SetVerifyHook(VerifyFunction hook, void *data)
{
    sVerifyFunction = hook;
    sVerifyData = data;
}

void Debug::Verify(bool verifyValue, const char8_t *verifyLiteral, const char8_t *file, size_t line)
{
    if(sVerifyFunction != nullptr)
        sVerifyFunction(verifyValue,verifyLiteral,file,line,sVerifyData);
}
} // namespace Blaze
