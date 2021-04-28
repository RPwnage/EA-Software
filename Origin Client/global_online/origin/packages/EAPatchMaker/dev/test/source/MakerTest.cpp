/////////////////////////////////////////////////////////////////////////////
// EAPatchMaker/MakerTest.cpp
//
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include <EAPatchMakerTest/MakerTest.h>
#include <EAPatchMaker/Maker.h>
#include <EAPatchMaker/Version.h>
#include <EAPatchClient/PatchImpl.h>
#include <EATest/EATest.h>
#include <EAMain/EAEntryPointMain.inl>
#include <MemoryMan/MemoryMan.inl>
#include <MemoryMan/CoreAllocator.inl>
#include <EAStdC/EAString.h>
#include <EAStdC/EASprintf.h>
#include <EAIO/PathString.h>
#include <EAIO/EAFileUtil.h>
#include <EAIO/EAFileDirectory.h>


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
PathList                  gPatchDirectoryList;


///////////////////////////////////////////////////////////////////////////////
// ValidateHeap
//
void ValidateHeap()
{
    EA::Allocator::gpEAGeneralAllocator->ValidateHeap(EA::Allocator::GeneralAllocator::kHeapValidationLevelDetail);
}


///////////////////////////////////////////////////////////////////////////////
// SetupData
//
static void SetupData(int argc, char** argv)
{
    using namespace EA::IO;

    EA::EAMain::CommandLine              commandLine(argc, argv);
    EA::EAMain::CommandLine::FixedString sResult;

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

    // Find all directory paths
    if(!gDataDirectory.empty())
    {
        DirectoryIterator            di;
        DirectoryIterator::EntryList entryList;

        di.ReadRecursive(gDataDirectory.c_str(), entryList, EA_WCHAR("patch*"), kDirectoryEntryDirectory, true, true);

        for(DirectoryIterator::EntryList::iterator it = entryList.begin(); it != entryList.end(); ++it)
        {
            const DirectoryIterator::Entry& entry = *it;

            if(entry.msName.find(L"original") >= entry.msName.length()) // If "original" is not in the file name... (we ignore directories with the term "original" in them)
                gPatchDirectoryList.push_back(entry.msName.c_str());
        }
    }

    if(gPatchDirectoryList.empty())
    {
        EA::UnitTest::Report("No patch directories were found.\n");
        EA::UnitTest::Report("You need to run with the working directory set to the full path to EAPatchMaker/dev/test/data or\n");
        EA::UnitTest::Report("you need to run with the -d:<path> command line argument\n");
        EA::UnitTest::Report("The current data directory is \"%ls\".\n", gDataDirectory.c_str());
    }

    gTempDirectory.resize(EA::IO::kMaxDirectoryLength);
    gTempDirectory.resize((eastl_size_t)EA::IO::GetTempDirectory(&gTempDirectory[0]));
    gTempDirectory += L"EAPatchMakerTest";
    gTempDirectory += EA_FILE_PATH_SEPARATOR_STRING_W;
}



