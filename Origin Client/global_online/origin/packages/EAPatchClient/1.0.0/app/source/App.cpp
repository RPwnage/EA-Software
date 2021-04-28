/////////////////////////////////////////////////////////////////////////////
// EAPatchClient/App.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// Your app won't need all these headers in order to use EAPatch. Many of 
// these are specific to this app and its framework. See <EAPatchClientApp/BuildPatch.h>
// to see what you really need to include to use EAPatch.
/////////////////////////////////////////////////////////////////////////////

#include <EAPatchClient/Client.h>
#include <EAPatchClientApp/BuildPatch.h>
#include <UTFSockets/Manager.h>
#include <UTFSockets/Allocator.h>
#include <UTFInternet/Allocator.h>
#include <UTFInternet/HTTPServer.h>
#include <EASTL/string.h>
#include <EAStdC/EAString.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileDirectory.h>
#include <EAIO/Allocator.h>
#include <EATest/EATest.h>
#include <EAMain/EAEntryPointMain.inl>
#include <MemoryMan/MemoryMan.h>
#include <MemoryMan/MemoryMan.inl>
#include <MemoryMan/CoreAllocator.inl>
#include <stdio.h>
#include <netconn.h> // DirtySDK
#if defined(EA_PLATFORM_PS3)
    #include <cell/sysmodule.h>
#elif defined(EA_PLATFORM_WINDOWS)
    #include <conio.h>
    #include <Windows.h>
#endif

// Prevents false positive memory leaks on GCC platforms (such as PS3)
#if defined(EA_COMPILER_GNUC)
    #define EA_MEMORY_GCC_USE_FINALIZE
#endif



///////////////////////////////////////////////////////////////////////////////
// EASTL requirements
// Needed by any app using EAPatchClient. Most EA Apps have this already.
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
// Needed by any app using EAPatchClient. Most EA Apps have this already.
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
EA::Patch::String gDestDir;
EA::Patch::String gLocalDir;
EA::Patch::String gInfoURL;
EA::Patch::String gInfoPath;
EA::Patch::String gImplURL;
bool              gCancel;

bool gbTraceDirectoryRetrieverEvents    = false;
bool gbTraceDirectoryRetrieverTelemetry = false;
bool gbTracePatchBuilderEvents          = false;
bool gbTracePatchBuilderTelemetry       = false;


