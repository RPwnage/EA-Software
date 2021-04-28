///////////////////////////////////////////////////////////////////////////////
// EAPatchMakerApp/MakerApp.cpp
//
// Copyright (c) Electronic Arts. All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////


#include <EAPatchMaker/Version.h>
#include <EAPatchMaker/Report.h>
#include <EAPatchMaker/Maker.h>
#include <EAPatchMakerApp/MakerApp.h>
#include <EAPatchMakerApp/CommandLine.h>
#include <EAPatchClient/XML.h>
#include <EAIO/EAFileStream.h>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <MemoryMan/MemoryMan.inl>
#include <MemoryMan/CoreAllocator.inl>


///////////////////////////////////////////////////////////////////////////////
// EASTL requirements
///////////////////////////////////////////////////////////////////////////////

int Vsnprintf8(char8_t* pDestination, size_t n, const char8_t*  pFormat, va_list arguments)
    { return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments); }

int Vsnprintf16(char16_t* pDestination, size_t n, const char16_t* pFormat, va_list arguments)
    { return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments); }

int Vsnprintf32(char32_t* pDestination, size_t n, const char32_t* pFormat, va_list arguments)
    { return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments); }

#if defined(EA_WCHAR_UNIQUE) && EA_WCHAR_UNIQUE
int VsnprintfW(wchar_t* pDestination, size_t n, const wchar_t* pFormat, va_list arguments)
    { return EA::StdC::Vsnprintf(pDestination, n, pFormat, arguments); }
#endif


///////////////////////////////////////////////////////////////////////////////
// DirtySDK requirements
///////////////////////////////////////////////////////////////////////////////

#if defined(EA_PLATFORM_WINDOWS) && EA_WINAPI_FAMILY_PARTITION(EA_WINAPI_PARTITION_DESKTOP)
    #pragma comment(lib, "IPHLPAPI.lib") // DirtySDK requires linking in this Windows library
#endif

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


// Returns true if this process was started in a new console view. This is useful to know 
// because it usually means the app was started by double-clicking an app icon and will
// go away without the user being able to see any output unless we intentionally wait on exit.
// This function must be called right away on process startup before anything is printed to the screen.
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


