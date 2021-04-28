/////////////////////////////////////////////////////////////////////////////
// EAPatchClient/Test.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchClientTest/Test.h>
#include <EAPatchClient/Client.h>
#include <UTFSockets/Manager.h>
#include <UTFSockets/Allocator.h>
#include <UTFInternet/Allocator.h>
#include <UTFInternet/HTTPServer.h>
#include <EASTL/algorithm.h>
#include <EASTL/string.h>
#include <EAStdC/EAString.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileDirectory.h>
#include <EAIO/Allocator.h>
#include <EATest/EATest.h>
#include <EATest/EAMissingImpl.inl>
#include <EAMain/EAEntryPointMain.inl>
#include <ExceptionHandler/ExceptionHandler.h>
#include <netconn.h>

//Prevents false positive memory leaks on GCC platforms (such as PS3)
#if defined(EA_COMPILER_GNUC)
    #define EA_MEMORY_GCC_USE_FINALIZE
#endif
#if defined(EA_PLATFORM_PS3)
    #include <cell/sysmodule.h>
#endif
#include <MemoryMan/MemoryMan.h>
#include <MemoryMan/MemoryMan.inl>
#include <MemoryMan/CoreAllocator.inl>



///////////////////////////////////////////////////////////////////////////////
// EASTL requirements
///////////////////////////////////////////////////////////////////////////////

#include <EAStdC/EASprintf.h>

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

#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
    int VsnprintfW(wchar_t* pDestination, size_t n, const wchar_t* pFormat, va_list arguments)
    {
        return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments);
    }
#endif



///////////////////////////////////////////////////////////////////////////////
// DirtySDK requirements
///////////////////////////////////////////////////////////////////////////////

extern "C" void* DirtyMemAlloc(int32_t iSize, int32_t /*iMemModule*/, int32_t /*iMemGroup*/)
{
    EA::Allocator::ICoreAllocator* pAllocator = EA::Patch::GetAllocator();
    EAPATCH_ASSERT(pAllocator);

    return pAllocator->Alloc(static_cast<size_t>(iSize), "DirtySDK", 0);
}

extern "C" void DirtyMemFree(void* p, int32_t /*iMemModule*/, int32_t /*iMemGroup*/)
{
    EA::Allocator::ICoreAllocator* pAllocator = EA::Patch::GetAllocator();
    EAPATCH_ASSERT(pAllocator);

    return pAllocator->Free(p); 
}


///////////////////////////////////////////////////////////////////////////////
// Globals
//
EA::IO::Path::PathStringW gDataDirectory;
EA::IO::Path::PathStringW gTempDirectory;

bool gbTraceDirectoryRetrieverEvents    = false;
bool gbTraceDirectoryRetrieverTelemetry = false;
bool gbTracePatchBuilderEvents          = false;
bool gbTracePatchBuilderTelemetry       = false;

EA::Internet::HTTPServer::Throttle gHTTPServerThrottle;

EA::Patch::String gExternalHTTPServerAddress; // If empty, then it's not used and instead we just use our internal HTTP server at 127.0.0.1 (our address).
uint16_t          gExternalHTTPServerPort = 80;


///////////////////////////////////////////////////////////////////////////////
// SetupData
//
static void SetupData(int argc, char** argv)
{
    using namespace EA::IO;

    EA::EAMain::CommandLine              commandLine(argc, argv);
    EA::EAMain::CommandLine::FixedString sResult;

    #if defined(EA_PLATFORM_PS3) && (EAIO_VERSION_N >= 21702)
        EA::IO::SetPS3GameDataNames("EAPatch Client Test", "EAPCT-00000", "1.0", "EAPCT00000GameData");
    #endif

    // Example: Text.exe -d:"C:\Dir\my data"
    if(commandLine.FindSwitch("-d", false, &sResult) >= 0)
    {
        // Convert from char8_t to wchar_t.
        int nRequiredStrlen = EA::StdC::Strlcpy((wchar_t*)NULL, sResult.c_str(), 0, sResult.length());
        gDataDirectory.resize((eastl_size_t)nRequiredStrlen);
        EA::StdC::Strlcpy(&gDataDirectory[0], sResult.c_str(), gDataDirectory.capacity(), sResult.length());
        EA::IO::Path::EnsureTrailingSeparator(gDataDirectory);
    }
    else
    {
        gDataDirectory.resize(EA::IO::kMaxDirectoryLength);
        gDataDirectory.resize((eastl_size_t)EA::IO::Directory::GetCurrentWorkingDirectory(&gDataDirectory[0]));
    }

    // Find all directory paths.
    EA::IO::Path::PathStringW dataDirReadMeFilePath = gDataDirectory;
    dataDirReadMeFilePath += L"ReadMe.txt";
    
    if(!EA::IO::File::Exists(dataDirReadMeFilePath.c_str()))
    {
        EA::UnitTest::Report("No patch directories were found.\n");
        EA::UnitTest::Report("You need to run with the working directory set to the full path to EAPatchClient/dev/test/data or\n");
        EA::UnitTest::Report("you need to run with the -d:<path> command line argument\n");
        EA::UnitTest::Report("The specified data directory is \"%ls\".\n", gDataDirectory.c_str());
        gDataDirectory.clear();
    }

    // Set up the temp directory path we use.
    gTempDirectory.resize(EA::IO::kMaxDirectoryLength);
    gTempDirectory.resize((eastl_size_t)EA::IO::GetTempDirectory(&gTempDirectory[0]));
    gTempDirectory += L"EAPatchClientTest";
    gTempDirectory += EA_FILE_PATH_SEPARATOR_STRING_W;
}