static int StaticTestPatch(const EA::IO::Path::PathStringW& dirPath)
{
    int nErrorCount = 0;

    using namespace EA::IO::Path;

    // sInputDirectoryPath8 will be something like "C:\abc\Patch 1\"
    const EA::Patch::String sInputDirectoryPath8(EA::StdC::Strlcpy<EA::Patch::String, EA::IO::Path::PathStringW>(dirPath));

    // sOutputDirectoryPath8 will be something like "C:\output\Patch 1\"
    const EA::Patch::String sOutputDirectoryPath8(EA::StdC::Strlcpy<EA::Patch::String, EA::IO::Path::PathStringW>(gTempDirectory));

    // sOutputDirectoryName will be something like "Patch 1"
    const eastl_size_t pos = sInputDirectoryPath8.find_last_of("\\/", sInputDirectoryPath8.size() - 2);
    const EA::Patch::String sOutputDirectoryName(&sInputDirectoryPath8[pos + 1], sInputDirectoryPath8.length() - 2 - pos);

    // sPatchImplFilePath will be something like "C:\abc\Patch 1\test implementation\Patch 1.eaPatchImpl"
    const EA::Patch::String sPatchImplFilePath(sInputDirectoryPath8 + "test implementation" + EA_FILE_PATH_SEPARATOR_STRING_8 + sOutputDirectoryName + kPatchImplFileNameExtension);
    EATEST_VERIFY(EA::IO::File::Exists(sPatchImplFilePath.c_str()));

    // sPreviousDirectory will be something like "C:\abc\Patch 1 original\"
    const EA::Patch::String sPreviousDirectory(sInputDirectoryPath8.substr(0, sInputDirectoryPath8.length() - 1) + " original" + EA_FILE_PATH_SEPARATOR_STRING_8);

    EA::Patch::Maker         maker;
    EA::Patch::MakerSettings makerSettings;

    makerSettings.mPatchImplPath        = sPatchImplFilePath;
    makerSettings.mInputDirectory       = sInputDirectoryPath8;
    makerSettings.mOutputDirectory      = sOutputDirectoryPath8 + sOutputDirectoryName + EA_FILE_PATH_SEPARATOR_STRING_8;
    makerSettings.mPreviousDirectory    = EA::IO::Directory::Exists(sPreviousDirectory.c_str()) ? sPreviousDirectory : EA::Patch::String();
    makerSettings.mbEnableOptimizations = !makerSettings.mPreviousDirectory.empty();
    makerSettings.mbEnableErrorCleanup  = true;
    EATEST_VERIFY(!makerSettings.mPatchImpl.HasError());

    bool bMakeResult = maker.MakePatch(makerSettings);
    EATEST_VERIFY(bMakeResult);

    EATEST_VERIFY(!maker.HasError());
    if(maker.HasError())
    {
        EA::Patch::String sError;
        EA::Patch::Error  error = maker.GetError();
        error.GetErrorString(sError);
        EA::UnitTest::Report("%s", sError.c_str());
    }

    // Lastly we compare the entries against the "Expected results.txt" values.
    // To do.

    return nErrorCount;
}


///////////////////////////////////////////////////////////////////////////////
// TestBasic
//
int TestBasic()
{
    int nErrorCount = 0;

    for(PathList::iterator it = gPatchDirectoryList.begin(); it != gPatchDirectoryList.end(); ++it)
    {
        const EA::IO::Path::PathStringW& dirPath = *it;
        nErrorCount += StaticTestPatch(dirPath);
    }

    return nErrorCount;
}



///////////////////////////////////////////////////////////////////////////////
// main
//
int EAMain(int argc, char** argv)
{
    EA::EAMain::PlatformStartup();

    #if EA_MEMORY_DEBUG_ENABLED
        if(EA::Allocator::gpEAGeneralAllocatorDebug)
        {
            EA::Allocator::gpEAGeneralAllocatorDebug->SetOption(EA::Allocator::GeneralAllocatorDebug::kOptionEnablePtrValidation, 1);
            #if defined(EA_PLATFORM_DESKTOP)
                EA::Allocator::gpEAGeneralAllocatorDebug->SetDefaultDebugDataFlag(EA::Allocator::GeneralAllocatorDebug::kDebugDataIdGuard);
                EA::Allocator::gpEAGeneralAllocatorDebug->SetGuardSize(3.f, 128, 65536);
            #endif
        }
    #endif

    EA::Allocator::gpEAGeneralAllocator->SetAutoHeapValidation(EA::Allocator::GeneralAllocator::kHeapValidationLevelDetail, 1);

    SetupData(argc, argv);

    // Add the tests
    EA::UnitTest::TestApplication testSuite("EAPatchMaker Unit Tests", argc, argv);

    // Add all tests
    testSuite.AddTest("Basic", TestBasic);

    // Run the prescribed tests.
    const int testResult = testSuite.Run();

    EA::EAMain::PlatformShutdown(testResult);

    return testResult;
}










