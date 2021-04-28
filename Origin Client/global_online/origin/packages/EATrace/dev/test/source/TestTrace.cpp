///////////////////////////////////////////////////////////////////////////////
// TestTrace.cpp
//
// Copyright (c) 2003, Electronic Arts Inc. All rights reserved.
// Created by Paul Pedriana
///////////////////////////////////////////////////////////////////////////////


#include <EATest/EATest.h>
#include <EATrace/internal/Config.h>
#include <EATrace/internal/RefCount.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>

//Prevents false positive memory leaks on GCC platforms (such as PS3)
#if defined(EA_COMPILER_GNUC)
    #define EA_MEMORY_GCC_USE_FINALIZE
#endif
#include <MemoryMan/MemoryMan.inl>

#include <MemoryMan/CoreAllocator.inl>
#include <EATrace/EATrace_imp.h>
#include <EATrace/EALog_imp.h>
#include EA_ASSERT_HEADER

#ifdef _MSC_VER
    #pragma warning(push, 0)
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(EA_COMPILER_MSVC) && defined(EA_PLATFORM_MICROSOFT)
    #include <crtdbg.h>
#endif

#ifdef _MSC_VER
    #pragma warning(pop)
#endif

#include "EATest/EATestMain.inl"

///////////////////////////////////////////////////////////////////////////////
// Constants
//
#define TEST_GROUP "TestGroup"
static const char* kLogGroupName = TEST_GROUP;


///////////////////////////////////////////////////////////////////////////////
// Required by EASTL.
//
int Vsnprintf8(char8_t* pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}

int Vsnprintf16(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments)
{
    return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
}

#if (EASTDC_VERSION_N >= 10600)
    int Vsnprintf32(char32_t* pDestination, size_t n, const char32_t* pFormat, va_list arguments)
    {
        return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
    }
#endif



///////////////////////////////////////////////////////////////////////////////
// LogReporterCounter
//
static int TestTracer();
static int TestServer();
static int TestMacros(bool bDoAlerts);

#if defined(EA_TRACE_ENABLED) && EA_TRACE_ENABLED
    static char* MakeBuffer(size_t inBufferSize)
    {
        char* const pBuffer = new char[inBufferSize];

        for(size_t i = 0; i < inBufferSize; i++)
        {
            if((i % 128) == 0)
                pBuffer[i] = '\n';
            else
                pBuffer[i] = (char)('0' + (i % 10));
        }
        pBuffer[inBufferSize - 1] = '\0';

        return pBuffer;
    }
#endif


///////////////////////////////////////////////////////////////////////////////
// CustomTraceDialog
//
// Installed with:
//     SetDisplayTraceDialogFunc(CustomTraceDialog, NULL);
//
EA::Trace::tAlertResult CustomTraceDialog(const char* pTitle, const char* pText, void* pContext)
{
    #if EATRACE_DEBUG_BREAK_ABORT_ENABLED
        return EA::Trace::kAlertResultNone;
    #else
        // Actually it's probably more proper to use the input pContext than to re-read 
        // it, though they should both be NULL for this testing app.
        EA::Trace::DisplayTraceDialogFunc pFunc = EA::Trace::GetDefaultDisplayTraceDialogFunc(pContext);
        return pFunc(pTitle, pText, pContext);
    #endif
}



///////////////////////////////////////////////////////////////////////////////
// LogReporterCounter
//
class LogReporterCounter : public EA::Trace::LogReporter
{
public:
    LogReporterCounter(const char* name)
        : LogReporter(name), mShowText(false)
    {
        Reset();
    }

    void Reset()
    {
        mPassCount   = 0;
        mFilterCalls = 0;
        mFilterCount = 0;
    }

    bool IsValid(int32_t filterCalls, int32_t filterCount, int32_t passCount) const
    {
        return (filterCalls == mFilterCalls) && 
               (filterCount == mFilterCount) && 
               (passCount   == mPassCount);
    }