///////////////////////////////////////////////////////////////////////////////
// ShouldStop
//
bool ShouldStop()
{
    #if defined(EA_PLATFORM_WINDOWS)
        return (_kbhit() && (_getch() == 27)); // 27 is 'esc' key.
    #else
        return false;
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// IsConsoleNew
//
// Returns true if this process was started in a new console view. This is useful to know 
// because it usually means the app was started by double-clicking an app icon and will
// go away without the user being able to see any output unless we intentionally wait on exit.
// This function must be called right away on process startup before anything is printed to the screen.
//
static bool IsConsoleNew()
{
    #if defined(EA_PLATFORM_WINDOWS)
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

        if(hConsole != NULL)
        {
            CONSOLE_SCREEN_BUFFER_INFO csbi;

            if(GetConsoleScreenBufferInfo(hConsole, &csbi))
            {
                // If the cursor position is at [0,0], we assume the console
                // is newly created and hasn't been written to. Strictly speaking,
                // this isn't necessarily so, but most often it will be so.
                return ((csbi.dwCursorPosition.X == 0) && 
                        (csbi.dwCursorPosition.Y == 0));
            }
        }
    #endif

    return false;
}


///////////////////////////////////////////////////////////////////////////////
// PrintUsage
//
static void PrintUsage()
{
    static bool bUsageDisplayed = false;

    if(!bUsageDisplayed)
    {
        bUsageDisplayed = true;

        printf("%s\n", "EAPatchClientApp");
        printf("%s\n", "By Paul Pedriana");
        printf("%s\n", "");
        printf("%s\n", "This utility demonstrates the building of a patch via EAPatch");
        printf("%s\n", "");
        printf("%s\n", "Arguments (order doesn't matter):");
        printf("%s\n", "   -destDir:<URL>           Path to the directory where patch is written. Required");
        printf("%s\n", "   -localDir:<URL>          Path to the directory where data to patch is. Optional");
        printf("%s\n", "");
        printf("%s\n", "   -infoURL:<URL>           URL to a .eaPatchInfo file. One of these four is required.");
        printf("%s\n", "   -infoPath:<FilePath>     File path to a .eaPatchInfo file. One of these four is required.");
        printf("%s\n", "   -implURL:<URL>           URL to a .eaPatchImpl file. One of these four is required.");
        printf("%s\n", "   -cancel                  Specifies to cancel the (presumably partial) patch in destDir. One of these four is required.");
        printf("%s\n", "");
        printf("%s\n", "   -tdre                    Trace directory retrieval events. Optional, off by default.");
        printf("%s\n", "   -tdrt                    Trace directory retrieval telemetry. Optional, off by default.");
        printf("%s\n", "   -tpbe                    Trace patch builder events. Optional, off by default.");
        printf("%s\n", "   -tpbt                    Trace patch builder telemetry. Optional, off by default.");
        printf("%s\n", "");
        printf("%s\n", "   -?                       Displays this help.");
        printf("%s\n", "");
        printf("%s\n", "If LocalDir specified then it's assumed that LocalDir will be patched");
        printf("%s\n", "but the results will be written to DestDir.");
        printf("%s\n", "");
        printf("%s\n", "One of InfoURL, InfoPath, ImplURL, or cancel must be specified.");
        printf("%s\n", "These are ways of identifying the source of the patch or to cancel a partial patch.");
        printf("%s\n", "Cancelling a patch causes the destination directory to be rolled back to its original state.");
        printf("%s\n", "");
        printf("%s\n", "You need to make sure you are running a standard web server at the address of the patch URL.");
        printf("%s\n", "You can stop a patch build mid-progress by pressing the escape key. You can resume it by");
        printf("%s\n", "simply starting it again, upon which it will resume where it left off. You can cancel the ");
        printf("%s\n", "patch and roll it back by running this app with -cancel after stopping it.");
        printf("%s\n", "This app cannot be run multiple times simultaneously on the same destination directory.");
        printf("%s\n", "");
        printf("%s\n", "Example usage:");
        printf("%s\n", "    EAPatchClientApp.exe -DestDir\":E:\\BaseBall 2015\\Data\" -ImplURL:http://112.54.33.103/Baseball 2015/Patch 3.eaPatchImpl");
        printf("%s\n", "    EAPatchClientApp.exe -tpbe -DestDir\":E:\\AppCustomDataDir\" -LocalDir\":E:\\BaseBall 2015\\Data\" -InfoURL:http://112.54.33.103/Patch 3.eaPatchInfo");
    }
}


///////////////////////////////////////////////////////////////////////////////
// ReadCommandLine
//
static void ReadCommandLine(int argc, char** argv)
{
    EA::EAMain::CommandLine commandLine(argc, argv);
    EA::EAMain::CommandLine::FixedString sResult;

    if(commandLine.FindSwitch("-destDir", false, &sResult) >= 0)
        gDestDir = sResult.c_str();

    if(commandLine.FindSwitch("-localDir", false, &sResult) >= 0)
        gLocalDir = sResult.c_str();

    if(commandLine.FindSwitch("-infoURL", false, &sResult) >= 0)
        gInfoURL = sResult.c_str();

    if(commandLine.FindSwitch("-infoPath", false, &sResult) >= 0)
        gInfoPath = sResult.c_str();

    if(commandLine.FindSwitch("-implURL", false, &sResult) >= 0)
        gImplURL = sResult.c_str();

    if(commandLine.FindSwitch("-cancel", false, NULL) >= 0)
        gCancel = true;

    // e.g. App.exe -tdre
    gbTraceDirectoryRetrieverEvents    = (commandLine.FindSwitch("-tdre", false, NULL) >= 0);
    gbTraceDirectoryRetrieverTelemetry = (commandLine.FindSwitch("-tdrt", false, NULL) >= 0);
    gbTracePatchBuilderEvents          = (commandLine.FindSwitch("-tpbe", false, NULL) >= 0);
    gbTracePatchBuilderTelemetry       = (commandLine.FindSwitch("-tpbt", false, NULL) >= 0);

    // e.g. App.exe -?
    if((commandLine.Argc() <= 1) || (commandLine.FindSwitch("-?", false, NULL) >= 0) || (commandLine.FindSwitch("-h", false, NULL) >= 0))
        PrintUsage();

    if(gInfoURL.empty() && gInfoPath.empty() && gImplURL.empty())
        PrintUsage();
}


///////////////////////////////////////////////////////////////////////////////
// SetupApp
//
static void SetupApp()
{
    #if defined(EA_PLATFORM_PS3)
        cellSysmoduleLoadModule(CELL_SYSMODULE_RTC);
        cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL);
        cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP);
        cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_GAME);
        cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_USERINFO);
        cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_SAVEDATA);
        cellSysmoduleLoadModule(CELL_SYSMODULE_NETCTL);
        cellSysmoduleLoadModule(CELL_SYSMODULE_NET);
        cellNetCtlInit();
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// SetupHeap
//
static void SetupHeap()
{
    ///////////////////////////////////////////////////////////////////////////////
    // Optional heap debug setup.
    ///////////////////////////////////////////////////////////////////////////////
    #if 0 // defined(EA_PLATFORM_DESKTOP) // If we have lots of CPU power...
        #if EA_MEMORY_DEBUG_ENABLED
            if(EA::Allocator::gpEAGeneralAllocatorDebug)
                EA::Allocator::gpEAGeneralAllocatorDebug->SetOption(EA::Allocator::GeneralAllocatorDebug::kOptionEnablePtrValidation, 1);
        #endif

        EA::Allocator::gpEAGeneralAllocator->SetAutoHeapValidation(EA::Allocator::GeneralAllocator::kHeapValidationLevelBasic, 1);
    #endif
}