static void PrintHelp()
{
    using namespace EA::Patch;

    static bool bHelpPrintedAlready = false;

    if(!bHelpPrintedAlready)
    {
        bHelpPrintedAlready = true;

        Report("*******************************************************************************\n");
        Report("EAPatchMaker\n");
        Report("Version %d.%d.%d\n", EAPATCHMAKER_VERSION_MAJOR, EAPATCHMAKER_VERSION_MINOR, EAPATCHMAKER_VERSION_PATCH);
        Report("Developed and maintained by EATech\n");
        Report("See the EAPatch documentation for more information about patch making, testing,\n");
        Report("and deploying. Or contact EATech Core Support by email for help and bug reports.\n");
        Report("\n");
        Report("A patch is built by supplying a minimal .eaPatchImpl XML file (described below) and \n");
        Report("a directory where the files for the patch are located. See the documentation for a \n");
        Report("description of the format, or see the Template.eaPatchImpl file for an example.\n");
        Report("The patch will consist of multiple generated files in addition to the input dir\n");
        Report("files, and a version of the supplied .eaPatchImpl with some additional fields added.\n");
        Report("If no .eaPatchImpl file is supplied or present in the inputDir directory, a default\n");
        Report("one will be generated that specifies to use all files in the directory recursively.\n");
        Report("\n");
        Report("Available arguments:\n");
        Report("    -help / -h / -?                     Displays this usage information\n");
        Report("    -v:<value> / -verbosity:<value>     Optional. Default is %u. 0 = error output only, 1 = low output, 2 = full output.\n", kReportVerbosityDefault);
        Report("                                                  If you want to see the status of each file as it's processed, use -v:2.\n");
        Report("    -wait / -w                          Waits for user confirmation before app exit.\n");
        Report("    -inputDir:<dir path>                Required. Sets the directory the patch is made from.\n");
        Report("                                                  Currently must not use Unicode characters on Windows.\n");
        Report("    -outputDir:<dir path>               Required. Sets the directory the patch is written to.\n");
        Report("                                                  Currently must not use Unicode characters on Windows.\n");
        Report("    -previousDir:<dir path>             Optional; required if -optimize is used.\n");
        Report("                                                  Specifies a directory with typical pre-patch contents.\n");
        Report("    -patchImplPath:<.eaPatchImpl path>  Optional; required if the .eaPatchImpl file isn't in the root of \n");
        Report("                                                  the inputDir and if you want a custom patch specification.\n");
        Report("                                                  Specifies the patch definition.\n");
        Report("    -validatePatchImpl:enabled|disabled Optional. Default is disabled. When enabled, the user is \n");
        Report("                                                  required to supply a valid .eaPatchImpl file.\n");
        Report("    -clearOutputDir:enabled|disabled    Optional. Default is enabled. When enabled, any contents of \n");
        Report("                                                  the output directory are deleted.\n");
        Report("    -errorCleanup:enabled|disabled      Optional. Default is enabled. When enabled, upon an error \n");
        Report("                                                  the output directory is deleted.\n");
        Report("    -optimize:enabled|disabled          Optional. Default is enabled, which may be slow. If you specify \n");
        Report("                                                  this then you must specify -previousDir.\n");
        Report("    -maxFileCount:<count>               Optional. Default value is %I64u. Used for sanity checking.\n", kMaxPatchFileCountDefault);
        Report("    -blockSize:<power-of-2 size>        Optional. Default value is 0 (unspecified). Allows you to force usage of a given block size.\n");
        Report("                                                  Ignored if -optimize is enabled. Min value is %u.\n", (unsigned)EA::Patch::kDiffBlockSizeMin);
        Report("\n");
        Report("Example usage:\n");
        Report("    EAPatchMaker.exe -inputDir:C:\\NewData -outputDir:C:\\Patch01 -patchImplPath:C:\\Patch01.eaPatchImpl\n");
        Report("    EAPatchMaker.exe -inputDir:C:\\GameApp\\NewData -outputDir:C:\\GameApp\\Patch01 -maxFileCount:2000000\n");
        Report("    EAPatchMaker.exe -previousDir:C:\\PrevData -inputDir:C:\\NewData -outputDir:C:\\PatchData -optimize:enabled\n");
        Report("\n");
        Report("Example usage for generating an Origin patch:\n");
        Report("    EAPatchMaker.exe -inputDir:C:\\TigerWoods2015 -outputDir:C:\\TigerWoods2015_SyncPackage -blockSize:65536\n");
        Report("*******************************************************************************\n");
    }
}