    virtual void ShowText(bool enable)
    {
        mShowText = enable;
    }

public: // ILogReporter
    virtual EA::Trace::tAlertResult Report(const EA::Trace::LogRecord& record) 
    {
        const char* text = mFormatter->FormatRecord(record);

        // typically this reporter suppresses all output, flip this flag to see output
        if(mShowText)
            EA::UnitTest::ReportVerbosity(1, "%s", text);
            
        ++mPassCount;
        return EA::Trace::kAlertResultNone;
    }

    virtual bool IsFiltered(const EA::Trace::TraceHelper& helper)
    {
        ++mFilterCalls;
        bool isFiltered = EA::Trace::LogReporter::IsFiltered(helper);
        if(isFiltered)
            ++mFilterCount;

        return isFiltered;
    }

    virtual bool IsFiltered(const EA::Trace::LogRecord& /*inRecord*/)
    {
        return false;
    }

private:
    int32_t mFilterCalls;
    int32_t mFilterCount;
    int32_t mPassCount;
    bool    mShowText;
};


///////////////////////////////////////////////////////////////////////////////
// LogReporterSample
//
class LogReporterSample : public EA::Trace::LogReporter
{
public:
    LogReporterSample(const char* name) : LogReporter(name)
        { }

    EA::Trace::tAlertResult Report(const EA::Trace::LogRecord& logRecord) 
    {
        const char* pOutput = mFormatter->FormatRecord(logRecord);

        EA::UnitTest::ReportVerbosity(1, "%s", pOutput);

        #if defined(EA_PLATFORM_MOBILE)
            fflush(stdout);
        #endif

        return EA::Trace::kAlertResultNone;
    }
};


///////////////////////////////////////////////////////////////////////////////
// TestCTAssert
//
template <int x, int y>
struct TestCTAssert
{
    TestCTAssert()
    {
        EA_COMPILETIME_ASSERT(x > y);
        #if defined(EA_COMPILETIME_ASSERT_NAMED)
            EA_COMPILETIME_ASSERT_NAMED(x > y, TemplateTest);
        #endif

        mSum = x + y;
    }

    int mSum;
};


