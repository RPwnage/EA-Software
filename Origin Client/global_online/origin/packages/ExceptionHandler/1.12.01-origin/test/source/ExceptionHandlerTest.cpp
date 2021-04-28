///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerTest.cpp
// 
// Copyright (c) 2005, Electronic Arts. All Rights Reserved.
// Maintained by Paul Pedriana
//
///////////////////////////////////////////////////////////////////////////////


// Test to verify that the platform-specific ExceptionHandler header can be included
#if defined(EA_PLATFORM_WINDOWS)
    #include <ExceptionHandler/Win32/ExceptionHandlerWin32.h>
#elif defined(EA_PLATFORM_PS3)
    #include <ExceptionHandler/PS3/ExceptionHandlerPS3.h>
#elif defined(EA_PLATFORM_XENON)
    #include <ExceptionHandler/Xenon/ExceptionHandlerXenon.h>
#endif // To do: Add others as they are verified.

#include <ExceptionHandler/ExceptionHandler.h>
#include <ExceptionHandler/ReportWriter.h>
#include <EACallstack/EAAddressRep.h>
#include <eathread/eathread.h>
#include <eathread/eathread_thread.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#if EASTDC_VERSION_N >= 10400
    #include <EAStdC/EAProcess.h>
#endif
#include <EAIO/PathString.h>
#include <EAIO/EAFileUtil.h>
#include <eathread/eathread_thread.h>
#include <MemoryMan/MemoryMan.inl>
#include <MemoryMan/CoreAllocator.inl>
#include <EATest/EATest.h>
#include <stdio.h>
#if defined(EA_PLATFORM_PS3)
    #include <netex/net.h>
#endif

#include "EAMain/EAEntryPointMain.inl"

EA::Callstack::AddressRepCache gAddressRepCache;



///////////////////////////////////////////////////////////////////////////////
// vsnprintf
//
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
// Exception generation functions
///////////////////////////////////////////////////////////////////////////////
void TestAccessViolation(void*)
{
    EA::Debug::CreateException(EA::Debug::kExceptionTypeAccessViolation);
    EA::Thread::ThreadSleep(3000); // This allows us to do debugging on some platforms.
}

void TestIllegalInstruction(void*)
{
    EA::Debug::CreateException(EA::Debug::kExceptionTypeIllegalInstruction);
    EA::Thread::ThreadSleep(3000);
}

void TestDivideByZero(void*)
{
    EA::Debug::CreateException(EA::Debug::kExceptionTypeDivideByZero);
    EA::Thread::ThreadSleep(3000);
}

void TestStackOverflow(void*)
{
    EA::Debug::CreateException(EA::Debug::kExceptionTypeStackOverflow);
    EA::Thread::ThreadSleep(3000);
}

void TestStackCorruption(void*)
{
    EA::Debug::CreateException(EA::Debug::kExceptionTypeStackCorruption);
    EA::Thread::ThreadSleep(3000);
}



///////////////////////////////////////////////////////////////////////////////
// ExceptionHandlerClientTest
//
class ExceptionHandlerClientTest : public EA::Debug::ExceptionHandlerClient
{
public:
    void Notify(EA::Debug::ExceptionHandler* pHandler, EA::Debug::ExceptionHandler::EventId id)
    {
        #if 0 // To do: Predicate this on something specific which we can control.
            if(id == EA::Debug::ExceptionHandler::kExceptionHandlingEnd)
            {
                EA::Debug::ReportWriter* pReportWriter = pHandler->GetReportWriter();

                if(pReportWriter->ReadReportBegin())
                {
                    char    buffer[256 + 1]; // +1 for the 0 we append below.
                    ssize_t size = 0;

                    // We may need to revise this to write to a temp string first, as some platforms
                    // unilaterally put a newline after each debug output statement.
                    while((size = (ssize_t)pReportWriter->ReadReportPart(buffer, EAArrayCount(buffer) - 1)) > 0)
                    {
                        buffer[size] = 0;
                        EA::UnitTest::Report("%s", buffer);
                    }

                    pReportWriter->ReadReportEnd();
                }
            }
        #endif
    }
};

ExceptionHandlerClientTest gExceptionHandlerClientTest;