static void ReadCommandLine(int argc, char** argv)
{
    EA::EAMain::CommandLine              commandLine(argc, argv);
    EA::EAMain::CommandLine::FixedString sResult;

    // e.g. Test.exe -tdre
    gbTraceDirectoryRetrieverEvents    = (commandLine.FindSwitch("-tdre", false, NULL) >= 0);
    gbTraceDirectoryRetrieverTelemetry = (commandLine.FindSwitch("-tdrt", false, NULL) >= 0);
    gbTracePatchBuilderEvents          = (commandLine.FindSwitch("-tpbe", false, NULL) >= 0);
    gbTracePatchBuilderTelemetry       = (commandLine.FindSwitch("-tpbt", false, NULL) >= 0);

    // e.g. Test.exe -HTTPConnectionLatency:2000
    if(commandLine.FindSwitch("-HTTPConnectionLatencyNs", false, &sResult, 0) >= 0) // This is nanoseconds.
        gHTTPServerThrottle.mConnectionLatencyNs = EA::StdC::StrtoU64(sResult.c_str(), NULL, 0);

    if(commandLine.FindSwitch("-HTTPRequestLatencyNs", false, &sResult, 0) >= 0)
        gHTTPServerThrottle.mRequestLatencyNs = EA::StdC::StrtoU64(sResult.c_str(), NULL, 0);

    if(commandLine.FindSwitch("-HTTPReadBytesPerSecond", false, &sResult, 0) >= 0)
        gHTTPServerThrottle.mReadBytesPerSecond = EA::StdC::StrtoU64(sResult.c_str(), NULL, 0);

    if(commandLine.FindSwitch("-HTTPWriteBytesPerSecond", false, &sResult, 0) >= 0)
        gHTTPServerThrottle.mWriteBytesPerSecond = EA::StdC::StrtoU64(sResult.c_str(), NULL, 0);

    // e.g. -HTTPServer:122.14.22.113
    // e.g. -HTTPServer:122.14.22.113:1234
    // When using this option, you need to run a web server at the given address and have its root directory
    // be the EAPatchClient/test/data directory.
    if(commandLine.FindSwitch("-HTTPServer", false, &sResult, 0) >= 0)
    {
        gExternalHTTPServerAddress.assign(sResult.c_str());

        eastl_size_t colonPos = gExternalHTTPServerAddress.find(':');

        if(colonPos != EA::Patch::String::npos)
        {
            gExternalHTTPServerPort = (uint16_t)EA::StdC::StrtoU32(&gExternalHTTPServerAddress[colonPos + 1], NULL, 0);
            gExternalHTTPServerAddress.resize(colonPos);
        }
    }
}


#if (EXCEPTIONHANDLER_VERSION_N >= 11100)
    ///////////////////////////////////////////////////////////////////////////////
    // ExceptionHandlerNotificationSink
    //
    // If we get an exception during processing, we echo its report to the unit test
    // output so it shows up in our execution report.
    //
    class ExceptionHandlerNotificationSink : public EA::Debug::ExceptionHandlerClient
    {
    public:
        void Notify(EA::Debug::ExceptionHandler* pHandler, EA::Debug::ExceptionHandler::EventId id)
        {
            if(id == EA::Debug::ExceptionHandler::kExceptionHandlingEnd)
            {
                EA::Debug::ReportWriter* pReportWriter = pHandler->GetReportWriter();

                if(pReportWriter->ReadReportBegin())
                {
                    char    buffer[256 + 1]; // +1 for the 0 we append below.
                    ssize_t size = 0;

                    EA::UnitTest::Report("**********************\n");
                    EA::UnitTest::Report("Exception report begin\n");

                    // We may need to revise this to write to a temp string first, as some platforms
                    // unilaterally put a newline after each debug output statement.
                    while((size = (ssize_t)pReportWriter->ReadReportPart(buffer, EAArrayCount(buffer) - 1)) > 0)
                    {
                        buffer[size] = 0;
                        EA::UnitTest::Report("%s", buffer);
                    }

                    EA::UnitTest::Report("Exception report end\n");
                    EA::UnitTest::Report("********************\n");

                    pReportWriter->ReadReportEnd();
                }
            }
        }
    };

    ExceptionHandlerNotificationSink gExceptionHandlerNotificationSink;