static bool ReadCommandLineArguments(int argc, char** argv, EA::Patch::MakerSettings& makerSettings, bool& bWaitAtEnd)
{
    bool bSuccess = true;
    EA::Patch::AppCommandLine cmdLine(argc, argv);
    EA::Patch::String s;        

    // Example usage: -inputDir:C:\GameApp\NewData\             //
    if((cmdLine.FindSwitch("-inputDir", false, &s, 0) >= 0))
    {
        makerSettings.mInputDirectory = s;
        // We verify the directory's existence later.
    }

    // Example usage: -outputDir:C:\Temp\Patch01\                //
    if((cmdLine.FindSwitch("-outputDir", false, &s, 0) >= 0))
    {
        makerSettings.mOutputDirectory = s;
        // We verify the directory's existence later.
    }

    // Example usage: -?
    if((cmdLine.FindSwitch("-help", false, NULL, 0) >= 0) ||
       (cmdLine.FindSwitch("-h",    false, NULL, 0) >= 0) ||
       (cmdLine.FindSwitch("-?",    false, NULL, 0) >= 0))
    {
        PrintHelp();
    }

    // Example usage: -verbosity:2
    if((cmdLine.FindSwitch("-verbosity", false, &s, 0) >= 0) || 
       (cmdLine.FindSwitch("-v",         false, &s, 0) >= 0))
    {
        unsigned reportVerbosity = EA::StdC::AtoU32(s.c_str());
        EA::Patch::SetReportVerbosity(reportVerbosity);
    }

    // Example usage: -wait
    if((cmdLine.FindSwitch("-wait", false, NULL, 0) >= 0) || 
       (cmdLine.FindSwitch("-w",    false, NULL, 0) >= 0))
    {
        bWaitAtEnd = true;
    }

    // Example usage: -previousDir:C:\GameApp\PrevData\          //
    if((cmdLine.FindSwitch("-previousDir", false, &s, 0) >= 0))
    {
        makerSettings.mPreviousDirectory = s;
        // We verify the directory's existence later.
    }

    // Example usage: -patchImplPath:C:\GameApp\Patch.eaPatchImpl
    if((cmdLine.FindSwitch("-patchImplPath", false, &s, 0) >= 0))
    {
        makerSettings.mPatchImplPath = s;
        // We verify the file's existence later.
    }

    // Example usage: -validatePatchImpl:enabled 
    if((cmdLine.FindSwitch("-validatePatchImpl", false, &s, 0) >= 0))
        makerSettings.mbValidatePatchImpl = (s.comparei("disabled") != 0);

    // Example usage: -clearOutputDir:disabled 
    if((cmdLine.FindSwitch("-clearOutputDir", false, &s, 0) >= 0))
        makerSettings.mbCleanOutputDirectory = (s.comparei("disabled") != 0);

    // Example usage: -optimize:disabled 
    if((cmdLine.FindSwitch("-optimize", false, &s, 0) >= 0))
        makerSettings.mbEnableOptimizations = (s.comparei("disabled") != 0);
    else
        makerSettings.mbEnableOptimizations = !makerSettings.mPreviousDirectory.empty();

    // Example usage: -errorCleanup:enabled 
    if((cmdLine.FindSwitch("-errorCleanup", false, &s, 0) >= 0))
        makerSettings.mbEnableErrorCleanup = (s.comparei("disabled") != 0);

    // Example usage: -maxFileCount:1000000
    if((cmdLine.FindSwitch("-maxFileCount", false, &s, 0) >= 0))
        makerSettings.mMaxFileCount = EA::StdC::AtoU64(s.c_str());

    // Example usage: -blockSize:65536
    if((cmdLine.FindSwitch("-blockSize", false, &s, 0) >= 0))
        makerSettings.mBlockSize = EA::StdC::AtoU32(s.c_str());

    if(makerSettings.mInputDirectory.empty() || makerSettings.mOutputDirectory.empty())
    {
        bSuccess = false;

        PrintHelp();

        if(makerSettings.mInputDirectory.empty() && makerSettings.mOutputDirectory.empty())
            EA::Patch::Report("The required -inputDir and -outputDir arguments are missing.\n");
        else if(makerSettings.mInputDirectory.empty())
            EA::Patch::Report("The required -inputDir argument is missing.\n");
        else if(makerSettings.mOutputDirectory.empty())
            EA::Patch::Report("The required -outputDir argument is missing.\n");
    }

    return bSuccess;
}


///////////////////////////////////////////////////////////////////////////////
// main
//
// To do (must do): Make this use wmain on Windows platforms, as Windows expects 
// you to use wchar_t for localization support, as Windows doesn't support UTF8.
// We'll have to convert the wchar_t-based argv to UTF8 char8_t, but we already 
// have code for that in EATest/EATestMain.inl which we can copy.
//
// We return 0 (success) or -1 (failure) from this process depending on the result.
//
int main(int argc, char** argv)
{
    using namespace EA::Patch;

    int           nErrorCount = 0;
    bool          bWaitAtEnd = IsConsoleNew();
    MakerSettings makerSettings;
    bool          bArgumentsOK = ReadCommandLineArguments(argc, argv, makerSettings, bWaitAtEnd);

    if(bArgumentsOK)
    {
        Maker maker;

        maker.MakePatch(makerSettings);

        if(maker.HasError())
        {
            nErrorCount++;
            Error error = maker.GetError();
            EA_UNUSED(error); // Don't print the error because we assume the error would already have been printed.
        }
    }

    if(makerSettings.mPatchImpl.HasError())
    {
        nErrorCount++;
        Error error = makerSettings.mPatchImpl.GetError();
        EA_UNUSED(error); // Don't print the error because we assume the error would already have been printed.
    }

    if(bWaitAtEnd)
    {
        Report("\nPress any key to exit.\n");
        getchar(); // Wait for the user and shutdown
    }

    return nErrorCount ? -1 : 0;
}