///////////////////////////////////////////////////////////////////////////////
// CustomReportWriter
//
// Currently we trap BeginSection/EndSection for "Call stack", but this won't
// be necessary after a slight mod to ReportWriter is implemented in the next
// release.
//
class CustomReportWriter : public EA::Debug::ReportWriter
{
public:
    CustomReportWriter() : mWriteEnabled(false)
        { mCallstackBuffer[0] = 0; }

    virtual void GetFileNameExtension(char* pExtension, size_t /*capacity*/)
        { *pExtension = 0; }

    virtual bool OpenReport(const char* /*pReportPath*/)
        { return true; }

    virtual bool CloseReport()
        { return true; }

    virtual bool BeginReport(const char* /*pReportName*/)
    {
        mCallstackBuffer[0] = 0;
        return true;
    }

    virtual bool EndReport()
    {
        // Implement your display of mCallstackBuffer here.
        printf("%s", mCallstackBuffer);
        return true;
    }

    virtual bool BeginSection(const char* pSectionName)
    {
        if(strcmp(pSectionName, "Call stack") == 0)
            mWriteEnabled = true;
        return true;
    }

    virtual bool EndSection(const char* pSectionName)
    {
        if(strcmp(pSectionName, "Call stack") == 0)
            mWriteEnabled = false;
        return true;
    }

    virtual bool Write(const char* pData, size_t count)
    {
        if(mWriteEnabled)
        {
            if(count == (size_t)-1)
                count = strlen(pData);

            strncat(mCallstackBuffer, pData, count);
        }

        return true;
    }

    //virtual bool WriteCallStack(const void* pPlatformContext)
    //{
    //    // Possibly implement your own version here to customize the output; 
    //    // you'd probably want to copy the base class version and modify it.
    //    // The problem with the base version is that it may provide more
    //    // callstack information than you want.
    //}

    const char* GetCallstackText() const
        { return mCallstackBuffer; }

protected:
    bool mWriteEnabled;
    char mCallstackBuffer[4096];  // An actual implementation may want to revise this.
};



///////////////////////////////////////////////////////////////////////////////
// TestExceptionSimulationHandling
//
static int TestExceptionSimulationHandling()
{
    using namespace EA::Debug;
    
    int nErrorCount = 0;

    // Set up the ExceptionHandler.
    ExceptionHandler eh;

    // Register a callback client. You don't need to do this, but we do it to
    // exercise the functionality.
    eh.RegisterClient(&gExceptionHandlerClientTest, true);

    // Set the exception handler to continue execution after the RunTrapped
    // calls below. This is as opposed to using kActionTerminate, which will
    // terminate the application, and as opposed to using kActionThrow, which 
    // rethrows the exception for some possible high level handler to catch.
    eh.SetMode(ExceptionHandler::kModeDefault); // This isn't actually necessary, but we put it here for instructive purposes.
    eh.SetEnabled(false);
    eh.SetAction(ExceptionHandler::kActionContinue);
    eh.EnableReportTypes(ExceptionHandler::kReportTypeException);
    eh.EnableReportFields(ExceptionHandler::kReportFieldAll);
    eh.SetBuildDescription("TestExceptionSimulationHandling");
    eh.GetReportWriter()->SetOption(ReportWriter::kOptionReportUserName, 1);
    eh.GetReportWriter()->SetOption(ReportWriter::kOptionReportComputerName, 1);
    eh.GetReportWriter()->SetOption(ReportWriter::kOptionReportProcessList, 1);

    // The following should not generate an exception but should write a report
    // out as if it was handling one.
    eh.SimulateExceptionHandling(kExceptionTypeAccessViolation);

    return nErrorCount;
}



// This function returns (intptr_t)pContext. 
static intptr_t EmptyThreadFunction(void* pContext)
    { return reinterpret_cast<intptr_t>(pContext); }

static intptr_t RunFunction(EA::Thread::RunnableFunction runnableFunction, void* pContext)
    { return EA::Debug::GetDefaultExceptionHandler()->RunTrapped(runnableFunction, pContext); }


struct EmptyThreadClass : public EA::Thread::IRunnable
    { intptr_t Run(void* pContext){ return reinterpret_cast<intptr_t>(pContext); } };

static intptr_t RunClass(EA::Thread::IRunnable* pRunnableClass, void* pContext)
    { return EA::Debug::GetDefaultExceptionHandler()->RunTrapped(pRunnableClass, pContext); }