#endif



///////////////////////////////////////////////////////////////////////////////
// main
//
int EAMain(int argc, char** argv)
{
    EA::EAMain::PlatformStartup();
    EA::Patch::SetAllocator(EA::Allocator::ICoreAllocator::GetDefaultAllocator());

    SetupData(argc, argv);
    ReadCommandLine(argc, argv);

    // Setup custom heap settings
    #if defined(EA_PLATFORM_DESKTOP) // If we have lots of CPU power...
        #if EA_MEMORY_DEBUG_ENABLED
            if(EA::Allocator::gpEAGeneralAllocatorDebug)
                EA::Allocator::gpEAGeneralAllocatorDebug->SetOption(EA::Allocator::GeneralAllocatorDebug::kOptionEnablePtrValidation, 1);
        #endif

        EA::Allocator::gpEAGeneralAllocator->SetAutoHeapValidation(EA::Allocator::GeneralAllocator::kHeapValidationLevelBasic, 1);
    #endif

    // Initialize the socket system
    EA::Sockets::SocketSystemInit();
    EA::Sockets::Manager socketManager;
    socketManager.Init();
    EA::Sockets::SetManager(&socketManager);
    #if ((EA_PLATFORM_UNIX) || defined(EA_PLATFORM_APPLE) || defined(EA_HAVE_SIGNAL_H)) && defined(SIGPIPE)
        signal(SIGPIPE, SIG_IGN);   // Ignore pipe signals, as they can occur with sockets on some Unixes and cause us to abort.
    #endif

    // DirtySDK socket setup
    NetConnStartup("");
    #if defined(EA_PLATFORM_XENON)
        NetConnControl('xlsp', 0, 0, NULL, NULL); // enable LSP service
    #elif defined(EA_PLATFORM_PS3)
        cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP);
    #endif
    NetConnConnect(NULL, NULL, 0);


    // Install a vectored exception handler, so we can catch exceptions and read the resulting exception report.
    #if EXCEPTIONHANDLER_VERSION_N >= 11100
        using namespace EA::Debug;

        ExceptionHandler eh;

        eh.RegisterClient(&gExceptionHandlerNotificationSink, true);
        #if defined(EA_PLATFORM_WINDOWS)                // The Windows default is kModeStackBased, but we want kModeVectored.
            eh.SetMode(ExceptionHandler::kModeVectored);
        #else
            eh.SetMode(ExceptionHandler::kModeDefault); // This isn't actually necessary, but we put it here for instructive purposes.
        #endif
        eh.SetEnabled(true);
        eh.SetAction(ExceptionHandler::kActionThrow);
        eh.EnableReportTypes(ExceptionHandler::kReportTypeException);
        eh.EnableReportFields(ExceptionHandler::kReportFieldHeader | ExceptionHandler::kReportFieldCallStack | ExceptionHandler::kReportFieldRegisters);
        eh.SetBuildDescription("EAPatchClient");
        eh.GetReportWriter()->SetOption(ReportWriter::kOptionReportUserName, 1);
        eh.GetReportWriter()->SetOption(ReportWriter::kOptionReportComputerName, 1);
        eh.GetReportWriter()->SetOption(ReportWriter::kOptionReportProcessList, 1);
    #endif

    // Add the tests
    EA::UnitTest::TestApplication testSuite("EAPatchClient Unit Tests", argc, argv);

    // Add all tests
    testSuite.AddTest("Basic",              TestBasic);
    testSuite.AddTest("File",               TestFile);
    testSuite.AddTest("Hash",               TestHash);
    testSuite.AddTest("XML",                TestXML);
    testSuite.AddTest("HTTP",               TestHTTP);
    testSuite.AddTest("Telemetry",          TestTelemetry);
    testSuite.AddTest("Storage",            TestStorage);
    testSuite.AddTest("PatchDirectory",     TestPatchDirectory);
    testSuite.AddTest("PatchBuilder",       TestPatchBuilder);
    testSuite.AddTest("PatchBuilderCancel", TestPatchBuilderCancel);

    // Run the prescribed tests.
    const int testResult = testSuite.Run();

    // Shutdown the socket system.
    EA::Sockets::SetManager(NULL);
    socketManager.Shutdown();
    EA::Sockets::SocketSystemShutdown();

    NetConnDisconnect();
    NetConnShutdown(0);
    EA::EAMain::PlatformShutdown(testResult);

    return testResult;
}