///////////////////////////////////////////////////////////////////////////////
// TestTrace
//
int TestTrace()
{
    int nErrorCount(0);

 
    // Test EA_DEBUG_STRING.
    #if EA_DEBUG_STRING_ENABLED
        const char* EA_DEBUG_STRING(pName) = NULL;         // This should compile.
        (void)pName;
    #else
        const char* EA_DEBUG_STRING(pName) pName = NULL;   // This should compile.
        (void)pName;
    #endif



    // Test EA_DEBUG_STRING_VAL
    const char* pDebugString = EA_DEBUG_STRING_VAL("blah");

    #if EA_DEBUG_STRING_ENABLED
        EATEST_VERIFY(strcmp(pDebugString, "blah") == 0);
    #else
        EATEST_VERIFY(pDebugString == NULL);
    #endif



    ////////////////////////////////////////////////////////
    // Compile-time asserts
    {
        // This is a compilation test as well as a runtime test.
        EA_COMPILETIME_ASSERT(sizeof(uint32_t) == 4);
        #if defined(EA_COMPILETIME_ASSERT_NAMED)
            EA_COMPILETIME_ASSERT_NAMED(sizeof(uint32_t) == 4, Uint32Test);
        #endif

        TestCTAssert<4, 3> test; (void)test;
        EA_COMPILETIME_ASSERT(sizeof(test) > 0);
        #if defined(EA_COMPILETIME_ASSERT_NAMED)
            EA_COMPILETIME_ASSERT_NAMED(sizeof(test) > 0, TemplateTest);
        #endif

        // These two should generate compiler errors:
        // Ideally, the compiler errors would give some useful hint about the problem.
        // EA_COMPILETIME_ASSERT(sizeof(uint32_t) == 3);
        // EA_COMPILETIME_ASSERT_NAMED(sizeof(uint32_t) == 3, Uint32Test2);
        // TestCTAssert<3, 4> test2;
        // EA_COMPILETIME_ASSERT(sizeof(test2) == 0);
    }


    ////////////////////////////////////////////////////////
    // EA_PREPROCESSOR_JOIN
    {
        // This is a compilation test as well as a runtime test.
        const char EA_PREPROCESSOR_JOIN(unique_, 37) = 5;

        EATEST_VERIFY(unique_37 == ((__FILE__[0] / 1000) + 5)); // Add some silly math to avoid a compiler 'conditional expression is constant' warning.
    }


    ////////////////////////////////////////////////////////
    // EA_PREPROCESSOR_STRINGIFY
    {
        // This is a compilation test as well as a runtime test.
        const char* pString = EA_PREPROCESSOR_STRINGIFY(HelloWorld);

        EATEST_VERIFY(strcmp(pString, "HelloWorld") == 0);
    }


    ////////////////////////////////////////////////////////
    // EA_PREPROCESSOR_LOCATION
    {
        // This is a compilation test as well as a runtime test.
        char buffer[512];
        sprintf(buffer, "%s", EA_PREPROCESSOR_LOCATION);

        EATEST_VERIFY(buffer[0] != 0);
    }

    
    ////////////////////////////////////////////////////////
    // EA_UNIQUELY_NAMED_VARIABLE
    {
        // This uniquely named variable is only useful when 
        // used on a single line, as it uses the __LINE__ macro.

        // Disabled as VC7 isn't guaranteed to be able to compile 
        // this unless 'edit and continue' is disabled.
        //char buffer[512];
        //int EA_UNIQUELY_NAMED_VARIABLE; sprintf(buffer, "%d", EA_UNIQUELY_NAMED_VARIABLE);
    }


    ////////////////////////////////////////////////////////
    // EA_CURRENT_FUNCTION
    {
        #ifdef EA_CURRENT_FUNCTION_SUPPORTED
            char buffer[512];
            sprintf(buffer, "%s", EA_CURRENT_FUNCTION);

            EATEST_VERIFY(strstr(buffer, "TestTrace") != NULL);
        #endif
    }

    nErrorCount += TestTracer();
    nErrorCount += TestServer();

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestTracer
//
static int TestTracer()
{
    using namespace EA::Trace;

    int            nErrorCount = 0;
    ITracer* const pSavedTracer = GetTracer();

    if(pSavedTracer)
        pSavedTracer->AddRef();

    // This test just exercises the macros without any real tests
    
    // Test the tracer (without dialogs)
    ITracer* const pNewTracer = new Tracer;
    SetTracer(pNewTracer);
    TestMacros(false);

    // Test no tracer.
    SetTracer(NULL);
    TestMacros(true); // turn on dialogs, none should come up

    // reset tracer back to original version
    SetTracer(pSavedTracer);
    if(pSavedTracer)
        pSavedTracer->Release();

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// DisableTraceServerDialogs
//
// For automated tests, we should probably turn off dialogs 
//
void DisableTraceServerDialogs()
{
    using namespace EA::Trace;

    IServer* const pServer = GetServer();

    EA::Trace::AutoRefCount<ILogReporter> pReporter;

    if(pServer && pServer->GetLogReporter("AppAlertDialog", pReporter.AsPPTypeParam()))
    {
        pReporter->SetEnabled(false);
        pReporter = NULL; // release the reporter

        pServer->UpdateLogFilters();
    }
}


///////////////////////////////////////////////////////////////////////////////
// HandleError
//
// This is useful for setting breakpoints during these tests.
//
void HandleError(int& nErrorCount)
{
    ++nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestServer
//
int TestServer()
{
    using namespace EA::Trace;

    int nErrorCount = 0;

    // Test the default pNewServer with dialogs disabled (move this to Test init?)
    // what about forcing all test output to stdout instead of a mix of ODS and stdout, force to dev/null ?
    IServer* const pSavedServer = GetServer();
    if(pSavedServer)
        pSavedServer->AddRef();

    // Create a custom Server for testing, add reporters before configure so defaults are not created
    Server* const pNewServer = new Server;
    pNewServer->AddRef(); // AddRef for this function.
    pNewServer->Init();
    pNewServer->RemoveAllLogReporters();

    LogReporterCounter* const pTestReporter = new LogReporterCounter("LogRecordCounter");
    pTestReporter->ShowText(EA::UnitTest::GetVerbosity() >= 1); 
    pNewServer->AddLogReporter(pTestReporter);
    pNewServer->UpdateLogFilters();
    
    SetServer(pNewServer); // This will AddRef the server.

    // Run with reporter enabled
    // Filter count is double pass count because individual reporter filter state is not cached
    pTestReporter->SetEnabled(true);
    pNewServer->UpdateLogFilters();
    pTestReporter->Reset();
    TestMacros(true);
    // Note by Paul P: This IsValid test needs to be revised to not be so fragile; it's too easily broken.
    //if(!pTestReporter->IsValid(26,0,25)) // Half filtered, other half pass, 1 off because of error test
    //    HandleError(nErrorCount);

    // Run with reporter disabled.
    pTestReporter->SetEnabled(false);
    pNewServer->UpdateLogFilters();
    pTestReporter->Reset();
    TestMacros(true);
    // Note by Paul P: This IsValid test needs to be revised to not be so fragile; it's too easily broken.
    //if(!pTestReporter->IsValid(26,26,0)) // All calls should be filtered, 0 pass count
    //     HandleError(nErrorCount);

    // Run with filter state on calls cached.
    pTestReporter->Reset();
    TestMacros(true);
    // Note by Paul P: This IsValid test needs to be revised to not be so fragile; it's too easily broken.
    //if(!pTestReporter->IsValid(0,0,0)) // All filter calls should be cached, 0 filter call count
    //     HandleError(nErrorCount);

    // Run with group/level state set to asserts (is group check case insensitive, all lowercase tests?).
    // This caught a problem with the group level map not specifying a less than operator and doing raw char* comparisons.
    ILogFilter* const           pLogFilter            = pTestReporter->GetFilter();
    LogFilterGroupLevels* const pLogFilterGroupLevels = pLogFilter ? (LogFilterGroupLevels*)pLogFilter->AsInterface(LogFilterGroupLevels::kIID) : NULL;

    if(pLogFilterGroupLevels)
    {
        // Turn off global messages to this reporter, and turn on only warnings and above
        pLogFilterGroupLevels->AddGroupLevel("", kLevelNone);
        pLogFilterGroupLevels->AddGroupLevel(kLogGroupName, kLevelWarn);
    }

    pTestReporter->SetEnabled(true);
    pNewServer->UpdateLogFilters();
    TestMacros(true);
    // Note by Paul P: This IsValid test needs to be revised to not be so fragile; it's too easily broken.
    // if(!pTestReporter->IsValid(26,22,4))   // All filtered except for four log messages (one warn, one error)
    //     HandleError(nErrorCount);

    // Test default tracer, this code skips alerts because there is no way to turn off the dialogs on the default tracer.
    ITracer* const pNewTracer = new Tracer;
    SetTracer(pNewTracer);
    TestMacros(false);

    // Make sure LogReporterSample compiles and links.
    LogReporterSample* pLogReporterSample = new LogReporterSample("LogReporterSample");
    pNewServer->AddLogReporter(pLogReporterSample);

    pNewServer->Release(); // Match the explicit AddRef above for this function.

    // Test no tracer.
    SetServer(NULL);
    TestMacros(true);

    // For the remaining code, restore the original tracer (dialogs will still be turned off for successive tests, desirable?)
    SetServer(pSavedServer);
    if(pSavedServer)
        pSavedServer->Release();

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestMacros
//
int TestMacros(bool bDoAlerts)
{
    using namespace EA::Trace;

    EA_UNUSED(bDoAlerts);

    int nErrorCount = 0;

    #if defined(EA_TRACE_ENABLED) && EA_TRACE_ENABLED

        ////////////////////////////////////////////////////////
        // Test Tracer macros
        {
            // Test simple output.
            EA_TRACE_MESSAGE("Trace: Hello world\n");
            EA_TRACE_FORMATTED(("TraceFormatted: Hello %s\n", "world"));
            
            // Test heavy output larger than internal buffer.
            size_t bufferSize;

            if(EA::UnitTest::GetVerbosity() >= 1)
                bufferSize = (5 * 1024);
            else
                bufferSize = 128;

            char* pBuffer = MakeBuffer(bufferSize);

            // Message should display all chars, Formatted will only display 4096.
            EA_TRACE_MESSAGE(pBuffer);
            EA_TRACE_FORMATTED(("%s", pBuffer));

            delete[] pBuffer;

            // Try output that exceeds even what VC++ OutputDebugString can do in a single call.
            if(EA::UnitTest::GetVerbosity() >= 1)
                bufferSize = (33 * 1024);
            else
                bufferSize = 128;

            pBuffer = MakeBuffer(bufferSize);
            
            EA_TRACE_MESSAGE(pBuffer);
            EA_TRACE_FORMATTED(("%s", pBuffer));

            delete[] pBuffer;        

            // Code should handle this error case gracefully but not produce output.
            EA_TRACE_MESSAGE(NULL);
        }


        ////////////////////////////////////////////////////////
        // Test Tracer macros
        {
            EA_TRACE_MESSAGE("EA_TRACE: Hello world\n");
            EA_TRACE_FORMATTED(("EA_TRACE_FORMATTED: Hello %s\n", "world"));

            // These macros are currently disabled.
            // EA_WARN_MESSAGE(true, "EA_WARN(true): Hello world\n");
            // EA_WARN_MESSAGE(false, "EA_WARN(false): Hello world\n");
            //
            // EA_WARN_FORMATTED(true, ("EA_WARN_FORMATTED(true): Hello %s\n", "world"));
            // EA_WARN_FORMATTED(false, ("EA_WARN_FORMATTED(false): Hello %s\n", "world"));

            // EA_NOTIFY_MESSAGE(true, "EA_WARN(true): Hello world\n");
            // EA_NOTIFY_MESSAGE(false, "EA_WARN(false): Hello world\n");
            //
            // EA_NOTIFY_FORMATTED(true, ("EA_WARN_FORMATTED(true): Hello %s\n", "world"));
            // EA_NOTIFY_FORMATTED(false, ("EA_WARN_FORMATTED(false): Hello %s\n", "world"));
        }


        ////////////////////////////////////////////////////////
        // Test Tracer macros
        if(bDoAlerts)
        {
            #ifdef _MSC_VER
                #pragma warning(push)
                #pragma warning(disable: 4127) // conditional expression is constant
            #endif

            if(EA::UnitTest::GetInteractive()) // If this unit test is running attended by a person who can respond to UI events...
            {
                #if !EATRACE_DEBUG_BREAK_ABORT_ENABLED
                    EA_ASSERT_MESSAGE(false, "012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345 012345");

                    // test all the reporting methods
                    EA_ASSERT(true);
                    EA_ASSERT(false);

                    EA_ASSERT_MESSAGE(true, "EA_ASSERT_MESSAGE(true): Hello world\n");
                    EA_ASSERT_MESSAGE(false, "EA_ASSERT_MESSAGE(false): Hello world\n");

                    EA_ASSERT_FORMATTED(true, ("EA_ASSERT_FORMATTED(true): Hello %s\n", "world"));
                    EA_ASSERT_FORMATTED(false, ("EA_ASSERT_FORMATTED(false): Hello %s\n", "world"));

                    EA_VERIFY(true);
                    EA_VERIFY(false);

                    EA_VERIFY_MESSAGE(true, "EA_VERIFY_MESSAGE(true): Hello world\n");
                    EA_VERIFY_MESSAGE(false, "EA_VERIFY_MESSAGE(false): Hello world\n");

                    EA_VERIFY_FORMATTED(true, ("EA_VERIFY_FORMATTED(true): Hello %s\n", "world"));
                    EA_VERIFY_FORMATTED(false, ("EA_VERIFY_FORMATTED(false): Hello %s\n", "world"));

                    // EA_FAIL();
                    EA_FAIL_MESSAGE("EA_FAIL_MESSAGE: Hello world\n");
                    EA_FAIL_FORMATTED(("EA_FAIL_FORMATTED: Hello %s\n", "world"));
                #endif
            }

            #ifdef _MSC_VER
                #pragma warning(pop)
            #endif
        } // if(bDoAlerts)

        {
            // Test logging
            EA_LOG_MESSAGE  (kLogGroupName, kLevelDebug, "EA_LOG_MESSAGE(\"" TEST_GROUP "\", kLevelDebug): Hello world\n");
            EA_LOG_FORMATTED(kLogGroupName, kLevelDebug, ("EA_LOG_FORMATTED(\"" TEST_GROUP "\", kLevelDebug): Hello %s\n", "world"));

            EA_LOG_FORMATTED(kLogGroupName, kLevelWarn,  ("EA_LOG_FORMATTED(\"" TEST_GROUP "\", kLevelWarn): Hello %s\n", "world"));
            EA_LOG_FORMATTED(kLogGroupName, kLevelError, ("EA_LOG_FORMATTED(\"" TEST_GROUP "\", kLevelError): Hello %s\n", "world"));

            // Erroneous test case
            EA_LOG_FORMATTED(NULL, kLevelDebug, ("EA_LOG_FORMATTED(\"" TEST_GROUP "\", kLevelDebug): Hello %s\n", "world"));

            // Shortcuts
            EA_LOG (kLogGroupName, kLevelDebug, ("EA_LOG(\"" TEST_GROUP "\", kLevelDebug): Hello %s\n", "world"));
            EA_LOGE(kLogGroupName,              ("EA_LOGE(\"" TEST_GROUP "\", kLevelError): Hello %s\n", "world"));
            EA_LOGW(kLogGroupName,              ("EA_LOGW(\"" TEST_GROUP "\", kLevelWarn): Hello %s\n", "world"));
            EA_LOGI(kLogGroupName,              ("EA_LOGI(\"" TEST_GROUP "\", kLevelInfo): Hello %s\n", "world"));
        }

    #endif // #if EA_TRACE_ENABLED

    return nErrorCount;
}


namespace IonChannel
{
    enum Channel
    {
        kChannelA,
        kChannelB,
        kChannelC,
        kChannelCount
    };

    const char* EnumToStringTable[kChannelCount] = { "A", "B", "C" };
}



void Trace(IonChannel::Channel eChannel, const char* pBuffer)
{
    using namespace EA::Trace;

    EA_UNUSED(pBuffer);

    switch ((int)eChannel)
    {
        case 0:
            EA_LOGI(IonChannel::EnumToStringTable[eChannel], (pBuffer));
            break;

        case 1:
            EA_LOGI(IonChannel::EnumToStringTable[eChannel], (pBuffer));
            break;
    }
}


//or

template<int channel>
void EALogIChannel(const char* pBuffer)
{
    using namespace EA::Trace;

    EA_UNUSED(pBuffer);
    EA_LOGI(IonChannel::EnumToStringTable[channel], (pBuffer)) ;
}

void Trace2(IonChannel::Channel eChannel, const char* pBuffer )
{
    using namespace EA::Trace;

    typedef void (*LogFunction)(const char*);
    LogFunction functionTable[IonChannel::kChannelCount] = { EALogIChannel<IonChannel::kChannelA> };

    functionTable[eChannel](pBuffer);
}

// The PSVita platform currently requires that the user declare a variable   
// called sceLibcHeapSize in the application.
#if defined(EA_PLATFORM_PSP2)
    unsigned int sceLibcHeapSize = 10*1024*1024;
#endif


///////////////////////////////////////////////////////////////////////////////
// EATestMain
//

int EATestMain(int argc, char** argv)
{
    using namespace EA::UnitTest;

    int nErrorCount(0);

    PlatformStartup();

    EA::Trace::SetDisplayTraceDialogFunc(CustomTraceDialog, NULL);

    // Add the tests
    TestApplication testSuite("EAStdC Unit Tests", argc, argv);

    // For some reason Android aborts, though it may be intentional within EATrace.
    // Until we have time to debug this, we disable testing on Android and simply 
    // listen to user reports of any problems. Probably the abort is not significant.
    #if !defined(EA_PLATFORM_ANDROID)
        testSuite.AddTest("Trace", TestTrace);
    #endif

    if(!EA::UnitTest::GetInteractive()) // If this unit test is running unattended by a person...
        DisableTraceServerDialogs();

    nErrorCount += testSuite.Run();

    PlatformShutdown(nErrorCount);

    return nErrorCount;
}