///////////////////////////////////////////////////////////////////////////////
// TestMisc
//
static int TestMisc()
{
    using namespace EA::Debug;

    int  nErrorCount = 0;

    { 
        // Test EAThread-based RunTrapped functions. We don't actually generate exceptions in this test.


        // virtual intptr_t RunTrapped(EA::Thread::RunnableFunction runnableFunction, void* context);
        ExceptionHandler eh;
        intptr_t         testValue = 37;
        intptr_t         resultValue;
        EA::Debug::SetDefaultExceptionHandler(&eh); // We set this here so that RunFunction and RunClass above can grab it via GetDafaultExceptionHandler.

        EA::Thread::Thread thread;
        EA::Thread::Thread::SetGlobalRunnableFunctionUserWrapper(RunFunction);
        resultValue = 0;
        thread.Begin(EmptyThreadFunction, reinterpret_cast<void*>(testValue), NULL);
        thread.WaitForEnd(EA::Thread::kTimeoutNone, &resultValue);
        EATEST_VERIFY(resultValue == testValue); // Verify that the thread function returned the context value that we passed to it.


        // virtual intptr_t RunTrapped(EA::Thread::IRunnable* runnableClass, void* context);
        EmptyThreadClass threadClass;

        EA::Debug::SetDefaultExceptionHandler(&eh); // We set this here so that our RunThread and RunClass functions above can grab it via GetDafaultExceptionHandler.
        EA::Thread::Thread::SetGlobalRunnableClassUserWrapper(RunClass);
        resultValue = 0;
        thread.Begin(&threadClass, reinterpret_cast<void*>(testValue), NULL);
        thread.WaitForEnd(EA::Thread::kTimeoutNone, &resultValue);
        EATEST_VERIFY(resultValue == testValue); // Verify that the thread function returned the context value that we passed to it.
    }

    return nErrorCount;
}



///////////////////////////////////////////////////////////////////////////////
// TestCallstackReporting
//
// Tests the interception of exceptions and writing a callstack to the display
// as a result.
//
static int TestCallstackReporting()
{
    using namespace EA::Debug;

    int  nErrorCount = 0;
    bool result;

    // Set up the ExceptionHandler.
    ExceptionHandler eh;

    // Register a callback client. You don't need to do this, but we do it to
    // exercise the functionality.
    eh.RegisterClient(&gExceptionHandlerClientTest, true);

    // Set the exception handler to continue execution after the RunTrapped
    // calls below. This is as opposed to using kActionTerminate, which will
    // terminate the application, and as opposed to using kActionThrow, which 
    // rethrows the exception for some possible high level handler to catch.
    
    #if defined(EA_PLATFORM_WINDOWS) // We use vectored for Windows in order to test it as a new feature. It's not required.
        eh.SetMode(ExceptionHandler::kModeVectored);
    #else
        eh.SetMode(ExceptionHandler::kModeDefault); // This isn't actually necessary, but we put it here for instructive purposes.
    #endif

    eh.SetEnabled(true);
    eh.SetAction(ExceptionHandler::kActionDefault);
    eh.EnableReportTypes(ExceptionHandler::kReportTypeException);
    eh.EnableReportFields(ExceptionHandler::kReportFieldAll);
    eh.SetBuildDescription("TestCallstackReporting");
    eh.GetReportWriter()->SetOption(ReportWriter::kOptionReportUserName, 1);
    eh.GetReportWriter()->SetOption(ReportWriter::kOptionReportComputerName, 1);
    eh.GetReportWriter()->SetOption(ReportWriter::kOptionReportProcessList, 1);

    // On some platforms (e.g. Playstation 3) we cannot recover from exceptions
    // and so the first code that generates an exception below will not return.

    if(eh.IsEnabled())
    {
        // Test manual callstack generation
        result = eh.RunTrapped(TestAccessViolation, NULL); // This won't return if EA_EXCEPTIONHANDLER_RECOVERY_SUPPORTED is 0.

        #if EA_EXCEPTIONHANDLER_RECOVERY_SUPPORTED
            const EA::Callstack::Context* pContext = eh.GetExceptionContext();
            EATEST_VERIFY(result && (pContext != NULL));
            // To do: Test pContext.
        #endif

        // Test a custom ReportWriter
        CustomReportWriter customReportWriter;
        eh.SetReportWriter(&customReportWriter);
        result = eh.RunTrapped(TestAccessViolation, NULL);
        EATEST_VERIFY(result);
        EATEST_VERIFY(*customReportWriter.GetCallstackText() != 0);
    }

    return nErrorCount;
}