///////////////////////////////////////////////////////////////////////////////
// SetupNetwork
//
static void SetupNetwork()
{
    ///////////////////////////////////////////////////////////////////////////////
    // UTFSockets setup
    // Needed only if you are embedding UTFInternet's HTTPServer. 
    // Not needed by EAPatchClient.
    ///////////////////////////////////////////////////////////////////////////////
    EA::Sockets::SocketSystemInit();

    EA::Sockets::Manager* pSocketManager = new EA::Sockets::Manager;
    pSocketManager->Init();
    EA::Sockets::SetManager(pSocketManager);

    #if ((EA_PLATFORM_UNIX) || defined(EA_PLATFORM_APPLE) || defined(EA_HAVE_SIGNAL_H)) && defined(SIGPIPE)
        signal(SIGPIPE, SIG_IGN);   // Ignore pipe signals, as they can occur with sockets on some Unixes and cause us to abort.
    #endif

    ///////////////////////////////////////////////////////////////////////////////
    // DirtySDK setup
    // Needed by any app using EAPatchClient. Most EA Apps have this already.
    ///////////////////////////////////////////////////////////////////////////////
    NetConnStartup("");
    #if defined(EA_PLATFORM_XENON)
        NetConnControl('xlsp', 0, 0, NULL, NULL); // enable LSP service
    #elif defined(EA_PLATFORM_PS3)
        cellSysmoduleLoadModule(CELL_SYSMODULE_SYSUTIL_NP);
    #endif
    NetConnConnect(NULL, NULL, 0);
}


///////////////////////////////////////////////////////////////////////////////
// ShutdownNetwork
//
static void ShutdownNetwork()
{
    ///////////////////////////////////////////////////////////////////////////////
    // DirtySDK shutdown
    // Needed by any app using EAPatchClient. Most EA Apps have this already.
    ///////////////////////////////////////////////////////////////////////////////
    NetConnDisconnect();
    NetConnShutdown(0);

    ///////////////////////////////////////////////////////////////////////////////
    // UTFSockets shutdown
    // Needed only if you are embedding UTFInternet's HTTPServer. 
    // Not needed by EAPatchClient.
    ///////////////////////////////////////////////////////////////////////////////
    EA::Sockets::Manager* pSocketManager = EA::Sockets::GetManager();
    EA::Sockets::SetManager(NULL);
    pSocketManager->Shutdown();
    delete pSocketManager;

    EA::Sockets::SocketSystemShutdown();
}



///////////////////////////////////////////////////////////////////////////////
// main
//
int EAMain(int argc, char** argv)
{
    bool bWaitAtEnd = IsConsoleNew();

    // Setup
    SetupApp();
    SetupHeap();
    SetupNetwork();
    ReadCommandLine(argc, argv);

    // Run
    if(gInfoURL.size())
        BuildPatchFromInfoURL(gInfoURL.c_str(), gDestDir.c_str(), gLocalDir.c_str());
    else if(gInfoPath.size())
        BuildPatchFromInfoFile(gInfoPath.c_str(), gDestDir.c_str(), gLocalDir.c_str());
    else if(gImplURL.size())
        BuildPatchFromImplURL(gImplURL.c_str(), gDestDir.c_str(), gLocalDir.c_str());
    else if(gCancel)
        CancelPatch(gDestDir.c_str());
    else
        printf("No patch source or cancellation was specified.\n");

    // Shutdown
    ShutdownNetwork();

    if(bWaitAtEnd)
    {
        printf("Press any key to exit.\n");
        getchar();
    }

    return 0;
}