///////////////////////////////////////////////////////////////////////////////
// TestReportWriting
//
// Tests the interception of exceptions and generating a report of the exception.
//
static int TestReportWriting()
{
    using namespace EA::Debug;

    int  nErrorCount = 0;
    bool result;

    // Set up the ExceptionHandler.
    ExceptionHandler eh;

    // Register a callback client. You don't need to do this, but we do it to
    // exercise the functionality.
    eh.RegisterClient(&gExceptionHandlerClientTest, true);

    // Set the exception handler to continue execution after the RunTrapped
    // calls below. This is as opposed to using kActionTerminate, which will
    // terminate the application, and as opposed to using kActionThrow, which 
    // rethrows the exception for some possible high level handler to catch.

    #if defined(EA_PLATFORM_WINDOWS) // We use vectored for Windows in order to test it as a new feature. It's not required.
        eh.SetMode(ExceptionHandler::kModeVectored);
    #else
        eh.SetMode(ExceptionHandler::kModeDefault); // This isn't actually necessary, but we put it here for instructive purposes.
    #endif

    eh.SetEnabled(true);
    eh.SetAction(ExceptionHandler::kActionDefault);
    eh.SetBuildDescription("TestReportWriting");
    eh.GetReportWriter()->SetOption(ReportWriter::kOptionReportUserName, 1);
    eh.GetReportWriter()->SetOption(ReportWriter::kOptionReportComputerName, 1);
    eh.GetReportWriter()->SetOption(ReportWriter::kOptionReportProcessList, 1);

    // On some platforms (e.g. Playstation 3) we cannot recover from exceptions
    // and so the first code that generates an exception below will not return.

    if(eh.IsEnabled())
    {
        result = eh.RunTrapped(TestAccessViolation, NULL);
        EATEST_VERIFY(result);

        result = eh.RunTrapped(TestIllegalInstruction, NULL);
        EATEST_VERIFY(result);

        result = eh.RunTrapped(TestDivideByZero, NULL);
        EATEST_VERIFY(result);

        // The debugger always forces an app termination when a stack 
        // overflow occurs,so we skip this test if running under a debugger.

        // The following is disabled for now for Windows as there's a bug in Exceptionhandler when 
        // it comes to using minidump for stack overflows. MiniDump will fail in win32 dlls because 
        // the stack is corrupted. We need to allocate a new stack pointer and work from there.
        #if !defined(EA_COMPILER_MSVC)
            if(!EA::Debug::ExceptionHandler::IsDebuggerPresent())
            {
                result = eh.RunTrapped(TestStackOverflow, NULL);
                EATEST_VERIFY(result);
            }
        #endif

        // Can't run this, as it currently destroys the stack such that we can't run any more.
        // result = eh.RunTrapped(TestStackCorruption, NULL);
        // EATEST_VERIFY(result);

        // This time we run it and terminate the application upon an exception.
        if(!EA::Debug::ExceptionHandler::IsDebuggerPresent())
        {
            eh.SetAction(EA::Debug::ExceptionHandler::kActionTerminate, nErrorCount);
            eh.RunTrapped(TestAccessViolation, NULL);
        }
    }

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// GenericHelperThreadFunction
//
struct HelperThreadControl
{
    bool mbShouldContinue;
    EA::Debug::ExceptionType mExceptionToGenerate;
    intptr_t mReturnValue;
    
    HelperThreadControl() : mbShouldContinue(true), mExceptionToGenerate(EA::Debug::kExceptionTypeNone), mReturnValue(0){}
};

static intptr_t GenericHelperThreadFunction(void* pContext)
{
    volatile HelperThreadControl* pMessage = reinterpret_cast<volatile HelperThreadControl*>(pContext);
    
    while(pMessage->mbShouldContinue && (pMessage->mExceptionToGenerate == EA::Debug::kExceptionTypeNone))
    {
        if(pMessage->mExceptionToGenerate)
            EA::Debug::CreateException(pMessage->mExceptionToGenerate);
        
        EA::Thread::ThreadSleep(200);
    }
        
    return 0;
}


///////////////////////////////////////////////////////////////////////////////
// EAMain
//
int EAMain(int argc, char** argv)
{
    int nErrorCount = 0;

    EA::EAMain::PlatformStartup();

    #if (EASTDC_VERSION_N >= 11002)
        if(argc > 0) // Not all platforms and runtime libraries guarantee that argv[0] is present or that it points to the app path.
            EA::StdC::SetCurrentProcessPath(argv[0]);
    #endif

    #if defined(EA_PLATFORM_PS3)
        sys_net_initialize_parameter_t param;
        param.memory      = malloc(128 * 1024);
        param.memory_size = 128 * 1024;
        param.flags       = 0;
        sys_net_initialize_network_ex(&param);
    #endif

    EA::IO::InitializeFileSystem(true);
    EA::Thread::SetAllocator(EA::Allocator::ICoreAllocator::GetDefaultAllocator());

    EA::EAMain::CommandLine commandLine(argc, argv);
    const char *argValue;

    // Set up the AddressRepCache
    wchar_t dbPath[EA::IO::kMaxPathLength] = { 0 };

    #if EASTDC_VERSION_N >= 10400
        EA::StdC::GetCurrentProcessPath(dbPath);
    #endif

    #if defined(EA_COMPILER_MSVC)
        EA::StdC::Strcpy(EA::IO::Path::GetFileExtension(dbPath), L".pdb");
        gAddressRepCache.AddDatabaseFile(dbPath);

        EA::StdC::Strcpy(EA::IO::Path::GetFileExtension(dbPath), L".map");
    #endif

    // We create an arbitrary other thread for the purpose of showing up in our thread reports and 
    // so we can have it (rather than the main thread) generate exceptions for some tests.
    HelperThreadControl threadControl;
    EA::Thread::Thread  helperThread;
    const int           kHelperThreadCount = 1;

    for(int i = 0; i < kHelperThreadCount; i++)
    {
        helperThread.Begin(GenericHelperThreadFunction, &threadControl);
        helperThread.SetName("GenericHelperThread");
        EA::Thread::ThreadSleep(50);
    }
    
    // Setup default values for bRunTests, bRunSimulation, bRunExceptions.
    #if EA_EXCEPTIONHANDLER_HANDLING_SUPPORTED   // If the ExceptionHandler class is supported by the given platform...
        bool bRunTests      = true;
        bool bRunSimulation = true;
      #if EA_EXCEPTIONHANDLER_RECOVERY_SUPPORTED // If the app can continue executing after handling an exception...
        bool bRunExceptions = true;
      #else
        bool bRunExceptions = (EA::UnitTest::IsDebuggerPresent() && (commandLine.FindSwitch("-e") >= 0));
      #endif
    #else
        bool bRunTests      = false;
        bool bRunSimulation = false;
        bool bRunExceptions = false;
    #endif

    // Possibly override values for bRunTests, bRunSimulation, bRunExceptions via the command line.
    // Example usage: ExceptionHandlerTest.exe -RunTests:yes
    if(commandLine.FindSwitch("-RunTests", false, &argValue) >= 0)
        bRunTests = EA::StdC::Strcmp(argValue, "yes") == 0;
    if(commandLine.FindSwitch("-RunSimulation", false, &argValue) >= 0)
        bRunSimulation = EA::StdC::Strcmp(argValue, "yes") == 0;
    if(commandLine.FindSwitch("-RunExceptions", false, &argValue) >= 0)
        bRunExceptions = bRunTests = EA::StdC::Strcmp(argValue, "yes") == 0;

    {
        EA::UnitTest::TestApplication testSuite("ExceptionHandler Unit Tests", argc, argv);

        if(bRunTests)
        {
            if(bRunSimulation)
                testSuite.AddTest("ExceptionSimulationHandling", TestExceptionSimulationHandling);

            if(bRunExceptions)
            {
                testSuite.AddTest("Misc",               TestMisc);
                testSuite.AddTest("CallstackReporting", TestCallstackReporting);
                testSuite.AddTest("ReportWriting",      TestReportWriting);
            }
        }
        else
        {
            // Make compiler disuse warnings go away.
            EA::StdC::Sprintf(dbPath, L"%p %p %p", TestExceptionSimulationHandling, TestCallstackReporting, TestReportWriting);
        }

        nErrorCount = testSuite.Run();
    }

    threadControl.mbShouldContinue = false;
    helperThread.WaitForEnd(EA::Thread::ConvertRelativeTime(EA::Thread::ThreadTime(5000)), &threadControl.mReturnValue);

    EA::EAMain::PlatformShutdown(nErrorCount);

    return nErrorCount;
}






